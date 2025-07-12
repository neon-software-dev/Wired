/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_STATICMESHDATA_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_STATICMESHDATA_H

#include "MeshData.h"
#include "MeshVertex.h"

#include <vector>
#include <cstdint>

namespace Wired::Render
{
    struct StaticMeshData : public MeshData
    {
        StaticMeshData() = default;

        StaticMeshData(std::vector<MeshVertex> _vertices, std::vector<uint32_t> _indices)
            : vertices(std::move(_vertices))
            , indices(std::move(_indices))
        { }

        [[nodiscard]] std::optional<Volume> GetCullVolume() const override { return cullVolume; }

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::optional<Volume> cullVolume;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_STATICMESHDATA_H
