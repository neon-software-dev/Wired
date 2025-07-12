/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/World/Components.h>

#include "WorldState.h"

#include <NEON/Common/SharedLib.h>

namespace Wired::Engine
{

template <typename T>
void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const T& component)
{
    dynamic_cast<WorldState*>(pWorldState)->AddOrUpdateComponent(entityId, component);
}

template <typename T>
void RemoveComponent(IWorldState* pWorldState, EntityId entityId)
{
    dynamic_cast<WorldState*>(pWorldState)->RemoveComponent<T>(entityId);
}

template <typename T>
std::optional<T> GetComponent(IWorldState* pWorldState, EntityId entityId)
{
    return dynamic_cast<WorldState*>(pWorldState)->GetComponent<T>(entityId);
}

template NEON_PUBLIC void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const TransformComponent& component);
template NEON_PUBLIC void RemoveComponent<TransformComponent>(IWorldState* pWorldState, EntityId entityId);
template NEON_PUBLIC std::optional<TransformComponent> GetComponent(IWorldState* pWorldState, EntityId entityId);

template NEON_PUBLIC void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const SpriteRenderableComponent& component);
template NEON_PUBLIC void RemoveComponent<SpriteRenderableComponent>(IWorldState* pWorldState, EntityId entityId);
template NEON_PUBLIC std::optional<SpriteRenderableComponent> GetComponent(IWorldState* pWorldState, EntityId entityId);

template NEON_PUBLIC void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const MeshRenderableComponent& component);
template NEON_PUBLIC void RemoveComponent<MeshRenderableComponent>(IWorldState* pWorldState, EntityId entityId);
template NEON_PUBLIC std::optional<MeshRenderableComponent> GetComponent(IWorldState* pWorldState, EntityId entityId);

template NEON_PUBLIC void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const ModelRenderableComponent& component);
template NEON_PUBLIC void RemoveComponent<ModelRenderableComponent>(IWorldState* pWorldState, EntityId entityId);
template NEON_PUBLIC std::optional<ModelRenderableComponent> GetComponent(IWorldState* pWorldState, EntityId entityId);

template NEON_PUBLIC void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const CustomRenderableComponent& component);
template NEON_PUBLIC void RemoveComponent<CustomRenderableComponent>(IWorldState* pWorldState, EntityId entityId);
template NEON_PUBLIC std::optional<CustomRenderableComponent> GetComponent(IWorldState* pWorldState, EntityId entityId);

template NEON_PUBLIC void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const LightComponent& component);
template NEON_PUBLIC void RemoveComponent<LightComponent>(IWorldState* pWorldState, EntityId entityId);
template NEON_PUBLIC std::optional<LightComponent> GetComponent(IWorldState* pWorldState, EntityId entityId);

template NEON_PUBLIC void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const PhysicsComponent& component);
template NEON_PUBLIC void RemoveComponent<PhysicsComponent>(IWorldState* pWorldState, EntityId entityId);
template NEON_PUBLIC std::optional<PhysicsComponent> GetComponent(IWorldState* pWorldState, EntityId entityId);

}
