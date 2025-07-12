/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODE_H

#include "SceneNodeComponent.h"

#include <string>

namespace Wired::Engine
{
    struct SceneNode
    {
        enum class Type
        {
            Entity,
            Player
        };

        virtual ~SceneNode() = default;

        [[nodiscard]] virtual Type GetType() const = 0;

        std::string name;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODE_H
