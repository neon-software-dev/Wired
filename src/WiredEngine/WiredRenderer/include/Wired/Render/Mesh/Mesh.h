/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_MESH_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_MESH_H

#include "MeshData.h"

#include <optional>
#include <array>
#include <memory>

namespace Wired::Render
{
    enum class MeshType
    {
        Static, // Not skeleton-based
        Bone    // Skeleton-based
    };

    static constexpr uint32_t MESH_MAX_LOD = 3; // Number of LODs a mesh can have

    struct MeshLOD
    {
        bool isValid{false};
        float renderDistance{0.0f};
        std::unique_ptr<MeshData> pMeshData{nullptr};
    };

    struct Mesh
    {
        MeshType type{};

        // Note: lod renderDistances should be ordered from nearest to farthest, and don't
        // leave invalid entries inbetween valid entries
        std::array<MeshLOD, MESH_MAX_LOD> lodData{};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MESH_MESH_H
