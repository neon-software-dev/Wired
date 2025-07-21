/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WiredEngine.h"
#include "EngineAccess.h"
#include "Resources.h"
#include "Packages.h"
#include "WorkThreadPool.h"

#include "World/WorldState.h"
#include "World/RendererSyncer.h"

#include "Audio/AudioManager.h"

#include <Wired/Engine/ISurfaceAccess.h>
#include <Wired/Engine/Metrics.h>
#include <Wired/Engine/SpaceUtil.h>
#include <Wired/Engine/World/Camera2D.h>
#include <Wired/Engine/World/Camera3D.h>
#include <Wired/Engine/Render/EngineRenderWorldTask.h>
#include <Wired/Engine/Render/EnginePresentToSwapChainTask.h>

#include <Wired/Platform/IPlatform.h>
#include <Wired/Platform/ShaderUtil.h>

#include <Wired/Render/IRenderer.h>
#include <Wired/Render/RenderCommon.h>
#include <Wired/Render/Task/RenderGroupTask.h>
#include <Wired/Render/Task/PresentToSwapChainTask.h>

#include <NEON/Common/Space/SpaceUtil.h>
#include <NEON/Common/Space/Blit.h>
#include <NEON/Common/Timer.h>
#include <NEON/Common/Metrics/IMetrics.h>
#include <NEON/Common/Log/ILogger.h>

#ifdef WIRED_IMGUI
    #include <implot.h>
#endif

namespace Wired::Engine
{

WiredEngine::WiredEngine(NCommon::ILogger* pLogger,
                         NCommon::IMetrics* pMetrics,
                         const std::optional<ISurfaceAccess*>& surfaceAccess,
                         Platform::IPlatform* pPlatform,
                         Render::IRenderer* pRenderer)
    : m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_surfaceAccess(surfaceAccess)
    , m_pPlatform(pPlatform)
    , m_pRenderer(pRenderer)
{

}

WiredEngine::~WiredEngine()
{
    m_pLogger = nullptr;
    m_pMetrics = nullptr;
    m_surfaceAccess = std::nullopt;
    m_pPlatform = nullptr;
    m_pRenderer = nullptr;
}

void WiredEngine::Run(std::unique_ptr<Client> pClient)
{
    if (!StartUp(std::move(pClient))) { return; }
    RunLoop();
    ShutDown();
}

GPU::ImGuiGlobals GetImGuiGlobals()
{
    GPU::ImGuiGlobals globals{};

    #ifdef WIRED_IMGUI
        globals.pImGuiContext = ImGui::GetCurrentContext();

        ImGuiMemAllocFunc pTempAllocFunc{nullptr};
        ImGuiMemFreeFunc pTempAllocFreeFunc{nullptr};
        void* pUserData{nullptr};
        ImGui::GetAllocatorFunctions(&pTempAllocFunc, &pTempAllocFreeFunc, &pUserData);

        globals.pImGuiMemAllocFunc = pTempAllocFunc;
        globals.pImGuiMemFreeFunc = pTempAllocFreeFunc;

        globals.pImPlotContext = ImPlot::GetCurrentContext();
    #endif

    return globals;
}

bool WiredEngine::StartUp(std::unique_ptr<Client> pClient)
{
    LogInfo("WiredEngine: Starting Up");

    // Create a surface for rendering to, if applicable
    const auto surfaceDetails = CreateWindowSurface();

    // Init ImGui, if built with ImGui support
    const std::optional<GPU::ImGuiGlobals> imGuiGlobals = InitImGui();

    // Init renderer
    Render::RenderSettings renderSettings{};

    if (!m_pRenderer->StartUp(surfaceDetails, m_pPlatform->GetWindow()->GetShaderBinaryType(), imGuiGlobals, renderSettings))
    {
        LogFatal("WiredEngine::StartUp: Failed to start the Renderer");
        return false;
    }

    // Init engine state/access
    m_pRunState = std::make_unique<RunState>(m_pLogger, m_pMetrics, m_pRenderer, m_pPlatform);
    m_pRunState->pClient = std::move(pClient);
    m_pRunState->imGuiActive = imGuiGlobals.has_value();
    if (!m_pRunState->StartUp())
    {
        LogFatal("WiredEngine::StartUp: Failed to start up run state");
        return false;
    }

    m_pEngineAccess = std::make_unique<EngineAccess>(m_pLogger, m_pMetrics, m_pPlatform, m_pRenderer, m_pRunState.get(), imGuiGlobals);

    // Extra events system init (must be done after renderer startup)
    m_pPlatform->GetEvents()->Initialize(imGuiGlobals);
    m_pPlatform->GetEvents()->RegisterCanRenderCallback([this](bool canRender){
        SetCanRender(canRender);
    });

    // Kick off one-time async initialize work
    InitializeAsync();

    return true;
}

void WiredEngine::ShutDown()
{
    LogInfo("WiredEngine: Shutting Down");

    // Unregister our events system canRender event callback
    m_pPlatform->GetEvents()->RegisterCanRenderCallback(std::nullopt);

    m_pEngineAccess = nullptr;

    m_pRunState->ShutDown();
    m_pRunState = nullptr;

    m_pRenderer->ShutDown();

    DestroyImGui();

    DestroyWindowSurface();
}

// May be called from multiple threads
void WiredEngine::SetCanRender(bool canRender)
{
    std::lock_guard<std::mutex> lock(m_canRenderMutex);
    m_canRender = canRender;
}

std::optional<GPU::ImGuiGlobals> WiredEngine::InitImGui()
{
#ifdef WIRED_IMGUI
    if (!m_surfaceAccess)
    {
        m_pLogger->Info("WiredEngine: In headless mode, not enabling ImGui");
        return std::nullopt;
    }

    //
    // Init ImGui
    //
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    // Configure ImGui
    ImGui::GetIO().ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard |
        ImGuiConfigFlags_NavEnableGamepad |
        ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    //
    // Fetch globals from the inited ImGui
    //
    const auto imGuiGlobals = GetImGuiGlobals();

    //
    // Initialize the surface for ImGui support
    //
    if (!(*m_surfaceAccess)->InitImGuiForSurface(imGuiGlobals))
    {
        m_pLogger->Error("WiredEngine::InitImGui: Failed to init ImGui for surface");
        return std::nullopt;
    }

    return imGuiGlobals;
#else
    return std::nullopt;
#endif
}

void WiredEngine::DestroyImGui()
{
#ifdef WIRED_IMGUI
    if (m_surfaceAccess)
    {
        (*m_surfaceAccess)->DestroyImGuiForSurface();
    }

    ImPlot::DestroyContext();
    ImGui::DestroyContext();
#endif
}

void WiredEngine::InitializeAsync()
{
    m_initResultFuture = std::async(std::launch::async, [this]() -> std::expected<InitOutput, bool> {
        //std::this_thread::sleep_for(std::chrono::seconds(3));

        InitOutput initOutput{};

        //
        // Find/read required engine shader assets
        //
        const auto engineShaders = m_pPlatform->GetFiles()->GetEngineShaderContentsBlocking(m_pPlatform->GetWindow()->GetShaderBinaryType());
        if (!engineShaders)
        {
            LogFatal("WiredEngine::InitializeAsync: Failed to get required engine shader asset contents");
            return std::unexpected(false);
        }

        initOutput.shaderAssets = *engineShaders;

        //
        // Open/read file package sources
        //
        m_pRunState->pPackages->OpenFilePackageSourcesBlocking();

        return initOutput;
    });
}

bool WiredEngine::InitializeSync(const InitOutput& initOutput)
{
    //
    // Load required/default renderer shaders
    //
    for (const auto& shaderAssetsIt : initOutput.shaderAssets)
    {
        const auto shaderType = Platform::GetShaderTypeFromAssetName(shaderAssetsIt.first);
        if (!shaderType)
        {
            LogFatal("WiredEngine::InitializeSync: Failed to determine shader type: {}", shaderAssetsIt.first);
            return false;
        }

        const GPU::ShaderSpec shaderSpec{
            .shaderName = shaderAssetsIt.first,
            .shaderType = *shaderType,
            .binaryType = m_pPlatform->GetWindow()->GetShaderBinaryType(),
            .shaderBinary = shaderAssetsIt.second
        };

        if (!m_pRenderer->CreateShader(shaderSpec).get())
        {
            LogFatal("WiredEngine::InitializeSync: Renderer failed to load graphics shader: {}", shaderAssetsIt.first);
            return false;
        }
    }

    //
    // Create default offscreen render target
    //
    if (!CreateDefaultRenderTargets())
    {
        LogFatal("WiredEngine::InitializeSync: Failed to crate default render targets");
        return false;
    }

    return true;
}

bool WiredEngine::CreateDefaultRenderTargets()
{
    //
    // Create the default offscreen target color texture
    //
    if (m_pRunState->offscreenColorTextureId.IsValid())
    {
        m_pRenderer->DestroyTexture(m_pRunState->offscreenColorTextureId);
    }

    const auto offscreenColorTextureId = m_pRenderer->CreateTexture_RenderTarget(
        {Render::TextureUsageFlag::ColorTarget, Render::TextureUsageFlag::ComputeSampled},
        "OffscreenColor"
    ).get();
    if (!offscreenColorTextureId)
    {
        LogError("WiredEngine::InitializeSync: Failed to create default offscreen color texture");
        return false;
    }

    m_pRunState->offscreenColorTextureId = *offscreenColorTextureId;

    //
    // Create the default offscreen target depth texture
    //
    if (m_pRunState->offscreenDepthTextureId.IsValid())
    {
        m_pRenderer->DestroyTexture(m_pRunState->offscreenDepthTextureId);
    }

    const auto offscreenDepthTextureId = m_pRenderer->CreateTexture_RenderTarget(
        {Render::TextureUsageFlag::DepthStencilTarget},
        "OffscreenDepth"
    ).get();
    if (!offscreenDepthTextureId)
    {
        LogError("WiredEngine::InitializeSync: Failed to create default offscreen depth texture");
        return false;
    }

    m_pRunState->offscreenDepthTextureId = *offscreenDepthTextureId;

    return true;
}

void WiredEngine::RunLoop()
{
    LogInfo("WiredEngine: RunLoop entered");

    while (m_keepRunning)
    {
        switch (m_initState)
        {
            case InitState::Initializing:   RunStep_Initializing(); break;
            case InitState::Finished:       RunStep(); break;
        }
    }

    if (m_initState == InitState::Finished)
    {
        m_pRunState->pClient->OnClientStop();
    }

    LogInfo("WiredEngine: RunLoop finished");
}

void WiredEngine::RunStep_Initializing()
{
    //
    // Pump events
    //
    ProcessEvents();

    //
    // Until we're initialized, if there's a swap chain, do empty
    // frame renders with no content, which just clear the screen
    //
    TryEnqueueFrameRender(std::chrono::milliseconds(m_pRunState->simTimeStepMs));

    //
    // If async initialization has finished, run the subsequent sync initialization
    // logic and then transition to the initialization finished state.
    //
    if (m_initResultFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
    {
        // Get the async init result and check that it was successful
        const auto asyncInitResult = m_initResultFuture.get();
        if (!asyncInitResult)
        {
            LogFatal("WiredEngine::RunStep_Initializing: Initialize async failed");
            m_keepRunning = false;
            return;
        }

        // Do post-async init work (e.g. loading engine default shaders into the renderer)
        if (!InitializeSync(*asyncInitResult))
        {
            LogFatal("WiredEngine::RunStep_Initializing: Initialize sync failed");
            m_keepRunning = false;
            return;
        }

        // Transition to initialization finished state and tell the client its starting
        LogInfo("WiredEngine: Initialization finished, transitioning to finished state");
        m_initState = InitState::Finished;
        m_pRunState->pClient->OnClientStart(m_pEngineAccess.get());
    }
}

void WiredEngine::RunStep()
{
    //
    // Accumulate time spent by the last run step
    //
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> lastFrameTimeMs = currentTime - m_pRunState->lastTimeSync;

    auto producedTimeMs = lastFrameTimeMs;

    // If the last run step took too long, put a max limit on how many simulation steps we should take
    // below to catch up to real time, so we prevent a death spiral
    if (producedTimeMs.count() > m_pRunState->maxProducedTimePerRunStepMs)
    {
        LogWarning("Simulation falling behind!");
        producedTimeMs = std::chrono::duration<double, std::milli>(m_pRunState->maxProducedTimePerRunStepMs);
    }

    m_pRunState->lastTimeSync = currentTime;
    m_pRunState->accumulatedTimeMs += producedTimeMs.count();

    //
    // Enqueue another frame render if possible
    //
    {
        // The amount of time remaining until we will have accumulated another sim step worth of time (can be negative)
        const auto remainingSimStepTime = (double)m_pRunState->simTimeStepMs - m_pRunState->accumulatedTimeMs;

        // If we've already accumulated a sim step worth of time we don't want to spend any time waiting
        // for the renderer to be free; if it's not immediately available just continue on to sim steps
        const auto waitTimeMs = std::max(remainingSimStepTime, 0.0);

        TryEnqueueFrameRender(std::chrono::duration<double, std::milli>(waitTimeMs));
    }

    //
    // Consume accumulated time by advancing the simulation forward in discrete time steps
    //
    while (m_pRunState->accumulatedTimeMs >= m_pRunState->simTimeStepMs)
    {
        SimulationStep();
        PostSimulationStep();

        if (!m_keepRunning)
        {
            return;
        }

        m_pRunState->simStepIndex++;
        m_pRunState->simStepTimeMs += m_pRunState->simTimeStepMs;
        m_pRunState->accumulatedTimeMs -= m_pRunState->simTimeStepMs;
    }
}

void WiredEngine::SimulationStep()
{
    NCommon::Timer simStepTimer(METRIC_SIM_STEP_TIME);

    // Process system events
    ProcessEvents();

    // Execute scene simulation step work
    m_pRunState->pClient->OnSimulationStep(m_pRunState->simTimeStepMs);

    // Update the audio manager's listening position as needed
    SyncAudioListener();

    // Execute internal simulation step work
    for (const auto& worldIt : m_pRunState->worlds)
    {
        worldIt.second->ExecuteSystems(m_pRunState.get());
    }

    // Pump the work thread to fulfill any finished tasks
    m_pRunState->pWorkThreadPool->PumpFinished();

    simStepTimer.StopTimer(m_pMetrics);
}

void WiredEngine::PostSimulationStep()
{
    //
    // Response to messages set by the client
    //

    // Client quit message
    if (m_pEngineAccess->PopQuitMsg() == true)
    {
        m_keepRunning = false;
        return;
    }

    // Set render settings message
    const auto setRenderSettingsMsg = m_pEngineAccess->PopSetRenderSettingsMsg();
    if (setRenderSettingsMsg)
    {
        SetRenderSettings(*setRenderSettingsMsg);
    }

    // Switch to client message. Intentionally doing the switch as the last step so
    // that the existing client gets callbacks about any other messages they had
    // sent before requesting a client switch
    auto switchToClientMsg = m_pEngineAccess->PopSwitchToClientMsg();
    if (switchToClientMsg)
    {
        SwitchToClient(std::move(*switchToClientMsg));
    }
}

void WiredEngine::SwitchToClient(std::unique_ptr<Client> client)
{
    m_pRunState->pClient->OnClientStop();

    m_pRunState->pClient = std::move(client);

    m_pRunState->pClient->OnClientStart(m_pEngineAccess.get());
}

void WiredEngine::SetRenderSettings(const Render::RenderSettings& renderSettings)
{
    // Update the renderer
    (void)m_pRenderer->RenderSettingsChanged(renderSettings).get();

    // Update internal state which depends on render settings
    if (!CreateDefaultRenderTargets())
    {
        m_pLogger->Error("WiredEngine::SetRenderSettings: Failed to create new default render targets");
    }

    m_pRunState->pClient->OnRenderSettingsChanged(renderSettings);
}

void WiredEngine::ProcessEvents()
{
    auto events = m_pPlatform->GetEvents()->PopEvents();
    while (!events.empty())
    {
        ProcessEvent(events.front());
        events.pop();
    }
}

void WiredEngine::ProcessEvent(const Platform::Event& event)
{
    if (std::holds_alternative<Platform::EventQuit>(event))
    {
        LogInfo("WiredEngine: Received quit event");
        SetCanRender(false);
        m_keepRunning = false;
    }
    else if (std::holds_alternative<Platform::EventWindowHidden>(event))
    {
        LogInfo("WiredEngine: Disabling rendering due to hidden window");
        SetCanRender(false);
    }
    else if (std::holds_alternative<Platform::EventWindowShown>(event))
    {
        LogInfo("WiredEngine: Enabling rendering due to visible window");
        SetCanRender(true);
    }
    else if (std::holds_alternative<Platform::KeyEvent>(event))
    {
        // Don't send the scene key events if we're not initialized yet
        if (m_initState != InitState::Finished) { return; }

        m_pRunState->pClient->OnKeyEvent(std::get<Platform::KeyEvent>(event));
    }
    else if (std::holds_alternative<Platform::MouseButtonEvent>(event))
    {
        // Don't send the scene mouse button events if we're not initialized yet
        if (m_initState != InitState::Finished) { return; }

        auto mouseButtonEvent = std::get<Platform::MouseButtonEvent>(event);

        // Check that the event was within the render area, and, is so, rewrite
        // the event to have virtual positions instead of screen positions
        const auto blitRects = NCommon::CalculateBlitRects(
            m_pRenderer->GetRenderSettings().presentBlitType,
            NCommon::Size2DReal::CastFrom(m_pRenderer->GetRenderSettings().resolution),
            NCommon::Size2DReal::CastFrom(*m_pPlatform->GetWindow()->GetWindowPixelSize())
        );

        const auto renderSurface = NCommon::Surface(m_pRenderer->GetRenderSettings().resolution);

        const auto renderSurfacePoint = ScreenSurfacePointToRenderSurfacePoint(
            ScreenSurfacePoint(mouseButtonEvent.xPos, mouseButtonEvent.yPos),
            blitRects.second,
            blitRects.first
        );

        if (renderSurfacePoint)
        {
            const auto renderSpacePoint = NCommon::MapSurfacePointToPointSpaceCenterOrigin<NCommon::Surface, NCommon::Point2DReal, NCommon::Point3DReal>(
                *renderSurfacePoint, renderSurface);

            const auto virtualSurface = NCommon::Surface(m_pRunState->virtualResolution);

            const auto virtualSpacePoint =
                NCommon::Map3DPointBetweenSurfaces<NCommon::Point3DReal, NCommon::Point3DReal>(renderSpacePoint, renderSurface, virtualSurface);

            mouseButtonEvent.xPos = virtualSpacePoint.x;
            mouseButtonEvent.yPos = virtualSpacePoint.y;

            m_pRunState->pClient->OnMouseButtonEvent(mouseButtonEvent);
        }
    }
    else if (std::holds_alternative<Platform::MouseMoveEvent>(event))
    {
        // Don't send the scene mouse movement events if we're not initialized yet
        if (m_initState != InitState::Finished) { return; }

        auto mouseMoveEvent = std::get<Platform::MouseMoveEvent>(event);

        // Convert the screen surface space coordinates to virtual point space coordinates
        const auto blitRects = NCommon::CalculateBlitRects(
            m_pRenderer->GetRenderSettings().presentBlitType,
            NCommon::Size2DReal::CastFrom(m_pRenderer->GetRenderSettings().resolution),
            NCommon::Size2DReal::CastFrom(*m_pPlatform->GetWindow()->GetWindowPixelSize())
        );

        const auto renderSurface = NCommon::Surface(m_pRenderer->GetRenderSettings().resolution);

        const auto renderSurfacePoint = ScreenSurfacePointToRenderSurfacePoint(
            ScreenSurfacePoint(*mouseMoveEvent.xPos, *mouseMoveEvent.yPos),
            blitRects.second,
            blitRects.first
        );

        // If the mouse was moved over the render surface, convert its position to virtual surface space
        if (renderSurfacePoint)
        {
            const auto renderSpacePoint = NCommon::MapSurfacePointToPointSpaceCenterOrigin<NCommon::Surface, NCommon::Point2DReal, NCommon::Point3DReal>(
                *renderSurfacePoint, renderSurface);

            const auto virtualSurface = NCommon::Surface(m_pRunState->virtualResolution);

            auto virtualSpacePoint =
                NCommon::Map3DPointBetweenSurfaces<NCommon::Point3DReal, NCommon::Point3DReal>(renderSpacePoint, renderSurface, virtualSurface);

            mouseMoveEvent.xPos = virtualSpacePoint.x;
            mouseMoveEvent.yPos = virtualSpacePoint.y;
        }
        // Otherwise, the mouse moved over the window, but not the renderable area, so null out the virtual
        // xPos and yPos
        else
        {
            mouseMoveEvent.xPos = std::nullopt;
            mouseMoveEvent.yPos = std::nullopt;
        }

        m_pRunState->pClient->OnMouseMoveEvent(mouseMoveEvent);
    }
}

void WiredEngine::TryEnqueueFrameRender(const std::chrono::duration<double, std::milli>& maxWaitTimeMs)
{
    std::lock_guard<std::mutex> lock(m_canRenderMutex);
    if (m_canRender)
    {
        // Wait for previous frame render to be processed
        if (m_pRunState->enqueueFrameRenderFuture.valid() &&
            m_pRunState->enqueueFrameRenderFuture.wait_for(maxWaitTimeMs) != std::future_status::ready)
        {
            return;
        }

        // Handle any error resulting from the previous frame render
        if (m_pRunState->enqueueFrameRenderFuture.valid())
        {
            const auto previousRenderResult = m_pRunState->enqueueFrameRenderFuture.get();
            if (!previousRenderResult)
            {
                switch (previousRenderResult.error())
                {
                    case GPU::SurfaceError::SurfaceInvalidated: HandleRenderSurfaceInvalidatedError(); break;
                    case GPU::SurfaceError::SurfaceLost: HandleRenderSurfaceLostError();
                    default: { /* no-op */ }
                }
            }
        }

        // Enqueue the next frame render
        EnqueueFrameRender();
    }
}

std::vector<std::shared_ptr<EngineRenderTask>> WiredEngine::GetDefaultRenderTasks() const
{
    std::vector<std::shared_ptr<EngineRenderTask>> tasks;

    //
    // If the engine is still initializing, do blank present tasks, which will just
    // clear the screen and do nothing else. If in headless mode, do no tasks.
    //
    if (m_initState == InitState::Initializing)
    {
        if (m_surfaceAccess)
        {
            auto presentTask = std::make_shared<EnginePresentToSwapChainTask>();
            presentTask->clearColor = {0.0f, 0.0f, 0.0f};
            tasks.push_back(presentTask);
        }

        return tasks;
    }

    auto renderDefaultWorldTask = std::make_shared<EngineRenderWorldTask>();
    renderDefaultWorldTask->worldName = DEFAULT_WORLD_NAME;
    renderDefaultWorldTask->targetColorTextureIds = {m_pRunState->offscreenColorTextureId};
    renderDefaultWorldTask->clearColor = {0.0f, 0.0f, 0.0f};
    renderDefaultWorldTask->targetDepthTextureId = {m_pRunState->offscreenDepthTextureId};

    tasks.push_back(renderDefaultWorldTask);

    if (m_surfaceAccess)
    {
        auto presentTask = std::make_shared<EnginePresentToSwapChainTask>();
        presentTask->presentTextureId = m_pRunState->offscreenColorTextureId;
        presentTask->clearColor = {0.0f, 0.0f, 0.0f};

        tasks.push_back(presentTask);
    }

    return tasks;
}

std::pair<Camera2D*, Camera3D*> GetRenderCameras(IWorldState* pWorldState, EngineRenderWorldTask* pRenderWorldTask)
{
    Camera2D* pSpriteCamera = nullptr;
    if (pRenderWorldTask->spriteCameraId.IsValid())
    {
        const auto camera = pWorldState->GetCamera2D(pRenderWorldTask->spriteCameraId);
        if (camera) { pSpriteCamera = *camera; }
    }
    if (pSpriteCamera == nullptr)
    {
        pSpriteCamera = pWorldState->GetDefaultCamera2D();
    }

    Camera3D* pWorldCamera = nullptr;
    if (pRenderWorldTask->worldCameraId.IsValid())
    {
        const auto camera = pWorldState->GetCamera3D(pRenderWorldTask->worldCameraId);
        if (camera) { pWorldCamera = *camera; }
    }
    if (pWorldCamera == nullptr)
    {
        pWorldCamera = pWorldState->GetDefaultCamera3D();
    }

    return {pSpriteCamera, pWorldCamera};
}

std::vector<std::shared_ptr<Render::RenderTask>> WiredEngine::ToRenderTasks(const std::vector<std::shared_ptr<EngineRenderTask>>& engineRenderTasks) const
{
    std::vector<std::shared_ptr<Render::RenderTask>> renderTasks;

    const auto virtualSurface = NCommon::Surface(m_pRunState->virtualResolution);
    const auto renderSurface = NCommon::Surface(m_pRenderer->GetRenderSettings().resolution);

    for (const auto& engineRenderTask : engineRenderTasks)
    {
        switch (engineRenderTask->GetType())
        {
            case EngineRenderTask::Type::RenderWorld:
            {
                const auto engineRenderWorldTask = std::dynamic_pointer_cast<EngineRenderWorldTask>(engineRenderTask);

                auto pWorld = m_pRunState->GetWorld(engineRenderWorldTask->worldName);

                auto renderCameras = GetRenderCameras(pWorld, engineRenderWorldTask.get());
                if (renderCameras.first == nullptr || renderCameras.second == nullptr)
                {
                    m_pLogger->Error("WiredEngine::ToRenderTasks: Failed to determine render cameras");
                    continue;
                }

                Render::Camera worldRenderCamera{};
                worldRenderCamera.position = renderCameras.second->GetPosition();
                worldRenderCamera.lookUnit = renderCameras.second->GetLookUnit();
                worldRenderCamera.upUnit = renderCameras.second->GetUpUnit();
                worldRenderCamera.rightUnit = renderCameras.second->GetRightUnit();
                worldRenderCamera.fovYDegrees = renderCameras.second->GetFovYDegrees();
                worldRenderCamera.aspectRatio = (float)renderSurface.size.w / (float)renderSurface.size.h;

                Render::Camera spriteRenderCamera{};
                spriteRenderCamera.position = renderCameras.first->GetPosition();
                spriteRenderCamera.lookUnit = renderCameras.first->GetLookUnit();
                spriteRenderCamera.upUnit = renderCameras.first->GetUpUnit();
                spriteRenderCamera.rightUnit = renderCameras.first->GetRightUnit();
                spriteRenderCamera.scale = renderCameras.first->GetScale();

                // Transform sprite camera position from virtual-space to render-space
                spriteRenderCamera.position = Render::ToGLM(
                    NCommon::Map3DPointBetweenSurfaces<VirtualSpacePoint, NCommon::Point3DReal>(
                        VirtualSpacePoint(spriteRenderCamera.position.x, spriteRenderCamera.position.y, spriteRenderCamera.position.z),
                        virtualSurface,
                        renderSurface
                    ));

                auto renderGroupTask = std::make_shared<Render::RenderGroupTask>();
                renderGroupTask->groupName = engineRenderWorldTask->worldName;
                renderGroupTask->targetColorTextureIds = engineRenderWorldTask->targetColorTextureIds;
                renderGroupTask->clearColor = engineRenderWorldTask->clearColor;
                renderGroupTask->targetDepthTextureId = engineRenderWorldTask->targetDepthTextureId;
                renderGroupTask->worldCamera = worldRenderCamera;
                renderGroupTask->spriteCamera = spriteRenderCamera;
                renderGroupTask->skyBoxTextureId = pWorld->GetSkyBoxTextureId();
                renderGroupTask->skyBoxTransform = pWorld->GetSkyBoxTransform();

                renderTasks.push_back(renderGroupTask);
            }
            break;
            case EngineRenderTask::Type::PresentToSwapChain:
            {
                const auto enginePresentTask = std::dynamic_pointer_cast<EnginePresentToSwapChainTask>(engineRenderTask);

                auto renderPresentTask = std::make_shared<Render::PresentToSwapChainTask>();
                renderPresentTask->presentTextureId = enginePresentTask->presentTextureId;
                renderPresentTask->clearColor = enginePresentTask->clearColor;

                renderTasks.push_back(renderPresentTask);
            }
            break;
        }
    }

    return renderTasks;
}

void WiredEngine::EnqueueFrameRender()
{
    NCommon::Timer enqueueFrameRenderTimer(METRIC_RENDER_FRAME_TIME);

    Render::RenderFrameParams renderFrameParams{};
    renderFrameParams.renderTasks = ToRenderTasks(GetDefaultRenderTasks());

    // If we're init finished, we can ask the client for ImGui to draw or
    // custom render tasks to run. (Otherwise, we run default render tasks
    // and no ImGui output)
    if (m_initState == InitState::Finished)
    {
        renderFrameParams.imDrawData = RenderImFrame();

        const auto clientRenderTasks =  m_pRunState->pClient->GetRenderTasks();
        if (clientRenderTasks)
        {
            renderFrameParams.renderTasks = ToRenderTasks(*clientRenderTasks);
        }
    }

    for (const auto& worldIt : m_pRunState->worlds)
    {
        auto stateUpdate = worldIt.second->CompileRenderStateUpdate(m_pRunState.get());
        if (stateUpdate.IsEmpty()) { continue; }

        renderFrameParams.stateUpdates.push_back(std::move(stateUpdate));
    }
    m_pMetrics->SetCounterValue(METRIC_RENDER_STATE_UPDATE_COUNT, renderFrameParams.stateUpdates.size());

    m_pRunState->enqueueFrameRenderFuture = m_pRenderer->RenderFrame(renderFrameParams);

    enqueueFrameRenderTimer.StopTimer(m_pMetrics);
}

std::optional<ImDrawData*> WiredEngine::RenderImFrame()
{
    // Require a surface to render ImGui
    if (!m_surfaceAccess)
    {
        return std::nullopt;
    }

    // Don't render ImGui until we've finished the init flow
    if (m_initState != InitState::Finished)
    {
        return std::nullopt;
    }

    #ifdef WIRED_IMGUI
        if (m_pRenderer->IsImGuiActive())
        {
            // Start an ImGui frame and have the client record commands into it
            (*m_surfaceAccess)->StartImGuiFrame();
            m_pRenderer->StartImGuiFrame();

            ImGui::NewFrame();
                const bool anyCommands = m_pRunState->pClient->OnRecordImGuiCommands();
            ImGui::EndFrame();

            // If the client had no ImGui commands, do nothing more
            if (!anyCommands)
            {
                return std::nullopt;
            }

            // Otherwise, render the ImGui draw commands and record the draw data for rendering
            ImGui::Render();
            return ImGui::GetDrawData();
        }
    #endif

    return std::nullopt;
}

std::optional<std::shared_ptr<NCommon::ImageData>> WiredEngine::GetRenderOutput() const
{
    if (m_initState != InitState::Finished) { return std::nullopt; }

    std::lock_guard<std::mutex> lock(m_pRunState->renderOutputMutex);
    return m_pRunState->renderOutput;
}

std::optional<std::unique_ptr<GPU::SurfaceDetails>> WiredEngine::CreateWindowSurface()
{
    if (!m_surfaceAccess) { return std::nullopt; }

    if (!(*m_surfaceAccess)->CreateSurface())
    {
        LogFatal("WiredEngine::StartUp: Failed to create a window surface");
        return std::nullopt;
    }

    return (*m_surfaceAccess)->GetSurfaceDetails();
}

void WiredEngine::DestroyWindowSurface()
{
    if (!m_surfaceAccess) { return; }

    (*m_surfaceAccess)->DestroySurface();
}

void WiredEngine::HandleRenderSurfaceInvalidatedError()
{
    if (!m_surfaceAccess) { return; }

    LogInfo("WiredEngine: Renderer notified the surface is invalidated");

    //
    // Update the renderer with the latest surface details
    //
    auto surfaceDetails = (*m_surfaceAccess)->GetSurfaceDetails();
    if (!surfaceDetails)
    {
        LogError("WiredEngine::HandleRenderSurfaceInvalidatedError: Failed to retrieve latest surface details");
        return;
    }

    (void)m_pRenderer->SurfaceDetailsChanged(std::move(*surfaceDetails)).get();
}

void WiredEngine::HandleRenderSurfaceLostError()
{
    if (!m_surfaceAccess) { return; }

    LogInfo("WiredEngine: Renderer notified the surface has been lost");

    //
    // Re-create the surface
    //
    (*m_surfaceAccess)->DestroySurface();

    if (!(*m_surfaceAccess)->CreateSurface())
    {
        LogError("WiredEngine::HandleRenderSurfaceLostError: Failed to create a new surface");
        return;
    }

    //
    // Update the renderer with the latest surface details
    //
    auto surfaceDetails = (*m_surfaceAccess)->GetSurfaceDetails();
    if (!surfaceDetails)
    {
        LogError("WiredEngine::HandleRenderSurfaceLostError: Failed to retrieve latest surface details");
        return;
    }

    (void)m_pRenderer->SurfaceDetailsChanged(std::move(*surfaceDetails)).get();
}

void WiredEngine::SyncAudioListener()
{
    // If the client has configured an explicit audio listener, sync the audio manager to it
    const auto explicitAudioListener = m_pEngineAccess->GetAudioListener();
    if (explicitAudioListener)
    {
        m_pRunState->pAudioManager->UpdateAudioListener(*explicitAudioListener);
        return;
    }

    // Otherwise, if the client has configured a camera-synced audio listener, then get the camera's
    // current properties and sync the audio manager to listening from the camera's perspective
    const auto cameraSyncedAudioListener = m_pEngineAccess->GetCameraSyncedAudioListener();
    if (cameraSyncedAudioListener)
    {
        const auto pWorld = m_pRunState->GetWorld(cameraSyncedAudioListener->worldName);
        if (pWorld)
        {
            const auto pCamera = pWorld->GetCamera3D(cameraSyncedAudioListener->cameraId);
            if (pCamera)
            {
                m_pRunState->pAudioManager->UpdateAudioListener(AudioListener{
                    .gain = cameraSyncedAudioListener->gain,
                    .worldPosition = (*pCamera)->GetPosition(),
                    .lookUnit = (*pCamera)->GetLookUnit(),
                    .upUnit = (*pCamera)->GetUpUnit()
                });
            }
        }
    }
}

}
