/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSSPHERECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSSPHERECOMPONENT_H

#include "SceneNodeComponent.h"

#include <Wired/Engine/World/WorldCommon.h>

#include <glm/glm.hpp>

namespace Wired::Engine
{
    struct SceneNodePhysicsSphereComponent : public SceneNodeComponent
    {
        [[nodiscard]] Type GetType() const override { return Type::PhysicsSphere; };

        std::string physicsScene{Engine::DEFAULT_PHYSICS_SCENE.id};
        float localScale{1.0f}; // Note: Not a vec3 as spheres require uniform scaling

        float radius{1.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODEPHYSICSSPHERECOMPONENT_H
