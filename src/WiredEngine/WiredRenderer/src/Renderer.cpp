/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Renderer.h"
#include "Global.h"
#include "TransferBufferPool.h"
#include "Textures.h"
#include "Meshes.h"
#include "Materials.h"
#include "Samplers.h"
#include "Pipelines.h"
#include "Groups.h"
#include "Group.h"

#include "DrawPass/ObjectDrawPass.h"

#include "Renderer/ObjectRenderer.h"
#include "Renderer/SpriteRenderer.h"
#include "Renderer/EffectRenderer.h"
#include "Renderer/SkyBoxRenderer.h"

#include <Wired/Render/Metrics.h>
#include <Wired/Render/Task/RenderGroupTask.h>
#include <Wired/Render/Task/PresentToSwapChainTask.h>

#include <Wired/GPU/WiredGPU.h>

#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Metrics/IMetrics.h>
#include <NEON/Common/Timer.h>

#ifdef WIRED_IMGUI
    #include <imgui.h>
    #include <imgui_impl_vulkan.h>
#endif

namespace Wired::Render
{

Renderer::Renderer(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, GPU::WiredGPU* pGPU)
    : m_pGPU(pGPU)
    , m_global(std::make_unique<Global>())
    , m_transferBufferPool(std::make_unique<TransferBufferPool>(m_global.get()))
    , m_textures(std::make_unique<Textures>(m_global.get()))
    , m_meshes(std::make_unique<Meshes>(m_global.get()))
    , m_materials(std::make_unique<Materials>(m_global.get()))
    , m_samplers(std::make_unique<Samplers>(m_global.get()))
    , m_pipelines(std::make_unique<Pipelines>(m_global.get()))
    , m_groups(std::make_unique<Groups>(m_global.get()))
    , m_objectRenderer(std::make_unique<ObjectRenderer>(m_global.get()))
    , m_spriteRenderer(std::make_unique<SpriteRenderer>(m_global.get()))
    , m_effectRenderer(std::make_unique<EffectRenderer>(m_global.get()))
    , m_skyBoxRenderer(std::make_unique<SkyBoxRenderer>(m_global.get()))
{
    m_global->pLogger = pLogger;
    m_global->pMetrics = pMetrics;
    m_global->pGPU = pGPU;
    m_global->pTransferBufferPool = m_transferBufferPool.get();
    m_global->pTextures = m_textures.get();
    m_global->pMeshes = m_meshes.get();
    m_global->pMaterials = m_materials.get();
    m_global->pSamplers = m_samplers.get();
    m_global->pPipelines = m_pipelines.get();
    m_global->pGroups = m_groups.get();
}

Renderer::~Renderer()
{
    m_pGPU = nullptr;
    m_groups = {};
    m_pipelines = {};
    m_samplers = {};
    m_materials = {};
    m_meshes = {};
    m_textures = {};
    m_transferBufferPool = {};
    m_global = {};
}

static GPU::GPUSettings GPUSettingsFromRenderSettings(const RenderSettings& renderSettings)
{
    return GPU::GPUSettings{
        .presentMode = renderSettings.presentMode,
        .framesInFlight = renderSettings.framesInFlight,
        .samplerAnisotropy = renderSettings.samplerAnisotropy,
        .numTimestamps = 64
    };
}

bool Renderer::StartUp(const std::optional<std::unique_ptr<GPU::SurfaceDetails>>& surfaceDetails,
                       const GPU::ShaderBinaryType& shaderBinaryType,
                       const std::optional<GPU::ImGuiGlobals>& imGuiGlobals,
                       const RenderSettings& renderSettings)
{
    m_global->pLogger->Info("Renderer: Starting Up");

    //
    // Store Data
    //
    std::optional<GPU::SurfaceDetails*> pSurfaceDetails;
    if (surfaceDetails)
    {
        pSurfaceDetails = surfaceDetails->get();
    }

    m_global->headless = !pSurfaceDetails.has_value();
    m_global->shaderBinaryType = shaderBinaryType;
    m_global->renderSettings = renderSettings;

    //
    // Start the GPU
    //
    const auto gpuSettings = GPUSettingsFromRenderSettings(renderSettings);

    if (!m_pGPU->StartUp(pSurfaceDetails, imGuiGlobals, gpuSettings))
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the GPU system");
        return false;
    }

    //
    // Init ImGui
    //
    m_global->imGuiActive = imGuiGlobals.has_value();

    if (m_global->imGuiActive)
    {
        #ifdef WIRED_IMGUI
            ImGui::SetCurrentContext(imGuiGlobals->pImGuiContext);
            ImGui::SetAllocatorFunctions(imGuiGlobals->pImGuiMemAllocFunc, imGuiGlobals->pImGuiMemFreeFunc, nullptr);
        #endif
    }

    //
    // Start internal systems
    //
    if (!m_textures->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the textures system");
        return false;
    }

    if (!m_meshes->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the meshes system");
        return false;
    }

    if (!m_materials->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the materials system");
        return false;
    }

    if (!m_samplers->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the samplers system");
        return false;
    }

    if (!m_groups->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the groups system");
        return false;
    }

    //
    // Start renderers
    //
    if (!m_objectRenderer->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the object renderer");
        return false;
    }

    if (!m_spriteRenderer->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the sprite renderer");
        return false;
    }

    if (!m_effectRenderer->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the effect renderer");
        return false;
    }

    if (!m_skyBoxRenderer->StartUp())
    {
        m_global->pLogger->Fatal("Renderer: Failed to start up the sky box renderer");
        return false;
    }

    //
    // Start our render thread
    //
    m_thread = std::make_unique<NCommon::MessageDrivenThreadPool>("Render", 1, std::nullopt, [this](){ OnIdle(); });

    return true;
}

void Renderer::ShutDown()
{
    m_global->pLogger->Info("Renderer: Shutting Down");

    // Stop our render thread
    m_thread = {};

    m_transferBufferPool->Destroy();

    // Shut down renderers
    m_skyBoxRenderer->ShutDown();
    m_effectRenderer->ShutDown();
    m_spriteRenderer->ShutDown();
    m_objectRenderer->ShutDown();

    // Shut down internal systems
    m_groups->ShutDown();
    m_pipelines->ShutDown();
    m_samplers->ShutDown();
    m_materials->ShutDown();
    m_meshes->ShutDown();
    m_textures->ShutDown();
    m_pGPU->ShutDown();

    m_global->ids.Reset();
    m_global->renderSettings = {};
}

RenderSettings Renderer::GetRenderSettings() const
{
    return m_global->renderSettings;
}

bool Renderer::IsImGuiActive() const
{
    return m_global->imGuiActive;
}

std::future<bool> Renderer::SurfaceDetailsChanged(std::unique_ptr<GPU::SurfaceDetails> surfaceDetails)
{
    const std::shared_ptr<GPU::SurfaceDetails> surfaceDetailsShared = std::move(surfaceDetails);
    return m_thread->DispatchForResult("SurfaceDetailsChanged", [=,this](){ OnSurfaceDetailsChanged(surfaceDetailsShared); return true; });
}

void Renderer::OnSurfaceDetailsChanged(const std::shared_ptr<GPU::SurfaceDetails>& surfaceDetails)
{
    m_pGPU->OnSurfaceDetailsChanged(surfaceDetails.get());
}

std::future<bool> Renderer::RenderSettingsChanged(const RenderSettings& renderSettings)
{
    return m_thread->DispatchForResult("SetRenderSettings", [=,this](){ OnRenderSettingsChanged(renderSettings); return true; });
}

void Renderer::OnRenderSettingsChanged(const RenderSettings& renderSettings)
{
    m_global->pLogger->Info("Renderer: Received new render settings");

    //
    // Update for new render settings
    //
    m_global->renderSettings = renderSettings;

    const auto commandBufferId = m_global->pGPU->AcquireCommandBuffer(true, "OnRenderSettingsChanged");

        // Let dependent systems know
        m_effectRenderer->OnRenderSettingsChanged();
        m_groups->OnRenderSettingsChanged(*commandBufferId);

    (void)m_global->pGPU->SubmitCommandBuffer(*commandBufferId);

    //
    // Update GPU for new GPU settings
    //
    const auto gpuSettings = GPUSettingsFromRenderSettings(renderSettings);
    m_global->pGPU->OnGPUSettingsChanged(gpuSettings);
}

#ifdef WIRED_IMGUI
void Renderer::StartImGuiFrame()
{
    if (!m_global->imGuiActive) { return; }

    ImGui_ImplVulkan_NewFrame();
}

std::optional<ImTextureID> Renderer::CreateImGuiTextureReference(TextureId textureId, DefaultSampler sampler)
{
    if (!m_global->imGuiActive) { return std::nullopt; }

    const auto texture = m_textures->GetTexture(textureId);
    if (!texture)
    {
        m_global->pLogger->Error("Renderer::CreateImGuiTextureReference: No such texture exists: {}", textureId.id);
        return std::nullopt;
    }

    const auto samplerId = m_samplers->GetDefaultSampler(sampler);

    return m_pGPU->CreateImGuiImageReference(texture->imageId, samplerId);
}
#endif

std::future<bool> Renderer::CreateShader(const GPU::ShaderSpec& shaderSpec)
{
    return m_thread->DispatchForResult("CreateShader", [=,this](){ return OnCreateShader(shaderSpec); });
}

bool Renderer::OnCreateShader(const GPU::ShaderSpec& shaderSpec)
{
    return m_pGPU->CreateShader(shaderSpec);
}

std::future<bool> Renderer::DestroyShader(const std::string& shaderName)
{
    return m_thread->DispatchForResult("DestroyShader", [=,this](){ return OnDestroyShader(shaderName); });
}

bool Renderer::OnDestroyShader(const std::string& shaderName)
{
    m_pGPU->DestroyShader(shaderName);
    return true;
}

std::future<std::expected<TextureId, bool>> Renderer::CreateTexture_RenderTarget(const TextureUsageFlags& usages, const std::string& tag)
{
    return m_thread->DispatchForResult("CreateTexture_RenderTarget", [=,this](){ return OnCreateTexture_RenderTarget(usages, tag); });
}

std::expected<TextureId, bool> Renderer::OnCreateTexture_RenderTarget(const TextureUsageFlags& usages, const std::string& tag)
{
    if (!usages.contains(Render::TextureUsageFlag::ColorTarget) && !usages.contains(Render::TextureUsageFlag::DepthStencilTarget))
    {
        m_global->pLogger->Error("Renderer::OnCreateTexture_RenderTarget: Usage must contain either ColorTarget or DepthStencilTarget", tag);
        return std::unexpected(false);
    }

    auto realUsageFlags = usages;
    realUsageFlags.insert(TextureUsageFlag::TransferSrc); // All render targets should support being blitted to the present image
    realUsageFlags.insert(TextureUsageFlag::TransferDst);  // All render targets should support being cleared

    const auto textureCreateParams = TextureCreateParams{
        .textureType = TextureType::Texture2D,
        .usageFlags = realUsageFlags,
        .size = {m_global->renderSettings.resolution.GetWidth(), m_global->renderSettings.resolution.GetHeight(), 1U},
        .numLayers = 1,
        .numMipLevels = 1
    };

    const auto commandBufferId = m_pGPU->AcquireCommandBuffer(true, "CreateRenderTarget");
    if (!commandBufferId)
    {
        m_global->pLogger->Error("Renderer::OnCreateTexture_RenderTarget: Failed to acquire a command buffer");
        return std::unexpected(false);
    }

    const auto result = m_textures->CreateFromParams(*commandBufferId, textureCreateParams, tag);
    if (!result)
    {
        m_global->pLogger->Error("Renderer::OnCreateTexture_RenderTarget: Failed to create texture for: {}", tag);
        m_pGPU->CancelCommandBuffer(*commandBufferId);
        return std::unexpected(false);
    }

    (void)m_pGPU->SubmitCommandBuffer(*commandBufferId);
    return *result;
}

std::future<std::expected<TextureId, bool>> Renderer::CreateTexture_FromImage(const NCommon::ImageData* pImageData, TextureType textureType, bool generateMipMaps, const std::string& tag)
{
    return m_thread->DispatchForResult("OnCreateTexture_FromImage", [=,this](){ return OnCreateTexture_FromImage(pImageData, textureType, generateMipMaps, tag); });
}

std::expected<TextureId, bool> Renderer::OnCreateTexture_FromImage(const NCommon::ImageData* pImageData, TextureType textureType, bool generateMipMaps, const std::string& tag)
{
    unsigned int numMipLevels = 1;

    if (generateMipMaps)
    {
        numMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(pImageData->GetPixelWidth(), pImageData->GetPixelHeight())))) + 1;
    }

    GPU::ColorSpace colorSpace{};

    switch (pImageData->GetPixelFormat())
    {
        case NCommon::ImageData::PixelFormat::B8G8R8A8_SRGB: colorSpace = GPU::ColorSpace::SRGB; break;
        case NCommon::ImageData::PixelFormat::B8G8R8A8_LINEAR: colorSpace = GPU::ColorSpace::Linear; break;
    }

    auto textureCreateParams = TextureCreateParams{
        .textureType = textureType,
        .usageFlags = {TextureUsageFlag::GraphicsSampled},
        .size = {(uint32_t)pImageData->GetPixelWidth(), (uint32_t)pImageData->GetPixelHeight(), 1U},
        .colorSpace = colorSpace,
        .numLayers = pImageData->GetNumLayers(),
        .numMipLevels = numMipLevels
    };

    const auto commandBufferId = m_pGPU->AcquireCommandBuffer(true, "OnCreateTexture_2DFromImage");
    if (!commandBufferId)
    {
        m_global->pLogger->Error("Renderer::OnCreateTexture_2DFromImage: Failed to acquire a command buffer");
        return std::unexpected(false);
    }

    // Create the texture
    const auto textureId = m_textures->CreateFromParams(*commandBufferId, textureCreateParams, tag);
    if (!textureId)
    {
        m_global->pLogger->Error("Renderer::OnCreateTexture_2DFromImage: Failed to create texture for: {}", tag);
        m_pGPU->CancelCommandBuffer(*commandBufferId);
        return std::unexpected(false);
    }

    // Transfer the image data to the first mip level of the texture's layers
    for (unsigned int layerIndex = 0; layerIndex < pImageData->GetNumLayers(); ++layerIndex)
    {
        const auto textureTransfer = TextureTransfer{
            // Source
            .data = pImageData->GetPixelData(layerIndex, 0),
            .dataByteSize = pImageData->GetLayerByteSize(),
            // Dest
            .textureId = *textureId,
            .level = 0,
            .layer = layerIndex,
            .destSize = std::nullopt, // Use dest image size
            .x = 0,
            .y = 0,
            .z = 1, // Depth of 1 for 2D textures
            .cycle = false // No need to cycle since the texture is newly created
        };

        if (!m_textures->TransferData(*commandBufferId, {textureTransfer}))
        {
            m_textures->DestroyTexture(*textureId);
            m_pGPU->CancelCommandBuffer(*commandBufferId);
            return std::unexpected(false);
        }
    }

    // Generate mipmap levels, if needed
    if (generateMipMaps && !m_textures->GenerateMipMaps(*commandBufferId, *textureId))
    {
        m_global->pLogger->Error("Renderer::OnCreateTexture_2DFromImage: Failed to generate mipmaps for: {}", tag);
    }

    (void)m_pGPU->SubmitCommandBuffer(*commandBufferId);

    return *textureId;
}

std::optional<NCommon::Size3DUInt> Renderer::GetTextureSize(TextureId textureId)
{
    const auto loadedTexture = m_textures->GetTexture(textureId);
    if (!loadedTexture)
    {
        return std::nullopt;
    }

    return loadedTexture->createParams.size;
}

std::future<bool> Renderer::DestroyTexture(TextureId textureId)
{
    return m_thread->DispatchForResult("DestroyTexture", [=,this](){ return OnDestroyTexture(textureId); });
}

bool Renderer::OnDestroyTexture(TextureId textureId)
{
    m_textures->DestroyTexture(textureId);
    return true;
}

std::future<std::expected<std::vector<MeshId>, bool>> Renderer::CreateMeshes(const std::vector<const Mesh*>& meshes)
{
    return m_thread->DispatchForResult("CreateMeshes", [=,this](){ return OnCreateMeshes(meshes); });
}

std::expected<std::vector<MeshId>, bool> Renderer::OnCreateMeshes(const std::vector<const Mesh*>& meshes)
{
    return m_meshes->CreateMeshes(meshes);
}

std::future<bool> Renderer::DestroyMesh(MeshId meshId)
{
    return m_thread->DispatchForResult("DestroyMesh", [=,this](){ return OnDestroyMesh(meshId); });
}

bool Renderer::OnDestroyMesh(MeshId meshId)
{
    m_meshes->DestroyMesh(meshId);
    return true;
}

MeshId Renderer::GetSpriteMeshId() const
{
    return m_global->spriteMeshId;
}

std::future<std::expected<std::vector<MaterialId>, bool>> Renderer::CreateMaterials(const std::vector<const Material*>& materials, const std::string& userTag)
{
    return m_thread->DispatchForResult("CreateMaterials", [=,this](){ return OnCreateMaterials(materials, userTag); });
}

std::expected<std::vector<MaterialId>, bool> Renderer::OnCreateMaterials(const std::vector<const Material*>& materials, const std::string& userTag)
{
    return m_materials->CreateMaterials(materials, userTag);
}

std::future<bool> Renderer::UpdateMaterial(MaterialId materialId, const Material* pMaterial)
{
    return m_thread->DispatchForResult("UpdateMaterial", [=,this](){ return OnUpdateMaterial(materialId, pMaterial); });
}

bool Renderer::OnUpdateMaterial(MaterialId materialId, const Material* pMaterial)
{
    return m_materials->UpdateMaterial(materialId, pMaterial);
}

std::future<bool> Renderer::DestroyMaterial(MaterialId materialId)
{
    return m_thread->DispatchForResult("DestroyMaterial", [=,this](){ return OnDestroyMaterial(materialId); });
}

ObjectId Renderer::CreateObjectId()
{
    return m_global->ids.objectIds.GetId();
}

SpriteId Renderer::CreateSpriteId()
{
    return m_global->ids.spriteIds.GetId();
}

LightId Renderer::CreateLightId()
{
    return m_global->ids.lightIds.GetId();
}

bool Renderer::OnDestroyMaterial(MaterialId materialId)
{
    m_materials->DestroyMaterial(materialId);
    return true;
}

void Renderer::OnIdle()
{
    m_pGPU->RunCleanUp(true);
}

std::future<std::expected<bool, GPU::SurfaceError>> Renderer::RenderFrame(const RenderFrameParams& renderFrameParams)
{
    return m_thread->DispatchForResult("RenderFrame", [=,this](){ return OnRenderFrame(renderFrameParams); });
}

std::expected<bool, GPU::SurfaceError> Renderer::OnRenderFrame(const RenderFrameParams& renderFrameParams)
{
    m_pGPU->StartFrame();

    auto allFrameWorkTimer = NCommon::Timer(METRIC_RENDERER_CPU_ALL_FRAME_WORK);

    ////////////////////////
    // Apply State Updates
    ////////////////////////

    if (!renderFrameParams.stateUpdates.empty())
    {
        const auto stateUpdatesCommandBufferId = *m_pGPU->AcquireCommandBuffer(true, "StateUpdates");

        for (const auto& stateUpdate: renderFrameParams.stateUpdates)
        {
            ApplyStateUpdate(stateUpdatesCommandBufferId, stateUpdate);
        }

        (void)m_pGPU->SubmitCommandBuffer(stateUpdatesCommandBufferId);
    }

    ////////////////////////
    // Execute Render Tasks
    ////////////////////////

    m_pGPU->SyncDownFrameTimestamps();
    UpdateGPUTimestampMetrics();

    const auto renderCommandBufferId = *m_pGPU->AcquireCommandBuffer(true, "Render");

    m_pGPU->ResetFrameTimestampsForRecording(renderCommandBufferId);

    m_pGPU->CmdWriteTimestampStart(renderCommandBufferId, METRIC_RENDERER_GPU_ALL_FRAME_WORK);

    for (const auto& renderTask : renderFrameParams.renderTasks)
    {
        const auto processResult = ProcessRenderTask(renderCommandBufferId, renderFrameParams, renderTask);
        if (!processResult)
        {
            m_pGPU->CancelCommandBuffer(renderCommandBufferId);
            m_pGPU->EndFrame();
            return processResult;
        }
    }

    m_pGPU->CmdWriteTimestampFinish(renderCommandBufferId, METRIC_RENDERER_GPU_ALL_FRAME_WORK);

    const auto submitResult = m_pGPU->SubmitCommandBuffer(renderCommandBufferId);
    if (!submitResult)
    {
        m_global->pLogger->Info("Renderer::OnRenderFrame: Failed to submit frame command buffer");
        m_pGPU->EndFrame();
        return submitResult;
    }

    m_pGPU->EndFrame();

    allFrameWorkTimer.StopTimer(m_global->pMetrics);

    return true;
}

void Renderer::ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate)
{
    if (stateUpdate.IsEmpty()) { return; }

    const auto group = m_groups->GetOrCreateGroup(stateUpdate.groupName);
    if (!group)
    {
        m_global->pLogger->Error("Renderer::ApplyStateUpdate: Failed to get or create group: {}", stateUpdate.groupName);
        return;
    }

    (*group)->ApplyStateUpdate(commandBufferId, stateUpdate);
}

std::expected<bool, GPU::SurfaceError> Renderer::ProcessRenderTask(GPU::CommandBufferId commandBufferId,
                                                                   const RenderFrameParams& renderFrameParams,
                                                                   const std::shared_ptr<RenderTask>& renderTask)
{
    switch (renderTask->GetType())
    {
        case RenderTask::Type::RenderGroup:
        {
            ProcessRenderTask_RenderGroup(commandBufferId, renderTask);
        }
        break;
        case RenderTask::Type::PresentToSwapChain:
        {
            const auto result = ProcessRenderTask_PresentToSwapChain(commandBufferId, renderFrameParams, renderTask);
            if (!result) { return result; }
        }
        break;
    }

    return true;
}

void Renderer::ProcessRenderTask_RenderGroup(GPU::CommandBufferId commandBufferId, const std::shared_ptr<RenderTask>& renderTask)
{
    const auto renderGroupTask = std::dynamic_pointer_cast<RenderGroupTask>(renderTask);

    const auto renderGroup = m_groups->GetOrCreateGroup(renderGroupTask->groupName);
    if (!renderGroup)
    {
        m_global->pLogger->Error("RendererSDL::ProcessRenderTask_RenderGroup: Failed to get/create render group: {}", renderGroupTask->groupName);
        return;
    }
    const auto pGroup = *renderGroup;

    const auto worldCameraViewProjection = GetWorldCameraViewProjection(m_global->renderSettings, renderGroupTask->worldCamera);
    if (!worldCameraViewProjection)
    {
        m_global->pLogger->Error("RendererSDL::RecordDrawCommands: Failed to get world camera view projection");
        return;
    }

    const auto spriteCameraViewProjection = GetScreenCameraViewProjection(m_global->renderSettings, renderGroupTask->spriteCamera);
    if (!spriteCameraViewProjection)
    {
        m_global->pLogger->Error("RendererSDL::RecordDrawCommands: Failed to get screen camera view projection");
        return;
    }

    if (renderGroupTask->targetColorTextureIds.empty() && !renderGroupTask->targetDepthTextureId)
    {
        m_global->pLogger->Error("RendererSDL::RecordDrawCommands: Need at least one color or depth texture for a render target");
        return;
    }

    NCommon::Size3DUInt renderExtent;

    std::vector<GPU::ColorRenderAttachment> colorAttachments;
    for (const auto& colorTextureId : renderGroupTask->targetColorTextureIds)
    {
        const auto colorTexture = m_global->pTextures->GetTexture(colorTextureId);
        if (!colorTexture)
        {
            m_global->pLogger->Error("RendererSDL::RecordDrawCommands: No such color texture exists: {}", colorTextureId.id);
            return;
        }

        colorAttachments.push_back(GPU::ColorRenderAttachment{
           .imageId = colorTexture->imageId,
           .mipLevel = 0,
           .layer = 0,
           .loadOp = GPU::LoadOp::Clear,
           .storeOp = GPU::StoreOp::Store,
           .clearColor = glm::vec4(renderGroupTask->clearColor, 1.0f),
           .cycle = true
        });

        renderExtent = colorTexture->createParams.size;
    }

    std::optional<GPU::DepthRenderAttachment> depthAttachment;

    if (renderGroupTask->targetDepthTextureId)
    {
        const auto depthTexture = m_global->pTextures->GetTexture(*renderGroupTask->targetDepthTextureId);
        if (!depthTexture)
        {
            m_global->pLogger->Error("RendererSDL::RecordDrawCommands: No such depth texture exists: {}", renderGroupTask->targetDepthTextureId->id);
            return;
        }

        depthAttachment = GPU::DepthRenderAttachment{
            .imageId = depthTexture->imageId,
            .loadOp = GPU::LoadOp::Clear,
            .storeOp = GPU::StoreOp::Store,
            .clearDepth = 0.0f, // Reversed z-axis
            .cycle = true
        };

        renderExtent = depthTexture->createParams.size;
    }

    //
    // Update camera-dependent draw passes with the latest camera view projection
    //
    (*pGroup->GetDrawPasses().GetDrawPass(DRAW_PASS_CAMERA_OBJECT_OPAQUE))->SetViewProjection(*worldCameraViewProjection);
    (*pGroup->GetDrawPasses().GetDrawPass(DRAW_PASS_CAMERA_OBJECT_TRANSLUCENT))->SetViewProjection(*worldCameraViewProjection);
    (*pGroup->GetDrawPasses().GetDrawPass(DRAW_PASS_CAMERA_SPRITE))->SetViewProjection(*spriteCameraViewProjection);

    //
    // Let group lights process the latest camera. This allows for invalidating directional shadow
    // renders which depend on the camera's current position (Note: this doesn't change any GPU state)
    //
    pGroup->GetLights().ProcessLatestWorldCamera(renderGroupTask->worldCamera);

    //
    // Let group lights run its shadow render sync flow. This invalidates shadow renders whose
    // draw pass is invalidated (e.g. from object updates), invalidates any shadow render draw
    // pass which needs a new view projection (e.g. directional lights), and right now it ends
    // by just bulk enqueueing all invalidated shader renders for refreshing, but in the future
    // refreshing can be delayed/staggered for better perf.
    //
    pGroup->GetLights().SyncShadowRenders(commandBufferId);

    //
    // Re-compute draw calls for all invalidated group draw passes. Note: This should happen
    // after shadow render draw passes are invalidated as needed by group lights (see above).
    //
    pGroup->GetDrawPasses().ComputeDrawCallsIfNeeded(commandBufferId);

    //
    // Record shadow map draw commands. Note: This should happen after the draw passes for the
    // shadow renders are recomputed (see above).
    //
    RecordShadowMapRenders(pGroup, commandBufferId);

    //
    // Draw the group
    //
    auto renderPass = m_pGPU->BeginRenderPass(
        commandBufferId,
        colorAttachments,
        depthAttachment,
        {0,0},
        {renderExtent.w, renderExtent.h},
        std::format("Render-{}", pGroup->GetName())
    );

    RendererInput rendererInput{};
    rendererInput.commandBuffer = commandBufferId;
    rendererInput.renderPass = *renderPass;
    rendererInput.colorAttachments = colorAttachments;
    rendererInput.depthAttachment = depthAttachment;
    rendererInput.worldViewProjection = *worldCameraViewProjection;
    rendererInput.screenViewProjection = *spriteCameraViewProjection;
    rendererInput.viewPort = {0, 0, renderExtent.w, renderExtent.h};
    rendererInput.skyBoxTextureId = renderGroupTask->skyBoxTextureId;
    rendererInput.skyBoxTransform = renderGroupTask->skyBoxTransform;

    RecordGroupCameraDrawPassCommands(pGroup, rendererInput);

    m_pGPU->EndRenderPass(*renderPass);

    //
    // Post process effects
    //
    if (!colorAttachments.empty())
    {
        const auto colorAttachment = renderGroupTask->targetColorTextureIds.at(0);

        m_effectRenderer->RunEffect(commandBufferId, *ColorCorrectionEffect(m_global.get()), colorAttachment);

        if (m_global->renderSettings.fxaa)
        {
            m_effectRenderer->RunEffect(commandBufferId, *FXAAEffect(m_global.get()), colorAttachment);
        }
    }
}

void Renderer::RecordGroupCameraDrawPassCommands(Group* pGroup, const RendererInput& rendererInput)
{
    //
    // Draw group objects, from the camera's perspective
    //
    const auto pOpaqueDrawPass = dynamic_cast<const ObjectDrawPass*>(*(pGroup)->GetDrawPasses().GetDrawPass(DRAW_PASS_CAMERA_OBJECT_OPAQUE));
    const auto pTranslucentDrawPass = dynamic_cast<const ObjectDrawPass*>(*(pGroup)->GetDrawPasses().GetDrawPass(DRAW_PASS_CAMERA_OBJECT_TRANSLUCENT));

    m_objectRenderer->RenderGpass(rendererInput, pGroup, pOpaqueDrawPass);
    m_objectRenderer->RenderGpass(rendererInput, pGroup, pTranslucentDrawPass);

    //
    // Draw group sprites, from the camera's perspective
    //
    const auto spriteDrawPass = dynamic_cast<const SpriteDrawPass*>(*(pGroup)->GetDrawPasses().GetDrawPass(DRAW_PASS_CAMERA_SPRITE));

    m_spriteRenderer->Render(rendererInput, pGroup, spriteDrawPass);

    //
    // Draw skybox, if applicable. After everything else is rendered, to reduce overdraw
    //
    m_skyBoxRenderer->Render(rendererInput);
}

void Renderer::RecordShadowMapRenders(Group* pGroup, GPU::CommandBufferId commandBufferId)
{
    m_pGPU->CmdWriteTimestampStart(commandBufferId, METRIC_RENDERER_GPU_ALL_SHADOW_MAP_RENDER_WORK);

    std::unordered_set<LightId> refreshedShadowRenders;

    for (const auto& lightStateIt : pGroup->GetLights().GetAll())
    {
        const auto& lightState = lightStateIt.second;

        if (!lightState.light.castsShadows)
        {
            continue;
        }

        if (!lightState.shadowMapTextureId)
        {
            m_global->pLogger->Error("Renderer::RecordShadowMapRenders: Light has no shadow map texture: {}", lightStateIt.first.id);
            continue;
        }

        const auto shadowMapTexture = m_textures->GetTexture(*lightState.shadowMapTextureId);
        if (!shadowMapTexture)
        {
            m_global->pLogger->Error("Renderer::RecordShadowMapRenders: No such shadow map texture exists: {}",lightState.shadowMapTextureId->id);
            continue;
        }

        const auto shadowMapExtent = shadowMapTexture->createParams.size;

        std::unordered_set<uint8_t> renderedShadowRenderIndices;

        for (unsigned int shadowRenderIndex = 0; shadowRenderIndex < lightState.shadowRenders.size(); ++shadowRenderIndex)
        {
            const auto& shadowRender = lightState.shadowRenders.at(shadowRenderIndex);

            if (shadowRender.state != ShadowRender::State::PendingRender)
            {
                continue;
            }

            const auto depthAttachment = GPU::DepthRenderAttachment {
                .imageId = shadowMapTexture->imageId,
                .mipLevel = 0,
                .layer = shadowRenderIndex, // Shadow map should have one layer per shadow render
                .loadOp = GPU::LoadOp::Clear,
                .storeOp = GPU::StoreOp::Store,
                .clearDepth = 0.0f, // Reversed z-axis
                .cycle = false
            };

            const auto renderPass = m_pGPU->BeginRenderPass(
                commandBufferId,
                {},
                depthAttachment,
                {0,0},
                {shadowMapExtent.w, shadowMapExtent.h},
                std::format("ShadowRender-{}", pGroup->GetName())
            );

                RendererInput rendererInput{};
                rendererInput.commandBuffer = commandBufferId;
                rendererInput.renderPass = *renderPass;
                rendererInput.colorAttachments = {};
                rendererInput.depthAttachment = depthAttachment;
                rendererInput.worldViewProjection = *shadowRender.pShadowDrawPass->GetViewProjection();
                rendererInput.screenViewProjection = {};
                rendererInput.viewPort = {0, 0, shadowMapExtent.w, shadowMapExtent.h};

                m_objectRenderer->RenderShadowMap(rendererInput, pGroup, shadowRender.pShadowDrawPass, lightState.light);

            m_pGPU->EndRenderPass(*renderPass);

            renderedShadowRenderIndices.insert((uint8_t)shadowRenderIndex);
        }

        pGroup->GetLights().MarkShadowRendersSynced(lightState.light.id, renderedShadowRenderIndices);
    }

    m_pGPU->CmdWriteTimestampFinish(commandBufferId, METRIC_RENDERER_GPU_ALL_SHADOW_MAP_RENDER_WORK);
}

std::expected<bool, GPU::SurfaceError> Renderer::ProcessRenderTask_PresentToSwapChain(GPU::CommandBufferId commandBufferId,
                                                                                      const RenderFrameParams& renderFrameParams,
                                                                                      const std::shared_ptr<RenderTask>& renderTask)
{
    const auto presentToSwapChainTask = std::dynamic_pointer_cast<PresentToSwapChainTask>(renderTask);

    std::optional<LoadedTexture> presentTexture;
    if (presentToSwapChainTask->presentTextureId)
    {
        presentTexture = m_textures->GetTexture(*presentToSwapChainTask->presentTextureId);
        if (!presentTexture)
        {
            m_global->pLogger->Error("Renderer::ProcessRenderTask_PresentToSwapChain: No such present texture exists: {}",
                                     presentToSwapChainTask->presentTextureId->id);
            // Note that we allow execution to continue, as if no present texture was supplied
        }
    }

    const auto swapChainImageId = m_pGPU->AcquireSwapChainImage(commandBufferId);
    if (!swapChainImageId)
    {
        m_global->pLogger->Warning("Renderer::ProcessRenderTask_PresentToSwapChain: Failed to acquire swap chain image");
        return std::unexpected(swapChainImageId.error());
    }

    //
    // Clear the swap chain image and blit the present texture on top of it
    //
    const auto copyPass = m_pGPU->BeginCopyPass(commandBufferId, "BlitToSwapChain");
        //
        // Clear the swap chain image
        //
        m_pGPU->CmdClearColorImage(*copyPass, *swapChainImageId, GPU::OneLevelOneLayerColorImageRange, glm::vec4(presentToSwapChainTask->clearColor, 1.0f), false);

        //
        // If we have a texture to present, blit it to the swap chain image
        //
        if (presentTexture)
        {
            const auto offscreenTextureSize = presentTexture->createParams.size;
            const auto presentTextureSize = m_pGPU->GetSwapChainSize();

            const auto blitRects = NCommon::CalculateBlitRects(
                m_global->renderSettings.presentBlitType,
                NCommon::Size2DReal(static_cast<float>(offscreenTextureSize.w), static_cast<float>(offscreenTextureSize.h)),
                NCommon::Size2DReal(static_cast<float>(presentTextureSize.w), static_cast<float>(presentTextureSize.h))
            );

            m_pGPU->CmdBlitImage(
                *copyPass,
                presentTexture->imageId,
                GPU::ImageRegion{
                    .layerIndex = 0,
                    .mipLevel = 0,
                    .offsets = {
                        NCommon::Point3DUInt{(uint32_t)blitRects.first.x, (uint32_t)blitRects.first.y, 0},
                        NCommon::Point3DUInt{(uint32_t)(blitRects.first.x + blitRects.first.w), (uint32_t)(blitRects.first.y + blitRects.first.h), 0}
                    }
                },
                *swapChainImageId,
                GPU::ImageRegion{
                    .layerIndex = 0,
                    .mipLevel = 0,
                    .offsets = {
                        NCommon::Point3DUInt{(uint32_t)blitRects.second.x, (uint32_t)blitRects.second.y, 0},
                        NCommon::Point3DUInt{(uint32_t)(blitRects.second.x + blitRects.second.w), (uint32_t)(blitRects.second.y + blitRects.second.h), 0}
                    }
                },
                GPU::Filter::Linear,
                false
            );
        }

    m_pGPU->EndCopyPass(*copyPass);

    //
    // As a last step before presentation, record any ImGui draw commands on top of the finished swap chain image
    //
    RecordImGuiDrawData(commandBufferId, *swapChainImageId, renderFrameParams.imDrawData);

    return true;
}

void Renderer::RecordImGuiDrawData(GPU::CommandBufferId commandBufferId, GPU::ImageId swapChainImageId, const std::optional<ImDrawData*>& drawData) const
{
    #ifdef  WIRED_IMGUI

        // If no ImGui to draw, nothing to do, bail out, don't start an empty render pass
        if (!drawData)
        {
            return;
        }

        // Swap chain color attachment for rendering
        const auto colorRenderAttachment = GPU::ColorRenderAttachment{
            .imageId = swapChainImageId,
            .mipLevel = 0,
            .layer = 0,
            .loadOp = GPU::LoadOp::Load,
            .storeOp = GPU::StoreOp::Store,
            .clearColor = {0,0,0,1}, // (not used, LoadOp::Load)
            .cycle = false
        };

        const auto renderPass = m_pGPU->BeginRenderPass(
            commandBufferId,
            {colorRenderAttachment},
            std::nullopt,
            {0,0},
            m_pGPU->GetSwapChainSize(),
            "Render-ImGui"
        );

            m_pGPU->CmdRenderImGuiDrawData(*renderPass, *drawData);

        m_pGPU->EndRenderPass(*renderPass);
    #else
        (void)commandBufferId; (void)swapChainImageId; (void)drawData;
    #endif
}

void RecordTimestampMetric(GPU::WiredGPU* pGPU, NCommon::IMetrics* pMetrics, const std::string& timestampName)
{
    const auto timestampDiffMs = pGPU->GetTimestampDiffMs(timestampName, 0);
    if (timestampDiffMs)
    {
        pMetrics->SetDoubleValue(timestampName, *timestampDiffMs);
    }
}

void Renderer::UpdateGPUTimestampMetrics()
{
    RecordTimestampMetric(m_pGPU, m_global->pMetrics, METRIC_RENDERER_GPU_ALL_FRAME_WORK);
    RecordTimestampMetric(m_pGPU, m_global->pMetrics, METRIC_RENDERER_GPU_ALL_SHADOW_MAP_RENDER_WORK);
}

}
