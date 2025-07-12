/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_PHYSICSSTATECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_PHYSICSSTATECOMPONENT_H

#include "../InternalIds.h"

namespace Wired::Engine
{
    struct PhysicsStateComponent
    {
        PhysicsId physicsId{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_PHYSICSSTATECOMPONENT_H
