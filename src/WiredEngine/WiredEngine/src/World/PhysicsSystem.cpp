/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PhysicsSystem.h"
#include "PhysicsStateComponent.h"

#include "../RunState.h"

#include "../World/WorldState.h"

#include <Wired/Engine/Metrics.h>
#include <Wired/Engine/World/Components.h>

#include <NEON/Common/Timer.h>
#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Metrics/IMetrics.h>

namespace Wired::Engine
{

PhysicsSystem::PhysicsSystem(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, WorldState* pWorldState)
    : m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_pWorldState(pWorldState)
{

}

void PhysicsSystem::Initialize(entt::basic_registry<EntityId>& registry)
{
    registry.on_construct<TransformComponent>().connect<&PhysicsSystem::OnComponentTouched>(this);
    registry.on_update<TransformComponent>().connect<&PhysicsSystem::OnComponentTouched>(this);
    registry.on_destroy<TransformComponent>().connect<&PhysicsSystem::OnComponentTouched>(this);

    registry.on_construct<PhysicsComponent>().connect<&PhysicsSystem::OnComponentTouched>(this);
    registry.on_update<PhysicsComponent>().connect<&PhysicsSystem::OnComponentTouched>(this);
    registry.on_destroy<PhysicsComponent>().connect<&PhysicsSystem::OnComponentTouched>(this);

    registry.on_destroy<PhysicsStateComponent>().connect<&PhysicsSystem::OnPhysicsStateComponentDestroyed>(this);
}

void PhysicsSystem::Destroy(entt::basic_registry<EntityId>&)
{

}

void PhysicsSystem::Execute(RunState* pRunState, WorldState* pWorldState, entt::basic_registry<EntityId>& registry)
{
    NCommon::Timer physicsTimer(METRIC_PHYSICS_SIM_TIME);

    Pre_SimulationStep(pRunState, pWorldState, registry);
        pWorldState->GetPhysicsInternal()->SimulationStep(pRunState->simTimeStepMs);
    Post_SimulationStep(pRunState, pWorldState, registry);

    FetchContacts(pWorldState);

    physicsTimer.StopTimer(m_pMetrics);
}

void PhysicsSystem::OnComponentTouched(entt::basic_registry<EntityId>&, EntityId entity)
{
    // Ignore events while this very system is executing, as we don't want us syncing entities
    // to the latest physics system data to count as an entity being "invalidated"
    if (dynamic_cast<WorldState*>(m_pWorldState)->GetExecutingSystem() == GetType())
    {
        return;
    }

    m_invalidedEntities.insert(entity);
}

void PhysicsSystem::OnPhysicsStateComponentDestroyed(entt::basic_registry<EntityId>& registry, EntityId entity)
{
    const auto [physicsComponent, physicsStateComponent] = registry.get<PhysicsComponent, PhysicsStateComponent>(entity);

    m_toDeleteEntities.insert({entity, {physicsComponent.scene, physicsStateComponent.physicsId}});
}

static RigidBodyData RigidBodyDataFromEntity(const TransformComponent& transformComponent, const PhysicsComponent& physicsComponent)
{
    RigidBodyData rigidBodyData{};
    rigidBodyData.type = physicsComponent.bodyType;
    rigidBodyData.shape = physicsComponent.shape;
    rigidBodyData.scale = transformComponent.GetScale();
    rigidBodyData.position = transformComponent.GetPosition();
    rigidBodyData.orientation = transformComponent.GetOrientation();
    rigidBodyData.linearVelocity = physicsComponent.linearVelocity;
    rigidBodyData.mass = physicsComponent.mass;
    rigidBodyData.linearDamping = physicsComponent.linearDamping;
    rigidBodyData.angularDamping = physicsComponent.angularDamping;

    return rigidBodyData;
}

static void SyncEntityToPhysicsData(TransformComponent& transformComponent, PhysicsComponent& physicsComponent, const RigidBodyData& rigidBodyData)
{
    transformComponent.SetPosition(rigidBodyData.position);
    transformComponent.SetOrientation(rigidBodyData.orientation);
    physicsComponent.linearVelocity = rigidBodyData.linearVelocity;
}

void PhysicsSystem::Pre_SimulationStep(RunState* pRunState, WorldState* pWorldState, entt::basic_registry<EntityId>& registry)
{
    //
    // Process entities that were touched with regard to physics data since the last call to Execute
    //
    for (const auto& entity : m_invalidedEntities)
    {
        ProcessInvalidatedEntity(pRunState, registry, entity);
    }
    m_invalidedEntities.clear();

    //
    // Remove entities from the physics system as needed
    //
    for (const auto& toDeleteIt : m_toDeleteEntities)
    {
        pWorldState->GetPhysicsInternal()->DestroyRigidBody(toDeleteIt.second.first, toDeleteIt.second.second);
        m_physicsIdToEntityId.erase(toDeleteIt.second.second);
    }
    m_toDeleteEntities.clear();

    //
    // Add new physics entities to the physics system
    //
    for (const auto& toAddEntity : m_toAddEntities)
    {
        const auto [transformComponent, physicsComponent] = registry.get<TransformComponent, PhysicsComponent>(toAddEntity);

        const auto rigidBodyData = RigidBodyDataFromEntity(transformComponent, physicsComponent);

        const auto physicsId = pWorldState->GetPhysicsInternal()->CreateRigidBody(physicsComponent.scene, rigidBodyData);
        if (!physicsId)
        {
            m_pLogger->Error("PhysicsSystem::Pre_SimulationStep: Failed to create rigid body for entity: {}", (uint64_t)toAddEntity);
            continue;
        }

        registry.emplace<PhysicsStateComponent>(toAddEntity, PhysicsStateComponent{
            .physicsId = *physicsId
        });

        m_physicsIdToEntityId.insert({*physicsId, toAddEntity});
    }
    m_toAddEntities.clear();

    //
    // Update existing physics entities
    //
    for (const auto& toUpdateEntity : m_toUpdateEntities)
    {
        const auto [transformComponent, physicsComponent, physicsStateComponent] =
            registry.get<TransformComponent, PhysicsComponent, PhysicsStateComponent>(toUpdateEntity);

        const auto rigidBodyData = RigidBodyDataFromEntity(transformComponent, physicsComponent);

        pWorldState->GetPhysicsInternal()->UpdateRigidBody(physicsComponent.scene, physicsStateComponent.physicsId, rigidBodyData);
    }
    m_toUpdateEntities.clear();
}

void PhysicsSystem::ProcessInvalidatedEntity(RunState*, entt::basic_registry<EntityId>& registry, EntityId entity)
{
    if (!registry.valid(entity))
    {
        return;
    }

    if (m_toDeleteEntities.contains(entity))
    {
        return;
    }

    const bool hasPhysicsState = registry.all_of<PhysicsStateComponent>(entity);
    const bool isCompletePhysicsEntity = registry.all_of<TransformComponent, PhysicsComponent>(entity);

    //
    // If the entity has physics state but no longer has enough components attached to be a
    // complete physics entity, then erase its physics state.
    //
    if (hasPhysicsState && !isCompletePhysicsEntity)
    {
        // Note that this causes OnPhysicsStateComponentDestroyed to be called, which enqueues
        // the entity for removal from the physics system via m_toDeleteEntities
        registry.erase<PhysicsStateComponent>(entity);

        return;
    }
    //
    // Otherwise, process updated renderables
    //
    else if (hasPhysicsState && isCompletePhysicsEntity)
    {
        m_toUpdateEntities.insert(entity);
    }
    //
    // Otherwise, process newly completed physics entities
    //
    else if (!hasPhysicsState && isCompletePhysicsEntity)
    {
        m_toAddEntities.insert(entity);
    }
}

void PhysicsSystem::Post_SimulationStep(RunState*, WorldState* pWorldState, entt::basic_registry<EntityId>& registry)
{
    pWorldState->GetPhysicsInternal()->UpdateBodiesFromSimulation();

    for (auto&& [entity, transformComponent, physicsComponent, physicsStateComponent] :
        registry.view<TransformComponent, PhysicsComponent, PhysicsStateComponent>().each())
    {
        const auto rigidBody = pWorldState->GetPhysicsInternal()->GetRigidBody(physicsComponent.scene, physicsStateComponent.physicsId);
        if (!rigidBody)
        {
            m_pLogger->Error("PhysicsSystem::Post_SimulationStep: Entity with physics state has no physics system body: {}", (uint64_t)entity);
            continue;
        }

        if (!(*rigidBody)->isDirty)
        {
            continue;
        }

        SyncEntityToPhysicsData(transformComponent, physicsComponent, (*rigidBody)->data);

        registry.emplace_or_replace<TransformComponent>(entity, transformComponent);
    }

    pWorldState->GetPhysicsInternal()->MarkBodiesSynced();
}

void PhysicsSystem::FetchContacts(WorldState* pWorld)
{
    // Erase contacts from last time system was run
    m_entityContacts.clear();

    // Re-query physics scenes for current contacts
    const auto sceneNames = pWorld->GetPhysicsInternal()->GetAllSceneNames();

    for (const auto& sceneName : sceneNames)
    {
        const auto contacts = pWorld->GetPhysicsInternal()->PopContacts(sceneName);

        for (const auto& contact : contacts)
        {
            const auto entityId1 = m_physicsIdToEntityId.find(contact.body1);
            if (entityId1 == m_physicsIdToEntityId.cend()) { continue; }

            const auto entityId2 = m_physicsIdToEntityId.find(contact.body2);
            if (entityId2 == m_physicsIdToEntityId.cend()) { continue; }

            m_entityContacts.push_back(EntityContact{
                .entity1 = entityId1->second,
                .entity2 = entityId2->second,
                .details = contact.details
            });
        }
    }
}

const std::vector<EntityContact>& PhysicsSystem::GetEntityContacts()
{
    return m_entityContacts;
}

}
