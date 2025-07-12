/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODERENDERABLEMODELCOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODERENDERABLEMODELCOMPONENT_H

#include "SceneNodeComponent.h"

#include <string>
#include <optional>

namespace Wired::Engine
{
    struct SceneNodeRenderableModelComponent : public SceneNodeComponent
    {
        [[nodiscard]] Type GetType() const override { return Type::RenderableModel; };

        std::optional<std::string> modelAssetName;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODERENDERABLEMODELCOMPONENT_H
