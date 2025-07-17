/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AudioSystem.h"

#include "../Audio/AudioManager.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Engine
{

AudioSystem::AudioSystem(NCommon::ILogger* pLogger, AudioManager* pAudioManager)
    : m_pLogger(pLogger)
    , m_pAudioManager(pAudioManager)
{

}

AudioSystem::~AudioSystem()
{
    m_pLogger = nullptr;
    m_pAudioManager = nullptr;
}

void AudioSystem::Initialize(entt::basic_registry<EntityId>&)
{

}

void AudioSystem::Execute(RunState*, WorldState*, entt::basic_registry<EntityId>& registry)
{
    //
    // Update the audio properties of any entity with both an audio component and a
    // transform component, so the audio source is attached to the entities' position
    // in the world.
    //
    for (auto&& [entity, audioStateComponent, transformComponent] :
        registry.view<AudioStateComponent, TransformComponent>().each())
    {
        UpdateSourceProperties(audioStateComponent, transformComponent);
    }

    //
    // For all entities with an audio component, destroy any static audio sources which have finished
    // playing. (However, for streamed sources, we keep those around, even if they're temporarily "finished").
    //
    for (auto&& [entity, audioStateComponent] :
        registry.view<AudioStateComponent>().each())
    {
        ProcessFinishedAudio(registry, (EntityId)entity, audioStateComponent);
    }

    //
    // Clean up any finished transient audio sources
    //
    m_pAudioManager->DestroyFinishedTransientSources();

    //
    // Clean up played buffers for streamed audio sources
    //
    m_pAudioManager->DestroyFinishedStreamedData();
}

void AudioSystem::UpdateSourceProperties(AudioStateComponent& audioStateComponent, const TransformComponent& transformComponent) const
{
    for (auto& activeSource : audioStateComponent.activeSources)
    {
        (void)m_pAudioManager->UpdateLocalSourcePosition(activeSource, transformComponent.GetPosition());
    }
}

void AudioSystem::ProcessFinishedAudio(entt::basic_registry<EntityId>& registry, const EntityId& entity, AudioStateComponent& audioStateComponent)
{
    std::unordered_set<AudioSourceId> finishedSources;

    //
    // Look for any static (non-streamed) audio sources associated with the entity
    //
    for (auto& sourceId : audioStateComponent.activeSources)
    {
        const auto sourceDataType = m_pAudioManager->GetSourceDataType(sourceId);

        if (!sourceDataType || *sourceDataType != SourceDataType::Static)
        {
            continue;
        }

        const auto sourceState = m_pAudioManager->GetSourceState(sourceId);
        if (!sourceState || (sourceState->playState == PlayState::Stopped))
        {
            finishedSources.insert(sourceId);
            continue;
        }
    }

    //
    // Remove the finished audio sources from the entity's audio component
    //
    for (const auto& finishedSource : finishedSources)
    {
        LogInfo("AudioSystem: Detected finished audio {} for entity {}", finishedSource, (uint64_t)entity);
        audioStateComponent.activeSources.erase(finishedSource);
    }

    //
    // If the audio component is no longer tracking any audio, destroy it
    //
    if (audioStateComponent.activeSources.empty())
    {
        registry.erase<AudioStateComponent>(entity);
    }
}

}
