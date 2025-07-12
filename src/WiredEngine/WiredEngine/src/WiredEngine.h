/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WIREDENGINE_H
#define WIREDENGINE_WIREDENGINE_SRC_WIREDENGINE_H

#include "RunState.h"

#include <Wired/Engine/IWiredEngine.h>
#include <Wired/Engine/Render/EngineRenderTask.h>

#include <Wired/GPU/SurfaceDetails.h>
#include <Wired/Render/Id.h>
#include <Wired/Render/Task/RenderTask.h>

#include <Wired/Platform/IPlatform.h>

#include <memory>
#include <mutex>
#include <future>
#include <unordered_map>
#include <string>
#include <vector>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Engine
{
    class ISurfaceAccess;
    class EngineAccess;

    class WiredEngine : public IWiredEngine
    {
        public:

            WiredEngine(NCommon::ILogger* pLogger,
                        NCommon::IMetrics* pMetrics,
                        const std::optional<ISurfaceAccess*>& surfaceAccess,
                        Platform::IPlatform* pPlatform,
                        Render::IRenderer* pRenderer);
            ~WiredEngine() override;

            void Run(std::unique_ptr<Client> pClient) override;
            [[nodiscard]] std::optional<std::shared_ptr<NCommon::ImageData>> GetRenderOutput() const override;

        private:

            enum class InitState
            {
                Initializing,   // The engine is running one-time async init / data load work
                Finished        // The engine has finished the above Initializing work
            };

            struct InitOutput
            {
                std::unordered_map<std::string, std::vector<std::byte>> shaderAssets;
            };

        private:

            [[nodiscard]] bool StartUp(std::unique_ptr<Client> pClient);
            void RunLoop();
            void RunStep_Initializing();
            void RunStep();
            void SimulationStep();
            void PostSimulationStep();
            void ProcessEvents();
            void ProcessEvent(const Platform::Event& event);
            void ShutDown();

            void SetCanRender(bool canRender);

            [[nodiscard]] std::optional<GPU::ImGuiGlobals> InitImGui();
            void DestroyImGui();

            void InitializeAsync();

            [[nodiscard]] bool InitializeSync(const InitOutput& initOutput);
            [[nodiscard]] bool CreateDefaultRenderTargets();

            [[nodiscard]] std::optional<std::unique_ptr<GPU::SurfaceDetails>> CreateWindowSurface();
            void DestroyWindowSurface();
            void HandleRenderSurfaceInvalidatedError();
            void HandleRenderSurfaceLostError();

            void SyncAudioListener();

            void TryEnqueueFrameRender(const std::chrono::duration<double, std::milli>& maxWaitTimeMs);
            void EnqueueFrameRender();
            [[nodiscard]] std::optional<ImDrawData*> RenderImFrame();
            void SwitchToClient(std::unique_ptr<Client> client);
            void SetRenderSettings(const Render::RenderSettings& renderSettings);

            [[nodiscard]] std::vector<std::shared_ptr<EngineRenderTask>> GetDefaultRenderTasks() const;
            [[nodiscard]] std::vector<std::shared_ptr<Render::RenderTask>> ToRenderTasks(const std::vector<std::shared_ptr<EngineRenderTask>>& engineRenderTasks) const;

        private:

            //
            // Systems provided to us
            //
            NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;
            std::optional<ISurfaceAccess*> m_surfaceAccess;
            Platform::IPlatform* m_pPlatform;
            Render::IRenderer* m_pRenderer;

            //
            // Init/Execution State
            //
            InitState m_initState{InitState::Initializing};
            std::future<std::expected<InitOutput, bool>> m_initResultFuture;
            bool m_keepRunning{true};
            std::mutex m_canRenderMutex;
            bool m_canRender{false}; // Whether render commands can be issued

            //
            // Internal run state
            //
            std::unique_ptr<RunState> m_pRunState;
            std::unique_ptr<EngineAccess> m_pEngineAccess;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WIREDENGINE_H
