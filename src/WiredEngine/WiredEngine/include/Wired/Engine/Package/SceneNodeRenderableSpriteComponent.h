/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODERENDERABLESPRITECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODERENDERABLESPRITECOMPONENT_H

#include "SceneNodeComponent.h"

#include <glm/glm.hpp>

#include <string>
#include <optional>

namespace Wired::Engine
{
    struct SceneNodeRenderableSpriteComponent : public SceneNodeComponent
    {
        [[nodiscard]] Type GetType() const override { return Type::RenderableSprite; };

        std::optional<std::string> imageAssetName;
        glm::vec2 destVirtualSize;
    };
}


#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODERENDERABLESPRITECOMPONENT_H
