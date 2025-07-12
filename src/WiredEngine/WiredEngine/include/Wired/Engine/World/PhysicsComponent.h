/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_PHYSICSCOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_PHYSICSCOMPONENT_H

#include <Wired/Engine/World/WorldCommon.h>

#include <Wired/Engine/Physics/PhysicsCommon.h>

#include <array>
#include <optional>

namespace Wired::Engine
{
    /**
     * Attaches to an entity to give it physics properties.
     *
     * Note that only position, orientation, and linear velocity can be updated dynamically; re-create
     * the entity if any other physics property needs to be explicitly changed after creation.
     */
    struct PhysicsComponent
    {
        /**
         * Create a static physics body - Has infinite mass, no velocity
         */
        [[nodiscard]] static PhysicsComponent StaticBody(const PhysicsSceneName& scene, const PhysicsShape& shape)
        {
            PhysicsComponent physicsComponent{};
            physicsComponent.scene = scene;
            physicsComponent.bodyType = RigidBodyType::Static;
            physicsComponent.shape = shape;

            return physicsComponent;
        }

        /**
         * Create a kinematic physics body - Has infinite mass, velocity can be changed
         */
        [[nodiscard]] static PhysicsComponent KinematicBody(const PhysicsSceneName& scene, const PhysicsShape& shape)
        {
            PhysicsComponent physicsComponent{};
            physicsComponent.scene = scene;
            physicsComponent.bodyType = RigidBodyType::Kinematic;
            physicsComponent.shape = shape;

            return physicsComponent;
        }

        /**
         * Create a dynamic physics body - Has mass, has velocity
         */
        [[nodiscard]] static PhysicsComponent DynamicBody(const PhysicsSceneName& scene, const PhysicsShape& shape, float mass)
        {
            PhysicsComponent physicsComponent{};
            physicsComponent.scene = scene;
            physicsComponent.bodyType = RigidBodyType::Dynamic;
            physicsComponent.shape = shape;
            physicsComponent.mass = mass;

            return physicsComponent;
        }

        PhysicsSceneName scene{};
        RigidBodyType bodyType{RigidBodyType::Dynamic};
        PhysicsShape shape;

        //
        // Dynamic/Kinematic body properties
        //
        std::optional<glm::vec3> linearVelocity;

        //
        // Dynamic body properties
        //
        std::optional<float> mass;
        std::optional<float> linearDamping;
        std::optional<float> angularDamping;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_PHYSICSCOMPONENT_H
