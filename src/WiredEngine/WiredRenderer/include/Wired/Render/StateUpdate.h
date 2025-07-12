/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_STATEUPDATE_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_STATEUPDATE_H

#include "Renderable/SpriteRenderable.h"
#include "Renderable/ObjectRenderable.h"
#include "Renderable/Light.h"

#include <unordered_set>
#include <vector>
#include <string>

namespace Wired::Render
{
    struct StateUpdate
    {
        std::string groupName;

        std::vector<SpriteRenderable> toAddSpriteRenderables;
        std::vector<ObjectRenderable> toAddObjectRenderables;
        std::vector<Light> toAddLights;

        std::vector<SpriteRenderable> toUpdateSpriteRenderables;
        std::vector<ObjectRenderable> toUpdateObjectRenderables;
        std::vector<Light> toUpdateLights;

        std::unordered_set<SpriteId> toDeleteSpriteRenderables;
        std::unordered_set<ObjectId> toDeleteObjectRenderables;
        std::unordered_set<LightId> toDeleteLights;
        // Note: If adding more toDeletes, make sure the renderer
        // returns their ids to the pool when processing the update

        [[nodiscard]] bool IsEmpty() const noexcept
        {
            return
                toAddSpriteRenderables.empty() &&
                toAddObjectRenderables.empty() &&
                toAddLights.empty() &&
                toUpdateSpriteRenderables.empty() &&
                toUpdateObjectRenderables.empty() &&
                toUpdateLights.empty() &&
                toDeleteSpriteRenderables.empty() &&
                toDeleteObjectRenderables.empty() &&
                toDeleteLights.empty();
        }
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_STATEUPDATE_H
