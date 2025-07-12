/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_AUDIOSYSTEM_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_AUDIOSYSTEM_H

#include "IWorldSystem.h"
#include "AudioStateComponent.h"

#include <Wired/Engine/Audio/AudioListener.h>
#include <Wired/Engine/World/TransformComponent.h>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Engine
{
    class AudioManager;

    class AudioSystem : public IWorldSystem
    {
        public:

            AudioSystem(NCommon::ILogger* pLogger, AudioManager* pAudioManager);
            ~AudioSystem() override;

            [[nodiscard]] Type GetType() const noexcept override { return Type::Audio; };

            void Initialize(entt::basic_registry<EntityId>& registry) override;
            void Destroy(entt::basic_registry<EntityId>& registry) override;

            void Execute(RunState* pRunState,
                         WorldState* pWorld,
                         entt::basic_registry<EntityId>& registry) override;

        private:

            void UpdateSourceProperties(AudioStateComponent& audioStateComponent,
                                        const TransformComponent& transformComponent) const;

            void ProcessFinishedAudio(entt::basic_registry<EntityId>& registry,
                                      const EntityId& entity,
                                      AudioStateComponent& audioStateComponent);

        private:

            NCommon::ILogger* m_pLogger;
            AudioManager* m_pAudioManager;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_AUDIOSYSTEM_H
