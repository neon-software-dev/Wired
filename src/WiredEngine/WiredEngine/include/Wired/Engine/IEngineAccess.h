/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IENGINEACCESS_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IENGINEACCESS_H

#include "EngineCommon.h"

#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/Audio/AudioListener.h>

#include <Wired/Render/RenderSettings.h>
#include <Wired/Render/Id.h>
#include <Wired/Render/SamplerCommon.h>

#include <Wired/GPU/ImGuiGlobals.h>

#include <NEON/Common/Space/Size2D.h>

#ifdef WIRED_IMGUI
    #include <imgui.h>
#endif

#include <string>
#include <optional>
#include <future>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Platform
{
    class IKeyboardState;
}

namespace Wired::Engine
{
    class IWorldState;
    class IResources;
    class IPackages;
    class Client;

    /**
     * Main interface through which client Scenes can access/manipulate engine state
     */
    class IEngineAccess
    {
        public:

            virtual ~IEngineAccess() = default;

            [[nodiscard]] virtual NCommon::ILogger* GetLogger() const = 0;
            [[nodiscard]] virtual NCommon::IMetrics* GetMetrics() const = 0;
            [[nodiscard]] virtual IResources* GetResources() const = 0;
            [[nodiscard]] virtual IPackages* GetPackages() const = 0;

            virtual void SwitchToClient(std::unique_ptr<Client> client) = 0;

            [[nodiscard]] virtual IWorldState* GetDefaultWorld() = 0;
            [[nodiscard]] virtual IWorldState* GetWorld(const std::string& worldName) = 0;

            [[nodiscard]] virtual unsigned int GetSimulationTimeStepMs() const = 0;
            [[nodiscard]] virtual std::uintmax_t GetSimStepIndex() const = 0;
            [[nodiscard]] virtual Platform::IKeyboardState* GetKeyboardState() const = 0;

            [[nodiscard]] virtual NCommon::Size2DUInt GetVirtualResolution() const = 0;
            virtual void SetVirtualResolution(const NCommon::Size2DUInt& resolution) = 0;
            [[nodiscard]] virtual VirtualSpaceSize RenderSizeToVirtualSize(const NCommon::Size2DReal& renderSize) const = 0;

            [[nodiscard]] virtual Render::RenderSettings GetRenderSettings() const = 0;
            virtual void SetRenderSettings(const Render::RenderSettings& renderSettings) = 0;

            virtual void SyncAudioListenerToCamera(const std::optional<CameraAudioListener>& cameraAudioListener) = 0;
            virtual void SetAudioListener(const std::optional<AudioListener>& audioListener) = 0;

            [[nodiscard]] virtual Render::TextureId GetDefaultOffscreenColorTextureId() const = 0;
            [[nodiscard]] virtual Render::TextureId GetDefaultOffscreenDepthTextureId() const = 0;

            [[nodiscard]] virtual bool IsImGuiAvailable() const = 0;
            [[nodiscard]] virtual GPU::ImGuiGlobals GetImGuiGlobals() const = 0;

            #ifdef WIRED_IMGUI
                [[nodiscard]] virtual std::optional<ImTextureID> CreateImGuiTextureReference(Render::TextureId textureId, Render::DefaultSampler sampler) = 0;
            #endif

            virtual void SetMouseCapture(bool doCaptureMouse) const = 0;
            [[nodiscard]] virtual bool IsMouseCaptured() const = 0;

            virtual void PumpFinishedWork() = 0;
            virtual void Quit() = 0;

            template <typename T>
            [[nodiscard]] T SpinWait(std::future<T> fut, const std::chrono::milliseconds& interval = std::chrono::milliseconds(10));
    };

    template <typename T>
    [[nodiscard]] T IEngineAccess::SpinWait(std::future<T> fut, const std::chrono::milliseconds& interval)
    {
        while (fut.wait_for(interval) != std::future_status::ready) { PumpFinishedWork(); }
        return fut.get();
    }
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IENGINEACCESS_H
