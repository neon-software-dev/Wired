/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODETRANSFORMCOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODETRANSFORMCOMPONENT_H

#include "SceneNodeComponent.h"

#include <glm/glm.hpp>

namespace Wired::Engine
{
    struct SceneNodeTransformComponent : public SceneNodeComponent
    {
        [[nodiscard]] Type GetType() const override { return Type::Transform; };

        glm::vec3 position{0};
        glm::vec3 scale{1};
        glm::vec3 eulerRotations{0};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODETRANSFORMCOMPONENT_H
