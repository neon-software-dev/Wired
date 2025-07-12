/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RunState.h"
#include "Resources.h"
#include "Packages.h"
#include "WorkThreadPool.h"

#include "Physics/JoltPhysics.h"

#include "Audio/AudioManager.h"

#include "World/WorldState.h"

#include <Wired/Engine/Client.h>

#include <Wired/Platform/IPlatform.h>

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Engine
{

RunState::RunState(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, Render::IRenderer* pRenderer, Platform::IPlatform* pPlatform)
    : pWorkThreadPool(std::make_unique<WorkThreadPool>(std::thread::hardware_concurrency()))
    , pAudioManager(std::make_unique<AudioManager>(pLogger, pMetrics))
    , pResources(std::make_unique<Resources>(pLogger, pPlatform, pAudioManager.get(), pRenderer))
    , pPackages(std::make_unique<Packages>(pLogger, pWorkThreadPool.get(), pResources.get(), pPlatform, pRenderer))
    , m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_pRenderer(pRenderer)
{

}

RunState::~RunState()
{
    pWorkThreadPool = nullptr;
    pAudioManager = nullptr;
    pResources = nullptr;
    pPackages = nullptr;
    pClient = nullptr;
    m_pLogger = nullptr;
    m_pMetrics = nullptr;
    m_pRenderer = nullptr;
}

bool RunState::StartUp()
{
    if (!pAudioManager->Startup())
    {
        m_pLogger->Error("RunState::StartUp: Failed to start audio manager");
        return false;
    }

    JoltPhysics::StaticInit();

    return true;
}

void RunState::ShutDown()
{
    pWorkThreadPool = nullptr;

    for (auto& world : worlds)
    {
        world.second->Destroy();
    }
    worlds.clear();
    pPackages->ShutDown();
    pResources->ShutDown();
    pAudioManager->Shutdown();

    JoltPhysics::StaticDestroy();
}

WorldState* RunState::GetWorld(const std::string& worldName)
{
    const auto it = worlds.find(worldName);
    if (it != worlds.cend())
    {
        return it->second.get();
    }

    auto world = std::make_unique<WorldState>(worldName, m_pLogger, m_pMetrics, pAudioManager.get(), pResources.get(), pPackages.get(), m_pRenderer);
    const bool result = world->StartUp();
    assert(result); (void)result;

    worlds.insert({worldName, std::move(world)});

    return worlds.at(worldName).get();
}

void RunState::PumpFinishedWork() const
{
    pWorkThreadPool->PumpFinished();
}

}
