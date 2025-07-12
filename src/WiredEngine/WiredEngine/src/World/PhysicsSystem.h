/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_PHYSICSSYSTEM_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_PHYSICSSYSTEM_H

#include "IWorldSystem.h"

#include "../InternalIds.h"

#include <Wired/Engine/Physics/PhysicsCommon.h>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Engine
{
    class PhysicsSystem : public IWorldSystem
    {
        public:

            PhysicsSystem(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, WorldState* pWorldState);

            [[nodiscard]] Type GetType() const noexcept override { return Type::Physics; }

            void Initialize(entt::basic_registry<EntityId>& registry) override;
            void Destroy(entt::basic_registry<EntityId>& registry) override;

            void Execute(RunState* pRunState, WorldState* pWorld, entt::basic_registry<EntityId>& registry) override;

            // Internal
            [[nodiscard]] const std::vector<EntityContact>& GetEntityContacts();

        private:

            void OnComponentTouched(entt::basic_registry<EntityId>& registry, EntityId entity);
            void OnPhysicsStateComponentDestroyed(entt::basic_registry<EntityId>& registry, EntityId entity);

            void Pre_SimulationStep(RunState* pRunState, WorldState* pWorldState, entt::basic_registry<EntityId>& registry);
            void ProcessInvalidatedEntity(RunState* pRunState, entt::basic_registry<EntityId>& registry, EntityId entity);

            void Post_SimulationStep(RunState* pRunState, WorldState* pWorldState, entt::basic_registry<EntityId>& registry);

            void FetchContacts(WorldState* pWorld);

        private:

            NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;
            WorldState* m_pWorldState;

            std::unordered_set<EntityId> m_invalidedEntities;

            std::unordered_set<EntityId> m_toAddEntities;
            std::unordered_set<EntityId> m_toUpdateEntities;
            std::unordered_map<EntityId, std::pair<PhysicsSceneName, PhysicsId>> m_toDeleteEntities;

            std::unordered_map<PhysicsId, EntityId> m_physicsIdToEntityId;

            std::vector<EntityContact> m_entityContacts;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_PHYSICSSYSTEM_H
