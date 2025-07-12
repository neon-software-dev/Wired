/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ObjectRenderer.h"

#include "../Samplers.h"
#include "../TransferBufferPool.h"
#include "../Group.h"

#include <NEON/Common/Timer.h>

namespace Wired::Render
{

ObjectRenderer::ObjectRenderer(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

ObjectRenderer::~ObjectRenderer()
{
    m_pGlobal = nullptr;
}

bool ObjectRenderer::StartUp()
{
    return true;
}

void ObjectRenderer::ShutDown()
{

}

inline std::string GetObjectDrawPassTypeString(ObjectDrawPassType objectDrawPassType)
{
    switch (objectDrawPassType)
    {
        case ObjectDrawPassType::Opaque: return "Opaque";
        case ObjectDrawPassType::Translucent: return "Translucent";
        case ObjectDrawPassType::ShadowCaster: return "ShadowCaster";
    }

    assert(false);
    return "";
}

void ObjectRenderer::RenderGpass(const RendererInput& input,
                                 const Group* pGroup,
                                 const ObjectDrawPass* pDrawPass)
{
    const auto sectionLabel = std::format("Object:RenderGpass-{}-{}", pGroup->GetName(), GetObjectDrawPassTypeString(pDrawPass->GetObjectDrawPassType()));

    m_pGlobal->pGPU->CmdPushDebugSection(input.renderPass.commandBufferId, sectionLabel);

        Render(input, pGroup, pDrawPass, RenderType::Gpass, std::nullopt);

    m_pGlobal->pGPU->CmdPopDebugSection(input.renderPass.commandBufferId);
}

void ObjectRenderer::RenderShadowMap(const RendererInput& input,
                                     const Group* pGroup,
                                     const ObjectDrawPass* pDrawPass,
                                     const Light& light)
{
    const auto sectionLabel =  std::format("Object:RenderShadowMap-{}-{}-{}", pGroup->GetName(), (uint32_t)pDrawPass->GetObjectDrawPassType(), light.id.id);

    m_pGlobal->pGPU->CmdPushDebugSection(input.renderPass.commandBufferId, sectionLabel);

        Render(input, pGroup, pDrawPass, RenderType::ShadowMap, light);

    m_pGlobal->pGPU->CmdPopDebugSection(input.renderPass.commandBufferId);
}

void ObjectRenderer::Render(const RendererInput& input,
                            const Group* pGroup,
                            const ObjectDrawPass* pDrawPass,
                            const RenderType& renderType,
                            const std::optional<Light>& shadowMapLight)
{
    if (pDrawPass->GetNumObjects() == 0) { return; }

    // Obtain object batches from the object draw pass
    std::vector<ObjectDrawPass::RenderBatch> renderBatches = pDrawPass->GetRenderBatches();

    // Sort the batches for best rendering performance
    SortBatchesForRendering(renderBatches);

    // Render each batch
    RenderState renderState{};

    for (const auto& renderBatch : renderBatches)
    {
        DoRenderBatch(input, pGroup, pDrawPass, renderType, shadowMapLight, renderBatch, renderState);
    }
}

void ObjectRenderer::SortBatchesForRendering(std::vector<ObjectDrawPass::RenderBatch>& batches) const
{
    // Sort by material then by mesh, as at the moment it's expensive to switch materials; we
    // want to render all batches that use the same material before switching to a new material
    std::ranges::sort(batches, [](const auto& o1, const auto& o2){
        return  std::tie(o1.materialId.id, o1.meshId.id) <
                std::tie(o2.materialId.id, o2.meshId.id);
    });
}

void ObjectRenderer::DoRenderBatch(const RendererInput& input,
                                   const Group* pGroup,
                                   const ObjectDrawPass* pDrawPass,
                                   RenderType renderType,
                                   const std::optional<Light>& shadowMapLight,
                                   const ObjectDrawPass::RenderBatch& renderBatch,
                                   RenderState& renderState)
{
    //
    // Fetch required draw data
    //
    const auto loadedMesh = m_pGlobal->pMeshes->GetMesh(renderBatch.meshId);
    if (!loadedMesh)
    {
        m_pGlobal->pLogger->Error("ObjectRenderer::DoRenderBatch: No such mesh exists: {}", renderBatch.meshId.id);
        return;
    }

    const auto loadedMaterial = m_pGlobal->pMaterials->GetMaterial(renderBatch.materialId);
    if (!loadedMaterial)
    {
        m_pGlobal->pLogger->Error("ObjectRenderer::DoRenderBatch: No such material exists: {}", renderBatch.materialId.id);
        return;
    }

    const auto batchInput = BatchInput{
        .pRendererInput = &input,
        .pGroup = pGroup,
        .pDrawPass = pDrawPass,
        .renderType = renderType,
        .renderBatch = renderBatch,
        .loadedMesh = *loadedMesh,
        .loadedMaterial = *loadedMaterial,
        .shadowMapLight = shadowMapLight
    };

    const auto vertexShaderName = GetVertexShaderName(renderType, loadedMesh->meshType);
    if (!vertexShaderName) { return; }

    const auto fragmentShaderName = GetFragmentShaderName(renderType, loadedMaterial->materialType);

    const auto graphicsPipeline = GetGraphicsPipeline(input, renderType, *vertexShaderName, fragmentShaderName, *loadedMaterial);
    if (!graphicsPipeline)
    {
        m_pGlobal->pLogger->Error("ObjectRenderer::DoRenderBatch: Failed to get graphics pipeline");
        return;
    }

    //
    // Bind Graphics State
    //

    // Bind Pipeline
    if (renderState.BindPipeline(*graphicsPipeline))
    {
        m_pGlobal->pGPU->CmdBindPipeline(input.renderPass, *graphicsPipeline);
    }

    // Bind Vertex Buffer
    const auto vertexBufferBinding = GPU::BufferBinding{.bufferId = m_pGlobal->pMeshes->GetVerticesBuffer(loadedMesh->meshType), .byteOffset = 0};
    if (renderState.BindVertexBuffer(vertexBufferBinding))
    {
        m_pGlobal->pGPU->CmdBindVertexBuffers(input.renderPass, 0, {vertexBufferBinding});
    }

    // Bind Index Buffer
    const auto indexBufferBinding = GPU::BufferBinding{.bufferId = m_pGlobal->pMeshes->GetIndicesBuffer(loadedMesh->meshType)};
    if (renderState.BindIndexBuffer(indexBufferBinding))
    {
        m_pGlobal->pGPU->CmdBindIndexBuffer(input.renderPass, indexBufferBinding, GPU::IndexType::Uint32);
    }

    // Bind descriptor sets
    if (renderState.SetNeedsBinding(0)) { BindSet0(batchInput, renderState); }

    const bool set1MeshUpdated = batchInput.loadedMesh.meshType == MeshType::Bone && renderState.BindMesh(renderBatch.meshId);
    if (renderState.SetNeedsBinding(1) || set1MeshUpdated) { BindSet1(batchInput, renderState); }

    if (renderState.SetNeedsBinding(2)) { BindSet2(batchInput, renderState); }

    const bool set3MaterialUpdated = renderState.BindMaterial(renderBatch.materialId);
    if (renderState.SetNeedsBinding(3) || set3MaterialUpdated) { BindSet3(batchInput, renderState); }

    //
    // Draw
    //

    // The draw commands buffer is ordered by batch id, with MESH_MAX_LOD spots for each batch
    const std::size_t drawCommandsByteOffset =
        (renderBatch.batchId * MESH_MAX_LOD) * sizeof(GPU::IndirectDrawCommand);

    // The draw counts buffer is directly indexed by batch id, with a single drawCount spot for each batch
    const std::size_t drawCountsByteOffset =
        renderBatch.batchId * sizeof(DrawCountPayload);

    m_pGlobal->pGPU->CmdDrawIndexedIndirectCount(
        input.renderPass,
        pDrawPass->GetDrawCommandsBuffer(),
        drawCommandsByteOffset,
        pDrawPass->GetDrawCountsBuffer(),
        drawCountsByteOffset,
        MESH_MAX_LOD, // Can be issuing up to a max of MESH_MAX_LOD draw commands for each batch
        sizeof(GPU::IndirectDrawCommand) // Stride
    );
}

// Global resources
void ObjectRenderer::BindSet0(const BatchInput& input, RenderState& renderState)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    if (input.loadedMesh.meshType == MeshType::Bone)
    {
        m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_meshPayloads", m_pGlobal->pMeshes->GetMeshPayloadsBuffer());
    }

    m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_materialPayloads", m_pGlobal->pMaterials->GetMaterialPayloadsBuffer());

    renderState.OnSetBound(0);
}

// Per-group resources
void ObjectRenderer::BindSet1(const BatchInput& input, RenderState& renderState)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    const auto globalPayload = GetGlobalPayload(input.pGroup, input.shadowMapLight);
    const auto viewProjectionPayload = GetViewProjectionPayload(input.pRendererInput->worldViewProjection);

    m_pGlobal->pGPU->CmdBindUniformData(renderPass, "u_globalData", &globalPayload, sizeof(ObjectGlobalUniformPayload));
    m_pGlobal->pGPU->CmdBindUniformData(renderPass, "u_viewProjectionData", &viewProjectionPayload, sizeof(ViewProjectionUniformPayload));
    m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_objectInstanceData", input.pGroup->GetDataStores().objects.GetInstancePayloadsBuffer());
    m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_lightData", input.pGroup->GetDataStores().lights.GetInstancePayloadsBuffer());
    m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_shadowMapData", input.pGroup->GetLights().GetShadowMapPayloadBuffer());

    if (input.renderType == RenderType::Gpass)
    {
        const auto shadowSamplerUniformPayloads = GetShadowSamplerUniformPayloads(input);

        m_pGlobal->pGPU->CmdBindUniformData(renderPass, "u_shadowSamplerData", shadowSamplerUniformPayloads.data(), shadowSamplerUniformPayloads.size() * sizeof(ShadowSamplerUniformPayload));

        BindShadowSamplers(input, shadowSamplerUniformPayloads);
    }

    if (input.loadedMesh.meshType == MeshType::Bone)
    {
        m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_boneTransformsData", input.pGroup->GetDataStores().objects.GetBoneTransformsBuffer(input.renderBatch.meshId));
        m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_boneMappingData", input.pGroup->GetDataStores().objects.GetBoneMappingBuffer(input.renderBatch.meshId));
    }

    renderState.OnSetBound(1);
}

// Per pass resources
void ObjectRenderer::BindSet2(const BatchInput& input, RenderState& renderState)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_drawData", input.pDrawPass->GetDrawDataBuffer());

    renderState.OnSetBound(2);
}

// Per draw resources
void ObjectRenderer::BindSet3(const BatchInput& input, RenderState& renderState)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    // Material Samplers
    const auto materialSamplerBindings = GetSamplerBindings(input.loadedMaterial);
    for (const auto& samplerBinding : materialSamplerBindings)
    {
        m_pGlobal->pGPU->CmdBindImageViewSampler(
            renderPass,
            samplerBinding.first,
            0,
            samplerBinding.second.texture.imageId,
            samplerBinding.second.samplerId
        );
    }

    renderState.OnSetBound(3);
}

std::optional<std::string> ObjectRenderer::GetVertexShaderName(RenderType, MeshType meshType) const
{
    switch (meshType)
    {
        case MeshType::Static: return m_pGlobal->pPipelines->GetShaderNameFromBaseName("mesh.vert");
        case MeshType::Bone: return m_pGlobal->pPipelines->GetShaderNameFromBaseName("mesh_bone.vert");
    }

    assert(false);
    return std::nullopt;
}

std::optional<std::string> ObjectRenderer::GetFragmentShaderName(RenderType renderType, MaterialType materialType) const
{
    switch (renderType)
    {
        case RenderType::Gpass:
        {
            switch (materialType)
            {
                case MaterialType::PBR: return m_pGlobal->pPipelines->GetShaderNameFromBaseName("mesh_pbr.frag");
            }
        }
        break;
        case RenderType::ShadowMap:
        {
            return m_pGlobal->pPipelines->GetShaderNameFromBaseName("mesh_shadow.frag");
        }
    }

    assert(false);
    return std::nullopt;
}

std::expected<GPU::PipelineId, bool> ObjectRenderer::GetGraphicsPipeline(const RendererInput& rendererInput,
                                                                         RenderType renderType,
                                                                         const std::string& vertexShaderName,
                                                                         const std::optional<std::string>& fragmentShaderName,
                                                                         const LoadedMaterial& loadedMaterial) const
{
    auto pipelineParams = GPU::GraphicsPipelineParams{};

    pipelineParams.vertexShaderName = vertexShaderName;
    pipelineParams.fragmentShaderName = fragmentShaderName;
    pipelineParams.colorAttachments = rendererInput.colorAttachments;
    pipelineParams.depthAttachment = rendererInput.depthAttachment;
    pipelineParams.viewport = rendererInput.viewPort;

    if (renderType == RenderType::ShadowMap)
    {
        pipelineParams.depthBiasEnabled = true;
    }

    pipelineParams.wireframeFillMode = m_pGlobal->renderSettings.objectsWireframe;

    if (loadedMaterial.twoSided)
    {
        pipelineParams.cullFace = GPU::CullFace::None;
    }
    else if (renderType == RenderType::ShadowMap)
    {
        pipelineParams.cullFace = GPU::CullFace::Front;
    }

    /*
     * pipelineParams.rasterizerFillMode = m_pShared->renderSettings.objectsWireframe ? RasterizerFillMode::Line : RasterizerFillMode::Fill;
    */

    return m_pGlobal->pPipelines->GetOrCreatePipeline(pipelineParams);
}

ObjectRenderer::ObjectGlobalUniformPayload ObjectRenderer::GetGlobalPayload(const Group* pGroup, const std::optional<Light>& shadowMapLight) const
{
    return ObjectGlobalUniformPayload {
        .surfaceTransform = glm::mat4(1),
        .lightId = shadowMapLight ? shadowMapLight->id.id : 0,
        .ambientLight = m_pGlobal->renderSettings.ambientLight,
        .highestLightId = (uint32_t)pGroup->GetDataStores().lights.GetInstanceCount(),
        .hdrEnabled = m_pGlobal->renderSettings.hdr,
        .shadowCascadeOverlap = m_pGlobal->renderSettings.shadowCascadeOverlapRatio
    };
}

ViewProjectionUniformPayload ObjectRenderer::GetViewProjectionPayload(const ViewProjection& _viewProjection) const
{
    auto viewProjection = _viewProjection;
    const float desiredRenderDistance = std::min(m_pGlobal->renderSettings.maxRenderDistance, m_pGlobal->renderSettings.objectsMaxRenderDistance);
    ReduceFarPlaneDistanceToNoFartherThan(viewProjection, desiredRenderDistance);

    return ViewProjectionPayloadFromViewProjection(viewProjection);
}

std::unordered_map<std::string, ObjectRenderer::TextureSamplerBind> ObjectRenderer::GetSamplerBindings(const LoadedMaterial& material) const
{
    const auto missingTexture = m_pGlobal->pTextures->GetMissingTexture2D();

    const auto missingTextureSamplerBinding = TextureSamplerBind{
        .texture = missingTexture,
        .samplerId = m_pGlobal->pSamplers->GetDefaultSampler(DefaultSampler::LinearRepeat)
    };

    switch (material.materialType)
    {
        case MaterialType::PBR: return GetSamplerBindingsPBR(material, missingTextureSamplerBinding);
    }

    assert(false);
    return {};
}

std::unordered_map<std::string, ObjectRenderer::TextureSamplerBind> ObjectRenderer::GetSamplerBindingsPBR(
    const LoadedMaterial& material,
    const TextureSamplerBind& missingTextureSamplerBinding) const
{
    std::unordered_map<std::string, TextureSamplerBind> samplerBindings;

    samplerBindings.insert({"i_albedoSampler", GetSamplerBinding(MaterialTextureType::Albedo, material, missingTextureSamplerBinding)});
    samplerBindings.insert({"i_metallicSampler", GetSamplerBinding(MaterialTextureType::Metallic, material, missingTextureSamplerBinding)});
    samplerBindings.insert({"i_roughnessSampler", GetSamplerBinding(MaterialTextureType::Roughness, material, missingTextureSamplerBinding)});
    samplerBindings.insert({"i_normalSampler", GetSamplerBinding(MaterialTextureType::Normal, material, missingTextureSamplerBinding)});
    samplerBindings.insert({"i_aoSampler", GetSamplerBinding(MaterialTextureType::AO, material, missingTextureSamplerBinding)});
    samplerBindings.insert({"i_emissionSampler", GetSamplerBinding(MaterialTextureType::Emission, material, missingTextureSamplerBinding)});

    return samplerBindings;
}

ObjectRenderer::TextureSamplerBind ObjectRenderer::GetSamplerBinding(MaterialTextureType materialTextureType,
                                                                     const LoadedMaterial& material,
                                                                     const TextureSamplerBind& missingTextureSamplerBinding) const
{
    const auto it = material.textureBindings.find(materialTextureType);
    if (it == material.textureBindings.cend())
    {
        return missingTextureSamplerBinding;
    }

    const auto loadedTexture = m_pGlobal->pTextures->GetTexture(it->second.textureId);
    if (!loadedTexture)
    {
        return missingTextureSamplerBinding;
    }

    const auto samplerInfo = GPU::SamplerInfo {
        .magFilter = GPU::SamplerFilter::Linear,
        .minFilter = GPU::SamplerFilter::Linear,
        .mipmapMode = GPU::SamplerMipmapMode::Linear,
        .addressModeU = it->second.uSamplerAddressMode,
        .addressModeV = it->second.vSamplerAddressMode,
        .addressModeW = it->second.wSamplerAddressMode,
        .anisotropyEnable = true
    };

    const auto sampler = m_pGlobal->pSamplers->GetOrCreateSampler(samplerInfo, std::format("{}", it->second.textureId.id));
    if (!sampler)
    {
        return missingTextureSamplerBinding;
    }

    return TextureSamplerBind{
        .texture = *loadedTexture,
        .samplerId = *sampler
    };
}

std::array<ShadowSamplerUniformPayload, SHADER_MAX_SHADOW_MAP_LIGHT_COUNT> ObjectRenderer::GetShadowSamplerUniformPayloads(const BatchInput& input)
{
    std::array<ShadowSamplerUniformPayload, SHADER_MAX_SHADOW_MAP_LIGHT_COUNT> payloads;

    uint32_t totalShadowCasterCount{0};

    uint32_t numPointShadowCasters{0};
    uint32_t numSpotlightShadowCasters{0};
    uint32_t numDirectionalShadowCasters{0};

    for (const auto& lightStateIt : input.pGroup->GetLights().GetAll())
    {
        const auto& lightState = lightStateIt.second;

        if (!lightState.light.castsShadows)
        {
            continue;
        }

        if (totalShadowCasterCount >= SHADER_MAX_SHADOW_MAP_LIGHT_COUNT)
        {
            m_pGlobal->pLogger->Warning("ObjectRenderer::GetShadowSamplerUniformPayloads: Reached max light count, ignoring the rest");
            break;
        }

        uint32_t arrayIndex{0};

        switch (lightState.light.type)
        {
            case LightType::Point:
            {
                arrayIndex = numPointShadowCasters++;

                if (arrayIndex >= SHADER_MAX_SHADOW_MAP_POINT_COUNT)
                {
                    m_pGlobal->pLogger->Warning("ObjectRenderer::GetShadowSamplerUniformPayloads: Reached max point light count, ignoring the rest");
                    continue;
                }
            }
            break;
            case LightType::Spotlight:
            {
                arrayIndex = numSpotlightShadowCasters++;

                if (arrayIndex >= SHADER_MAX_SHADOW_MAP_SPOTLIGHT_COUNT)
                {
                    m_pGlobal->pLogger->Warning("ObjectRenderer::GetShadowSamplerUniformPayloads: Reached max spot light count, ignoring the rest");
                    continue;
                }
            }
            break;
            case LightType::Directional:
            {
                arrayIndex = numDirectionalShadowCasters++;

                if (arrayIndex >= SHADER_MAX_SHADOW_MAP_DIRECTIONAL_COUNT)
                {
                    m_pGlobal->pLogger->Warning("ObjectRenderer::GetShadowSamplerUniformPayloads: Reached max directional count, ignoring the rest");
                    continue;
                }
            }
            break;
        }

        payloads.at(totalShadowCasterCount++) = ShadowSamplerUniformPayload{
            .lightId = lightState.light.id.id,
            .arrayIndex = arrayIndex
        };
    }

    return payloads;
}

inline LoadedTexture GetShadowMapMissingTexture(Global* pGlobal, LightType lightType)
{
    switch (lightType)
    {
        case LightType::Point: return pGlobal->pTextures->GetMissingTextureCube();
        case LightType::Spotlight: return pGlobal->pTextures->GetMissingTexture2D();
        case LightType::Directional: return pGlobal->pTextures->GetMissingTextureArray();
    }

    assert(false);
    return {};
}

void ObjectRenderer::BindShadowSamplers(const BatchInput& input, const std::array<ShadowSamplerUniformPayload, SHADER_MAX_SHADOW_MAP_LIGHT_COUNT>& shadowSamplerUniformPayloads)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    const auto shadowSamplerId = m_pGlobal->pSamplers->GetDefaultSampler(DefaultSampler::LinearClamp);

    const auto pointMissingImageId = GetShadowMapMissingTexture(m_pGlobal, LightType::Point).imageId;
    const auto spotlightMissingImageId = GetShadowMapMissingTexture(m_pGlobal, LightType::Spotlight).imageId;
    const auto directionalMissingImageId = GetShadowMapMissingTexture(m_pGlobal, LightType::Directional).imageId;

    // Correspond directly to sampler arrays in shaders. Filled by default with missing textures, and then used
    // sampler array slots are overwritten below ith actual shadow map textures to be bound.
    std::array<GPU::ImageId, SHADER_MAX_SHADOW_MAP_POINT_COUNT> pointShadowMapBinds;
    pointShadowMapBinds.fill(pointMissingImageId);

    std::array<GPU::ImageId, SHADER_MAX_SHADOW_MAP_SPOTLIGHT_COUNT> spotLightShadowMapBinds;
    spotLightShadowMapBinds.fill(spotlightMissingImageId);

    std::array<GPU::ImageId, SHADER_MAX_SHADOW_MAP_DIRECTIONAL_COUNT> directionalShadowMapBinds;
    directionalShadowMapBinds.fill(directionalMissingImageId);

    for (const auto& shadowSamplerUniformPayload : shadowSamplerUniformPayloads)
    {
        const LightId lightId(shadowSamplerUniformPayload.lightId);

        if (!lightId.IsValid())
        {
            continue;
        }

        const auto lightState = input.pGroup->GetLights().GetLightState(lightId);
        if (!lightState)
        {
            continue;
        }

        const auto shadowTexture = m_pGlobal->pTextures->GetTexture(*lightState->shadowMapTextureId);
        if (!shadowTexture)
        {
            continue;
        }

        switch (lightState->light.type)
        {
            case LightType::Point: pointShadowMapBinds.at(shadowSamplerUniformPayload.arrayIndex) = shadowTexture->imageId; break;
            case LightType::Spotlight: spotLightShadowMapBinds.at(shadowSamplerUniformPayload.arrayIndex) = shadowTexture->imageId; break;
            case LightType::Directional: directionalShadowMapBinds.at(shadowSamplerUniformPayload.arrayIndex) = shadowTexture->imageId; break;
        }
    }

    for (unsigned int x = 0; x < pointShadowMapBinds.size(); ++x)
    {
        m_pGlobal->pGPU->CmdBindImageViewSampler(
            renderPass,
            "i_shadowSampler_cube",
            x,
            pointShadowMapBinds.at(x),
            shadowSamplerId
        );
    }

    for (unsigned int x = 0; x < spotLightShadowMapBinds.size(); ++x)
    {
        m_pGlobal->pGPU->CmdBindImageViewSampler(
            renderPass,
            "i_shadowSampler_single",
            x,
            spotLightShadowMapBinds.at(x),
            shadowSamplerId
        );
    }

    for (unsigned int x = 0; x < directionalShadowMapBinds.size(); ++x)
    {
        m_pGlobal->pGPU->CmdBindImageViewSampler(
            renderPass,
            "i_shadowSampler_array",
            x,
            directionalShadowMapBinds.at(x),
            shadowSamplerId
        );
    }
}

}
