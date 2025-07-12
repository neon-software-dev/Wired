/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_RUNSTATE_H
#define WIREDENGINE_WIREDENGINE_SRC_RUNSTATE_H

#include <Wired/Render/Id.h>
#include <Wired/Render/IRenderer.h>

#include <NEON/Common/ImageData.h>
#include <NEON/Common/Space/Size2D.h>

#include <chrono>
#include <expected>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <future>

struct ImDrawData;

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Platform
{
    class IPlatform;
}

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Engine
{
    class Client;
    class IWorldState;
    class WorldState;
    class Resources;
    class Packages;
    class WorkThreadPool;
    class AudioManager;

    /**
     * Holds all run-specific state for a given run of the engine
     */
    class RunState
    {
        public:

            RunState(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, Render::IRenderer* pRenderer, Platform::IPlatform* pPlatform);
            ~RunState();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            [[nodiscard]] WorldState* GetWorld(const std::string& worldName);

            void PumpFinishedWork() const;

        public:

            //
            // Frame pacing state
            //
                // Timestep interval the engine simulation is stepped forward at
                const unsigned int simTimeStepMs = 10;
                // Maximum time that can be simulated/consumed per run step
                const unsigned int maxProducedTimePerRunStepMs = 50;

                // Zero-based simulation step index
                std::uintmax_t simStepIndex{0};
                // Runtime elapsed at the start of the current simulation step (stepIndex * simTimeStepMs)
                double simStepTimeMs{0.0};
                // Time point that accumulated time was last consumed at
                std::chrono::high_resolution_clock::time_point lastTimeSync{std::chrono::high_resolution_clock::now()};
                // Accumulated time to be consumed by simulation steps in simTimeStepMs-sized chunks
                double accumulatedTimeMs{0.0};

            //
            // Internal Systems
            //
                std::unique_ptr<WorkThreadPool> pWorkThreadPool;
                std::unique_ptr<AudioManager> pAudioManager;
                std::unique_ptr<Resources> pResources;
                std::unique_ptr<Packages> pPackages;

            //
            // Client/world state
            //
                Render::TextureId offscreenColorTextureId; // The default offscreen color target texture
                Render::TextureId offscreenDepthTextureId; // The default offscreen depth target texture

                std::unique_ptr<Client> pClient;
                NCommon::Size2DUInt virtualResolution{1920U, 1080U};

                std::unordered_map<std::string, std::unique_ptr<WorldState>> worlds;
                std::optional<ImDrawData*> imDrawData;

            //
            // Renderer
            //
                std::future<std::expected<bool, GPU::SurfaceError>> enqueueFrameRenderFuture;
                std::mutex renderOutputMutex;
                std::optional<std::shared_ptr<NCommon::ImageData>> renderOutput;

            //
            // ImGui
            //
                bool imGuiActive{false};

        private:

            NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;
            Render::IRenderer* m_pRenderer;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_RUNSTATE_H
