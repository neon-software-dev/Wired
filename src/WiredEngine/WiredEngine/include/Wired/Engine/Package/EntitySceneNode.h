/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_ENTITYSCENENODE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_ENTITYSCENENODE_H

#include "SceneNode.h"

#include <memory>
#include <vector>

namespace Wired::Engine
{
    struct EntitySceneNode : public SceneNode
    {
        [[nodiscard]] Type GetType() const override { return Type::Entity; }

        std::vector<std::shared_ptr<SceneNodeComponent>> components;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_ENTITYSCENENODE_H
