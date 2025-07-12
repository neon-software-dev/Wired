/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_H

#include "PhysicsBounds_Sphere.h"
#include "PhysicsBounds_Box.h"
#include "PhysicsBounds_HeightMap.h"

#include <variant>

namespace Wired::Engine
{
    using PhysicsBoundsVariant = std::variant<
        PhysicsBounds_Sphere,
        PhysicsBounds_Box,
        PhysicsBounds_HeightMap
    >;
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_H
