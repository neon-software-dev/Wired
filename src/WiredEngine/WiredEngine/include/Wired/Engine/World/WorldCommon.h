/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_WORLDCOMMON_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_WORLDCOMMON_H

#include <NEON/Common/Id.h>

#include <cstdint>

namespace Wired::Engine
{
    enum class EntityId : std::uint64_t {};

    DEFINE_INTEGRAL_ID_TYPE(CameraId)

    DEFINE_STRING_ID_TYPE(PackageName)

    DEFINE_STRING_ID_TYPE(PhysicsSceneName)
    static const auto DEFAULT_PHYSICS_SCENE = PhysicsSceneName("Default");

    // TODO: Make a WorldName type like PhysicsSceneName
    static const auto DEFAULT_WORLD_NAME = "Default";

    enum class CameraType
    {
        CAMERA_2D,
        CAMERA_3D
    };
}

DEFINE_INTEGRAL_ID_HASH(Wired::Engine::CameraId)
DEFINE_STRING_ID_HASH(Wired::Engine::PackageName)
DEFINE_STRING_ID_HASH(Wired::Engine::PhysicsSceneName)

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_WORLDCOMMON_H
