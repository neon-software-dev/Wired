/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_MESHDATA_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_MESHDATA_H

#include "../Volume.h"

#include <string>
#include <optional>

namespace Wired::Render
{
    /**
     * Base class for a mesh that can be registered with the renderer
     */
    struct MeshData
    {
        virtual ~MeshData() = default;

        [[nodiscard]] virtual std::optional<Volume> GetCullVolume() const = 0;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_MESHDATA_H
