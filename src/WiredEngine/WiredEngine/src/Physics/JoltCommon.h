/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTCOMMON_H
#define WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTCOMMON_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Wired::Engine
{
    inline glm::vec3 FromJPH(const JPH::Vec3& in) { return {in.GetX(), in.GetY(), in.GetZ()}; }
    inline JPH::Vec3 ToJPH(const glm::vec3& in) { return {in.x, in.y, in.z}; }

    inline glm::quat FromJPH(const JPH::Quat& in) { return {in.GetW(), in.GetX(), in.GetY(), in.GetZ()}; }
    inline JPH::Quat ToJPH(const glm::quat& in) { return {in.x, in.y, in.z, in.w}; }

    namespace Layers
    {
        static constexpr JPH::ObjectLayer NON_MOVING = 0;
        static constexpr JPH::ObjectLayer MOVING = 1;
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    }
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTCOMMON_H
