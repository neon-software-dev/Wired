/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_INTERNALIDS_H
#define WIREDENGINE_WIREDENGINE_SRC_INTERNALIDS_H

#include <NEON/Common/Id.h>

namespace Wired::Engine
{
    DEFINE_INTEGRAL_ID_TYPE(PhysicsId)
}

DEFINE_INTEGRAL_ID_HASH(Wired::Engine::PhysicsId)

#endif //WIREDENGINE_WIREDENGINE_SRC_INTERNALIDS_H
