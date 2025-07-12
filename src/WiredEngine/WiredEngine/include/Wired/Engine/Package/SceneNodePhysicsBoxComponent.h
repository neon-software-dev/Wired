/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSBOXCOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSBOXCOMPONENT_H

#include "SceneNodeComponent.h"

#include <Wired/Engine/World/WorldCommon.h>

#include <glm/glm.hpp>

#include <string>

namespace Wired::Engine
{
    struct SceneNodePhysicsBoxComponent : public SceneNodeComponent
    {
        [[nodiscard]] Type GetType() const override { return Type::PhysicsBox; };

        std::string physicsScene{Engine::DEFAULT_PHYSICS_SCENE.id};
        glm::vec3 localScale{1,1,1};

        glm::vec3 min{-0.5f};
        glm::vec3 max{0.5f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSBOXCOMPONENT_H
