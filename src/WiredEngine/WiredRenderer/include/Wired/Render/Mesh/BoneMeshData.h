/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_BONEMESHDATA_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_BONEMESHDATA_H

#include "MeshData.h"
#include "BoneMeshVertex.h"

#include <vector>

namespace Wired::Render
{
    /**
     * A mesh with a bone-based skeleton
     */
    struct BoneMeshData : public MeshData
    {
        BoneMeshData() = default;

        BoneMeshData(std::vector<BoneMeshVertex> _vertices,
                     std::vector<uint32_t> _indices,
                     uint32_t _numBones)
            : vertices(std::move(_vertices))
            , indices(std::move(_indices))
            , numBones(_numBones)
        { }

        [[nodiscard]] std::optional<Volume> GetCullVolume() const override { return cullVolume; }

        std::vector<BoneMeshVertex> vertices;
        std::vector<uint32_t> indices;
        uint32_t numBones;
        std::optional<Volume> cullVolume;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_BONEMESHDATA_H
