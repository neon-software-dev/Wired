/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_ENGINEACCESS_H
#define WIREDENGINE_WIREDENGINE_SRC_ENGINEACCESS_H

#include <Wired/Engine/IEngineAccess.h>

#include <NEON/Common/Log/ILogger.h>

#include <memory>
#include <unordered_map>
#include <vector>
#include <optional>

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Platform
{
    class IPlatform;
}

namespace Wired::Engine
{
    class RunState;

    class EngineAccess : public IEngineAccess
    {
        public:

            EngineAccess(NCommon::ILogger* pLogger,
                         NCommon::IMetrics* pMetrics,
                         Platform::IPlatform* pPlatform,
                         Render::IRenderer* pRenderer,
                         RunState* pRunState,
                         const std::optional<GPU::ImGuiGlobals>& imGuiGlobals);
            ~EngineAccess() override;

            //
            // IEngineAccess
            //
            [[nodiscard]] NCommon::ILogger* GetLogger() const override { return m_pLogger; }
            [[nodiscard]] NCommon::IMetrics* GetMetrics() const override { return m_pMetrics; }
            [[nodiscard]] IResources* GetResources() const override;
            [[nodiscard]] IPackages* GetPackages() const override;
            void SwitchToClient(std::unique_ptr<Client> client) override;
            [[nodiscard]] IWorldState* GetDefaultWorld() override;
            [[nodiscard]] IWorldState* GetWorld(const std::string& worldName) override;
            [[nodiscard]] unsigned int GetSimulationTimeStepMs() const override;
            [[nodiscard]] std::uintmax_t GetSimStepIndex() const override;
            [[nodiscard]] Platform::IKeyboardState* GetKeyboardState() const override;
            [[nodiscard]] NCommon::Size2DUInt GetVirtualResolution() const override;
            void SetVirtualResolution(const NCommon::Size2DUInt& resolution) override;
            [[nodiscard]] VirtualSpaceSize RenderSizeToVirtualSize(const NCommon::Size2DReal& renderSize) const override;
            [[nodiscard]] Render::RenderSettings GetRenderSettings() const override;
            void SetRenderSettings(const Render::RenderSettings& renderSettings) override;
            void SyncAudioListenerToCamera(const std::optional<CameraAudioListener>& cameraAudioListener) override;
            void SetAudioListener(const std::optional<AudioListener>& audioListener) override;
            [[nodiscard]] Render::TextureId GetDefaultOffscreenColorTextureId() const override;
            [[nodiscard]] Render::TextureId GetDefaultOffscreenDepthTextureId() const override;
            [[nodiscard]] bool IsImGuiAvailable() const override;
            [[nodiscard]] std::optional<GPU::ImGuiGlobals> GetImGuiGlobals() const override { return m_imGuiGlobals; }
            #ifdef WIRED_IMGUI
                [[nodiscard]] std::optional<ImTextureID> CreateImGuiTextureReference(Render::TextureId textureId, Render::DefaultSampler sampler) override;
            #endif
            void SetMouseCapture(bool doCaptureMouse) const override;
            [[nodiscard]] bool IsMouseCaptured() const override;
            void PumpFinishedWork() override;
            void Quit() override;

            //
            // Internal
            //
            [[nodiscard]] std::optional<std::unique_ptr<Client>> PopSwitchToClientMsg();
            [[nodiscard]] std::optional<bool> PopQuitMsg();
            [[nodiscard]] std::optional<Render::RenderSettings> PopSetRenderSettingsMsg();

            [[nodiscard]] std::optional<CameraAudioListener> GetCameraSyncedAudioListener() const;
            [[nodiscard]] std::optional<AudioListener> GetAudioListener() const;

        private:

            NCommon::ILogger* m_pLogger{nullptr};
            NCommon::IMetrics* m_pMetrics{nullptr};
            Platform::IPlatform* m_pPlatform{nullptr};
            Render::IRenderer* m_pRenderer{nullptr};
            RunState* m_pRunState{nullptr};
            std::optional<GPU::ImGuiGlobals> m_imGuiGlobals;

            std::optional<CameraAudioListener> m_cameraSyncedAudioListener;
            std::optional<AudioListener> m_audioListener;

            // Signals received from the scene
            std::optional<std::unique_ptr<Client>> m_switchToClientMsg;
            std::optional<bool> m_quitMsg;
            std::optional<Render::RenderSettings> m_setRenderSettingsMsg;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_ENGINEACCESS_H
