/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EditorClient.h"
#include "GridLogic.h"

#include "Window/ViewportWindow.h"

#include <Wired/Engine/IResources.h>
#include <Wired/Engine/EngineImGui.h>
#include <Wired/Engine/World/Camera2D.h>
#include <Wired/Engine/World/Components.h>
#include <Wired/Engine/Render/EngineRenderWorldTask.h>
#include <Wired/Engine/Render/EnginePresentToSwapChainTask.h>

#include <Wired/Platform/IKeyboardState.h>

#include <NEON/Common/Log/ILogger.h>

namespace Wired
{

static const auto EDITOR_PACKAGE_NAME = Engine::PackageName("EditorPackage");
static constexpr auto ASSET_VIEW_WORLD = "AssetView";

void EditorClient::OnClientStart(Engine::IEngineAccess* pEngine)
{
    Engine::Client::OnClientStart(pEngine);

    //
    // Init renderer
    //
    auto renderSettings = pEngine->GetRenderSettings();
    renderSettings.ambientLight = {1,1,1}; // Full bright ambient light
    renderSettings.shadowQuality = Render::ShadowQuality::Low;
    renderSettings.fxaa = false;
    renderSettings.resolution = {1920, 1080};
    pEngine->SetRenderSettings(renderSettings);

    //
    // Init client/engine
    //
    const auto assetViewColorId = engine->GetResources()->CreateTexture_RenderTarget(
        {Render::TextureUsageFlag::ColorTarget, Render::TextureUsageFlag::GraphicsSampled},
        "AssetViewColor"
    );
    if (!assetViewColorId)
    {
        engine->GetLogger()->Fatal("EditorClient::OnClientStart: Failed to create asset view color render target");
        engine->Quit();
        return;
    }

    const auto assetViewDepthId = engine->GetResources()->CreateTexture_RenderTarget(
        {Render::TextureUsageFlag::DepthStencilTarget},
        "AssetViewDepth"
    );
    if (!assetViewDepthId)
    {
        engine->GetLogger()->Fatal("EditorClient::OnClientStart: Failed to create asset view depth render target");
        engine->Quit();
        return;
    }

    //
    // Load editor-specific package
    //
    if (!engine->SpinWait(engine->GetPackages()->LoadPackageResources(EDITOR_PACKAGE_NAME)))
    {
        engine->GetLogger()->Fatal("EditorClient::OnClientStart: Failed to load EditorPackage resources");
        engine->Quit();
        return;
    }

    m_editorResources = std::make_unique<EditorResources>(
        engine,
        *engine->GetPackages()->GetLoadedPackageResources(EDITOR_PACKAGE_NAME),
        *assetViewColorId,
        *assetViewDepthId
    );

    //
    // ImGui/View init
    //
    Engine::EnsureImGui(engine); // Required if Wired was build as shared libs
    ImGui::GetIO().FontGlobalScale = 2.0f;
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();

    m_mainWindow = std::make_unique<MainWindow>(engine, m_editorResources.get());
}

struct alignas(16) GridDataUniformPayload
{
    alignas(8) glm::vec2 gridInterval{};
    alignas(4) float gridLineSize{};
    alignas(16) glm::vec3 gridLineColor{};
    alignas(4) float cameraScale{};
};

bool EditorClient::OnRecordImGuiCommands()
{
    //ImGui::ShowStyleEditor();
    //ImGui::ShowIDStackToolWindow();
    //bool s = true; ImGui::ShowDemoWindow(&s);

    (*m_mainWindow)();

    return true;
}

void EditorClient::OnSimulationStep(unsigned int timeStepMs)
{
    Client::OnSimulationStep(timeStepMs);

    MaintainGridLines2DEntity();
    MaintainAssetViewEntity();
}

std::optional<std::vector<std::shared_ptr<Engine::EngineRenderTask>>> EditorClient::GetRenderTasks() const
{
    std::vector<std::shared_ptr<Engine::EngineRenderTask>> renderTasks;

    // Render the default world into the engine default offscreen textures
    auto renderWorldTask = std::make_unique<Engine::EngineRenderWorldTask>();
    renderWorldTask->worldName = Engine::DEFAULT_WORLD_NAME;
    renderWorldTask->targetColorTextureIds = {engine->GetDefaultOffscreenColorTextureId()};
    renderWorldTask->clearColor = glm::vec3(0,0,0);
    renderWorldTask->targetDepthTextureId = engine->GetDefaultOffscreenDepthTextureId();
    renderWorldTask->worldCameraId = engine->GetDefaultWorld()->GetDefaultCamera3D()->GetId();
    renderWorldTask->spriteCameraId = engine->GetDefaultWorld()->GetDefaultCamera2D()->GetId();
    renderTasks.push_back(std::move(renderWorldTask));

    // If a model is selected in the assets view, then we also need to render the asset view
    // world to our custom asset view texture(s) to render the selected model for the asset view
    // window to display
    const auto selectedAsset = m_mainWindow->GetAssetsWindowVM()->GetSelectedAsset();
    if (selectedAsset && (selectedAsset->assetType == Engine::AssetType::Model))
    {
        const auto assetViewWorld = engine->GetWorld(ASSET_VIEW_WORLD);

        auto renderAssetViewNode = std::make_unique<Engine::EngineRenderWorldTask>();
        renderAssetViewNode->worldName = ASSET_VIEW_WORLD;
        renderAssetViewNode->targetColorTextureIds = {m_editorResources->GetAssetViewColorTextureId()};
        renderAssetViewNode->clearColor = glm::vec3(0, 0, 0);
        renderAssetViewNode->targetDepthTextureId = m_editorResources->GetAssetViewDepthTextureId();
        renderAssetViewNode->worldCameraId = assetViewWorld->GetDefaultCamera3D()->GetId();
        renderAssetViewNode->spriteCameraId = assetViewWorld->GetDefaultCamera2D()->GetId();
        renderTasks.push_back(std::move(renderAssetViewNode));
    }

    auto presentTask = std::make_unique<Engine::EnginePresentToSwapChainTask>();
    presentTask->clearColor = glm::vec3(0,0,0);
    renderTasks.push_back(std::move(presentTask));

    return renderTasks;
}

void EditorClient::MaintainGridLines2DEntity()
{
    // If the viewport is using a 2D camera, create a grid lines entity to draw
    // 2D grid lines in the viewport render, otherwise destroy the entity if it exists
    const auto viewportCamera = m_mainWindow->GetVM()->GetViewportCamera();
    if (viewportCamera)
    {
        if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_2D)
        {
            CreateOrUpdateGridLines2DEntity(dynamic_cast<Engine::Camera2D*>(*viewportCamera));
        }
        else
        {
            DestroyGridLines2DEntity();
        }
    }
}

void EditorClient::CreateOrUpdateGridLines2DEntity(Engine::Camera2D const* pViewportCamera)
{
    (void)pViewportCamera;
    /*if (!m_gridLines2DEntity)
    {
        m_gridLines2DEntity = engine->GetDefaultWorld()->CreateEntity();
    }

    const auto gridVertShaderIt = std::ranges::find_if(m_editorResources->GetEditorPackageResources().graphicsShaders, [](const auto& it){
        return it.first.starts_with("grid.vert");
    });
    const auto gridFragShaderIt = std::ranges::find_if(m_editorResources->GetEditorPackageResources().graphicsShaders, [](const auto& it){
        return it.first.starts_with("grid.frag");
    });

    if (gridVertShaderIt == m_editorResources->GetEditorPackageResources().graphicsShaders.cend()) { return; }
    if (gridFragShaderIt == m_editorResources->GetEditorPackageResources().graphicsShaders.cend()) { return; }

    const auto renderResolution = engine->GetRenderSettings().renderResolution;
    const auto virtualResolution = engine->GetVirtualResolution();

    // Ratios to draw grid lines if render and virtual resolutions don't match
    const float wRatio = (float)renderResolution.w / (float)virtualResolution.w;
    const float hRatio = (float)renderResolution.h / (float)virtualResolution.h;

    const float gridInterval = CalculateGridInterval(pViewportCamera->GetScale());
    const float gridLineSize = CalculateGridLineThickness(pViewportCamera->GetScale());

    GridDataPayload gridDataPayload{
        .gridInterval = {gridInterval * wRatio, gridInterval * hRatio},
        .gridLineSize = gridLineSize,
        .gridLineColor = {1,1,1},
        .cameraScale = pViewportCamera->GetScale()
    };
    std::vector<std::byte> gridDataPayloadBytes(sizeof(GridDataPayload));
    memcpy(gridDataPayloadBytes.data(), &gridDataPayload, sizeof(GridDataPayload));

    Engine::UniformDataBind gridDataUniformBind{};
    gridDataUniformBind.shaderStages = {GPU::ShaderType::Fragment};
    gridDataUniformBind.data = gridDataPayloadBytes;
    gridDataUniformBind.userTag = "GridDataPayload";

    Engine::CustomRenderableComponent customComponent{};
    customComponent.meshId = engine->GetResources()->GetSpriteMeshId();
    customComponent.vertexShaderId = gridVertShaderIt->second;
    customComponent.fragmentShaderId = gridFragShaderIt->second;
    customComponent.screenViewProjectionUniformBind = {GPU::ShaderType::Fragment};
    customComponent.uniformDataBinds = { gridDataUniformBind };
    Engine::AddOrUpdateComponent(engine->GetDefaultWorld(), *m_gridLines2DEntity, customComponent);*/
}

void EditorClient::DestroyGridLines2DEntity()
{
    if (m_gridLines2DEntity)
    {
        engine->GetDefaultWorld()->DestroyEntity(*m_gridLines2DEntity);
        m_gridLines2DEntity = std::nullopt;
    }
}

void EditorClient::MaintainAssetViewEntity()
{
    //
    // If there's a model asset selected in the assets window, create an asset view entity
    // that represents the model, otherwise destroy the entity if it exists
    //
    const auto selectedAsset = m_mainWindow->GetAssetsWindowVM()->GetSelectedAsset();
    if (selectedAsset && (selectedAsset->assetType == Engine::AssetType::Model))
    {
        CreateOrUpdateAssetViewEntity(selectedAsset->assetName);
    }
    else
    {
        DestroyAssetViewEntity();
    }
}

void EditorClient::CreateOrUpdateAssetViewEntity(const std::string& modelAssetName)
{
    const auto packageResources = m_mainWindow->GetVM()->GetPackageResources();
    if (!packageResources) { return; }

    const auto modelId = packageResources->models.find(modelAssetName);
    if (modelId == packageResources->models.cend())
    {
        engine->GetLogger()->Error("EditorClient::CreateOrUpdateAssetViewEntity: Package model doesnt exist: {}", modelAssetName);
        return;
    }

    const auto assetViewWorld = engine->GetWorld(ASSET_VIEW_WORLD);

    //
    // Create the entity if it doesn't exist
    //
    if (!m_assetViewEntity)
    {
        m_assetViewEntity = assetViewWorld->CreateEntity();
    }

    //
    // Transform
    //
    auto transformComponent = Engine::GetComponent<Engine::TransformComponent>(assetViewWorld, *m_assetViewEntity);
    if (!transformComponent)
    {
        transformComponent = Engine::TransformComponent{};
    }
    transformComponent->SetPosition({0, 0, -5});
    Engine::AddOrUpdateComponent(assetViewWorld, *m_assetViewEntity, *transformComponent);

    //
    // Model
    //
    auto modelComponent = Engine::GetComponent<Engine::ModelRenderableComponent>(assetViewWorld, *m_assetViewEntity);
    if (!modelComponent)
    {
        modelComponent = Engine::ModelRenderableComponent{};
    }
    modelComponent->modelId = modelId->second;

    const auto selectedAnimationName = m_mainWindow->GetAssetsWindowVM()->GetSelectedModelAnimationName();
    if (selectedAnimationName)
    {
        if (!modelComponent->animationState || (modelComponent->animationState->animationName != *selectedAnimationName))
        {
            modelComponent->animationState = Engine::ModelAnimationState(Engine::ModelAnimationType::Looping, *selectedAnimationName);
        }
    }
    else
    {
        modelComponent->animationState = std::nullopt;
    }
    Engine::AddOrUpdateComponent(assetViewWorld, *m_assetViewEntity, *modelComponent);
}

void EditorClient::DestroyAssetViewEntity()
{
    if (m_assetViewEntity)
    {
        engine->GetWorld(ASSET_VIEW_WORLD)->DestroyEntity(*m_assetViewEntity);
        m_assetViewEntity = std::nullopt;
    }
}

}
