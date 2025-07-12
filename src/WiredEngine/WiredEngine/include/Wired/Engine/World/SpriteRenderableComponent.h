/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_SPRITERENDERABLECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_SPRITERENDERABLECOMPONENT_H

#include "../EngineCommon.h"

#include <Wired/Render/Id.h>

#include <NEON/Common/Space/Rect.h>

#include <string>
#include <optional>

namespace Wired::Engine
{
    /**
     * Allows for attaching a sprite to an entity
     */
    struct SpriteRenderableComponent
    {
        /**
         * The TextureId of the texture that should be used for this sprite
         */
        Render::TextureId textureId{};

        /**
         * An optional subset of the source texture's pixel area to create the sprite
         * from. The default is to use the entire source texture.
         */
        std::optional<NCommon::RectReal> srcPixelRect{};

        /**
         * An optional virtual size to render the sprite as. Defaults to the virtual
         * size of the texture being selected as the source.
         */
        std::optional<VirtualSpaceSize> dstSize{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_SPRITERENDERABLECOMPONENT_H
