/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_BOX_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_BOX_H

#include <glm/glm.hpp>

namespace Wired::Engine
{
    struct PhysicsBounds_Box
    {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_BOX_H
