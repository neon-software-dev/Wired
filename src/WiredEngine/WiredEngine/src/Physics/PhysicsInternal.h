/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PHYSICS_PHYSICSINTERNAL_H
#define WIREDENGINE_WIREDENGINE_SRC_PHYSICS_PHYSICSINTERNAL_H

#include "../InternalIds.h"

#include <Wired/Engine/Physics/PhysicsCommon.h>
#include <Wired/Engine/Physics/PhysicsBounds.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <array>
#include <optional>

namespace Wired::Engine
{
    struct RigidBodyData
    {
        RigidBodyType type{};

        PhysicsShape shape{};

        glm::vec3 scale{1.0f};
        glm::vec3 position{0.0f};
        glm::quat orientation{glm::identity<glm::quat>()};

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

    struct RigidBody
    {
        bool isDirty{true};

        RigidBodyData data{};
    };

    struct PhysicsContact
    {
        PhysicsId body1{};
        PhysicsId body2{};
        ContactDetails details{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PHYSICS_PHYSICSINTERNAL_H
