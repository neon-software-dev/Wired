/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSHEIGHTMAPCOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSHEIGHTMAPCOMPONENT_H

#include "SceneNodeComponent.h"

#include <Wired/Engine/World/WorldCommon.h>

#include <glm/glm.hpp>

#include <string>

namespace Wired::Engine
{
    struct SceneNodePhysicsHeightMapComponent : public SceneNodeComponent
    {
        [[nodiscard]] Type GetType() const override { return Type::PhysicsHeightMap; };

        std::string physicsScene{Engine::DEFAULT_PHYSICS_SCENE.id};
        glm::vec3 localScale{1,1,1};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSHEIGHTMAPCOMPONENT_H
