/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_HEIGHTMAP_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_HEIGHTMAP_H

#include <Wired/Render/Id.h>

#include <glm/glm.hpp>

namespace Wired::Engine
{
    struct PhysicsBounds_HeightMap
    {
        Render::MeshId heightMapMeshId{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_HEIGHTMAP_H
