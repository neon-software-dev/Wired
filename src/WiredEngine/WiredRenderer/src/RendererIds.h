/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_IDS_H
#define WIREDENGINE_WIREDRENDERER_SRC_IDS_H

#include <Wired/Render/Id.h>

#include <NEON/Common/IdSource.h>

namespace Wired::Render
{
    struct RendererIds
    {
        NCommon::IdSource<TextureId> textureIds;
        NCommon::IdSource<MeshId> meshIds;
        NCommon::IdSource<MaterialId> materialIds;
        NCommon::IdSource<ObjectId> objectIds;
        NCommon::IdSource<SpriteId> spriteIds;
        NCommon::IdSource<LightId> lightIds;

        void Reset()
        {
            textureIds.Reset();
            meshIds.Reset();
            materialIds.Reset();
            objectIds.Reset();
            spriteIds.Reset();
            lightIds.Reset();
        }
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_IDS_H
