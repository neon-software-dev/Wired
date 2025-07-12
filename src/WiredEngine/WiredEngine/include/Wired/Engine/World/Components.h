/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_COMPONENTS_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_COMPONENTS_H

#include "IWorldState.h"
#include "WorldCommon.h"
#include "TransformComponent.h"
#include "SpriteRenderableComponent.h"
#include "MeshRenderableComponent.h"
#include "ModelRenderableComponent.h"
#include "CustomRenderableComponent.h"
#include "LightComponent.h"
#include "PhysicsComponent.h"

#include <optional>

namespace Wired::Engine
{
    /**
     * Adds the provided component to the specified entity, or updates the entity's component, if it
     * already had one of the same type.
     *
     * @tparam T The component type
     * @param pWorldState IWorldState instance to use. Must come from the engine.
     * @param entityId The id of the entity to be modified. Must exist in the IWorldState world.
     * @param component The component to be added/updated
     */
    template <typename T>
    void AddOrUpdateComponent(IWorldState* pWorldState, EntityId entityId, const T& component);

    /**
     * Removes a component from an entity
     *
     * @tparam T The component type
     * @param pWorldState IWorldState instance to use. Must come from the engine.
     * @param entityId The id of the entity to be modified. Must exist in the IWorldState world.
     */
    template <typename T>
    void RemoveComponent(IWorldState* pWorldState, EntityId entityId);

    /**
     * Gets the current value of an entity's component
     *
     * @tparam T The component type to query
     * @param pWorldState IWorldState instance to use. Must come from the engine.
     * @param entityId The id of the entity to be accessed. Must exist in the IWorldState world.
     *
     * @return A copy of the component's value, or std::nullopt if no such component
     */
    template <typename T>
    std::optional<T> GetComponent(IWorldState* pWorldState, EntityId entityId);
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_COMPONENTS_H
