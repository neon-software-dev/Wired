/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_IWORLDSYSTEM_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_IWORLDSYSTEM_H

#include <Wired/Engine/World/WorldCommon.h>

#include <entt/entt.hpp>

#include <string>

namespace Wired::Engine
{
    class RunState;
    class WorldState;

    class IWorldSystem
    {
        public:

            enum class Type
            {
                ModelAnimator,
                ImGui,
                Physics,
                Audio
            };

        public:

            virtual ~IWorldSystem() = default;

            [[nodiscard]] virtual Type GetType() const noexcept = 0;

            virtual void Initialize(entt::basic_registry<EntityId>& registry) { (void)registry; };
            virtual void Destroy(entt::basic_registry<EntityId>& registry) { (void)registry; };

            virtual void Execute(RunState* pRunState,
                                 WorldState* pWorld,
                                 entt::basic_registry<EntityId>& registry) = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_IWORLDSYSTEM_H
