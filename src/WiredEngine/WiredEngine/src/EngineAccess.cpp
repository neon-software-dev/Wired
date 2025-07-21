/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EngineAccess.h"
#include "RunState.h"
#include "Resources.h"
#include "Packages.h"

#include "World/WorldState.h"

#include <Wired/Engine/Client.h>
#include <Wired/Engine/SpaceUtil.h>
#include <Wired/Engine/World/WorldCommon.h>

#include <Wired/Render/IRenderer.h>

#include <Wired/Platform/IPlatform.h>
#include <Wired/Platform/IEvents.h>

#include <NEON/Common/Space/SpaceUtil.h>

namespace Wired::Engine
{

EngineAccess::EngineAccess(NCommon::ILogger* pLogger,
                           NCommon::IMetrics* pMetrics,
                           Platform::IPlatform* pPlatform,
                           Render::IRenderer* pRenderer,
                           RunState* pRunState,
                           const std::optional<GPU::ImGuiGlobals>& imGuiGlobals)
    : m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_pPlatform(pPlatform)
    , m_pRenderer(pRenderer)
    , m_pRunState(pRunState)
    , m_imGuiGlobals(imGuiGlobals)
{

}

EngineAccess::~EngineAccess()
{
    m_pLogger = nullptr;
    m_pPlatform = nullptr;
    m_pRenderer = nullptr;
    m_pRunState = nullptr;
    m_imGuiGlobals = std::nullopt;
}

IResources* EngineAccess::GetResources() const
{
    return m_pRunState->pResources.get();
}

IPackages* EngineAccess::GetPackages() const
{
    return m_pRunState->pPackages.get();
}

void EngineAccess::SwitchToClient(std::unique_ptr<Client> client)
{
    m_switchToClientMsg = std::move(client);
}

IWorldState* EngineAccess::GetDefaultWorld()
{
    return GetWorld(DEFAULT_WORLD_NAME);
}

IWorldState* EngineAccess::GetWorld(const std::string& worldName)
{
    return m_pRunState->GetWorld(worldName);
}

unsigned int EngineAccess::GetSimulationTimeStepMs() const
{
    return m_pRunState->simTimeStepMs;
}

std::uintmax_t EngineAccess::GetSimStepIndex() const
{
    return m_pRunState->simStepIndex;
}

Platform::IKeyboardState* EngineAccess::GetKeyboardState() const
{
    return m_pPlatform->GetEvents()->GetKeyboardState();
}

NCommon::Size2DUInt EngineAccess::GetVirtualResolution() const
{
    return m_pRunState->virtualResolution;
}

void EngineAccess::SetVirtualResolution(const NCommon::Size2DUInt& resolution)
{
    m_pRunState->virtualResolution = resolution;
}

VirtualSpaceSize EngineAccess::RenderSizeToVirtualSize(const NCommon::Size2DReal& renderSize) const
{
    const auto virtualSurface = NCommon::Surface(m_pRunState->virtualResolution);
    const auto renderSurface = NCommon::Surface(m_pRenderer->GetRenderSettings().resolution);

    return NCommon::MapSizeBetweenSurfaces<NCommon::Size2DReal, VirtualSpaceSize>(
        renderSize,
        virtualSurface,
        renderSurface
    );
}

Render::RenderSettings EngineAccess::GetRenderSettings() const
{
    return m_pRenderer->GetRenderSettings();
}

void EngineAccess::SetRenderSettings(const Render::RenderSettings& renderSettings)
{
    m_setRenderSettingsMsg = renderSettings;
}

void EngineAccess::SyncAudioListenerToCamera(const std::optional<CameraAudioListener>& cameraAudioListener)
{
    m_cameraSyncedAudioListener = cameraAudioListener;
}

void EngineAccess::SetAudioListener(const std::optional<AudioListener>& audioListener)
{
    m_audioListener = audioListener;
}

Render::TextureId EngineAccess::GetDefaultOffscreenColorTextureId() const
{
    return m_pRunState->offscreenColorTextureId;
}

Render::TextureId EngineAccess::GetDefaultOffscreenDepthTextureId() const
{
    return m_pRunState->offscreenDepthTextureId;
}

bool EngineAccess::IsImGuiAvailable() const
{
    return m_pRunState->imGuiActive && m_imGuiGlobals;
}

#ifdef WIRED_IMGUI
std::optional<ImTextureID> EngineAccess::CreateImGuiTextureReference(Render::TextureId textureId, Render::DefaultSampler sampler)
{
    return m_pRenderer->CreateImGuiTextureReference(textureId, sampler);
}
#endif

void EngineAccess::SetMouseCapture(bool doCaptureMouse) const
{
    m_pPlatform->GetWindow()->SetMouseCapture(doCaptureMouse);
}

bool EngineAccess::IsMouseCaptured() const
{
    return m_pPlatform->GetWindow()->IsCapturingMouse();
}

void EngineAccess::PumpFinishedWork()
{
    m_pRunState->PumpFinishedWork();
}

void EngineAccess::Quit()
{
    LogInfo("EngineAccess: Received quit message");
    m_quitMsg = true;
}

std::optional<std::unique_ptr<Client>> EngineAccess::PopSwitchToClientMsg()
{
    auto msg = std::move(m_switchToClientMsg);
    m_switchToClientMsg = {};
    return msg;
}

std::optional<bool> EngineAccess::PopQuitMsg()
{
    auto msg = m_quitMsg;
    m_quitMsg = {};
    return msg;
}

std::optional<Render::RenderSettings> EngineAccess::PopSetRenderSettingsMsg()
{
    auto msg = m_setRenderSettingsMsg;
    m_setRenderSettingsMsg = {};
    return msg;
}

std::optional<CameraAudioListener> EngineAccess::GetCameraSyncedAudioListener() const
{
    return m_cameraSyncedAudioListener;
}

std::optional<AudioListener> EngineAccess::GetAudioListener() const
{
    return m_audioListener;
}

}
