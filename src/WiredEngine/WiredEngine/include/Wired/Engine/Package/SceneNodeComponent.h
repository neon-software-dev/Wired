/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODECOMPONENT_H

namespace Wired::Engine
{
    struct SceneNodeComponent
    {
        enum class Type
        {
            Transform,
            RenderableSprite,
            RenderableModel,
            PhysicsBox,
            PhysicsSphere,
            PhysicsHeightMap
        };

        virtual ~SceneNodeComponent() = default;

        [[nodiscard]] virtual Type GetType() const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SCENENODECOMPONENT_H
