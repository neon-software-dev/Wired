/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PLAYERSCENENODE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PLAYERSCENENODE_H

#include "SceneNode.h"

#include <glm/glm.hpp>

namespace Wired::Engine
{
    struct PlayerSceneNode : public SceneNode
    {
        [[nodiscard]] Type GetType() const override { return Type::Player; }

        glm::vec3 position{0,0,0};
        float height{2.0f};
        float radius{0.5f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PLAYERSCENENODE_H
