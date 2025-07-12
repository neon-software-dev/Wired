/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PHYSICS_PHYSICSCOMMON_H
#define WIREDENGINE_WIREDENGINE_SRC_PHYSICS_PHYSICSCOMMON_H

#include "PhysicsBounds.h"

#include <Wired/Engine/World/WorldCommon.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <optional>
#include <vector>

namespace Wired::Engine
{
    enum class RigidBodyType
    {
        /** Infinite mass, manually controlled */
        Static,

        /** Specific mass, manually controlled */
        Kinematic,

        /** Specific mass, physics controlled */
        Dynamic
    };

    enum class ShapeUsage
    {
        /** The shape will take part in normal physics simulation */
        Simulation,

        /** The shape will be used as a trigger and not take part in the physics simulation */
        Trigger
    };

    struct PhysicsMaterial
    {
        float friction{1.0f};
        float restitution{0.1f};

        auto operator<=>(const PhysicsMaterial&) const = default;
    };

    struct PhysicsShape
    {
        /**
        * Whether the shape is part of the physics simulation or a trigger shape.
        *
        * Note: If set to Trigger, the shape will not take part in the physics simulation and will only be used
        * as a trigger shape.
        */
        ShapeUsage usage{ShapeUsage::Simulation};

        /** The material applied to the shape */
        PhysicsMaterial material{};

        /** Model-space bounds defining the shape */
        PhysicsBoundsVariant bounds{};

        /** Additional local scale applied to the shape's bounds (defaults to none) */
        glm::vec3 localScale{1.0f};

        /** Additional local transform applied to the shape's bounds, relative to the entity's model
         * space (defaults to none) */
        glm::vec3 localTransform{0.0f};

        /** Additional local orientation applied to the shape's bounds, relative to the entity's model
         * space (defaults to none) */
        glm::quat localOrientation{glm::identity<glm::quat>()};
    };

    enum class ContactType
    {
        Added,
        Removed
    };

    struct ContactDetails
    {
        ContactType type{};
        std::optional<float> penetrationDepth{std::nullopt};
        std::optional<std::vector<glm::vec3>> entity1ContactPoints_worldSpace{std::nullopt};
        std::optional<std::vector<glm::vec3>> entity2ContactPoints_worldSpace{std::nullopt};
    };

    struct EntityContact
    {
        EntityId entity1{};
        EntityId entity2{};
        ContactDetails details{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PHYSICS_PHYSICSCOMMON_H
