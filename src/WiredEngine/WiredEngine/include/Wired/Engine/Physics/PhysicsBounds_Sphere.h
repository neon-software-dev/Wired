/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_SPHERE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_SPHERE_H

namespace Wired::Engine
{
    /**
     * Spherical bounds for a physics object
     */
    struct PhysicsBounds_Sphere
    {
        float radius{1.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_PHYSICSBOUNDS_SPHERE_H
