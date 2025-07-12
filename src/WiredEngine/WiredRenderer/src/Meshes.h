/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_MESHES_H
#define WIREDENGINE_WIREDRENDERER_SRC_MESHES_H

#include "ItemBuffer.h"
#include "GPUBuffer.h"

#include "Renderer/RendererCommon.h"

#include <Wired/Render/Id.h>
#include <Wired/Render/Mesh/Mesh.h>
#include <Wired/Render/Mesh/MeshVertex.h>
#include <Wired/Render/Mesh/BoneMeshVertex.h>

#include <string>
#include <unordered_map>
#include <expected>
#include <memory>
#include <optional>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Render
{
    struct Global;

    struct LoadedMeshLOD
    {
        bool isValid{false};

        float renderDistance{0.0f};

        uint32_t vertexOffset{0};

        uint32_t numIndices{0};
        uint32_t firstIndex{0};
    };

    struct LoadedMesh
    {
        MeshType meshType{};

        std::optional<Volume> cullVolume_modelSpace;
        uint32_t numBones{0};

        LoadedMeshLOD lodData[MESH_MAX_LOD]{};
    };

    class Meshes
    {
        public:

            explicit Meshes(Global* pGlobal);
            ~Meshes();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            [[nodiscard]] std::expected<std::vector<MeshId>, bool> CreateMeshes(const std::vector<const Mesh*>& meshes);

            [[nodiscard]] std::optional<LoadedMesh> GetMesh(const MeshId& meshId) const;
            [[nodiscard]] GPU::BufferId GetVerticesBuffer(MeshType meshType) const;
            [[nodiscard]] GPU::BufferId GetIndicesBuffer(MeshType meshType) const;
            [[nodiscard]] GPU::BufferId GetMeshPayloadsBuffer() const;
            void DestroyMesh(const MeshId& meshId);

        private:

            struct MeshLODPayload
            {
                alignas(4) bool isValid{false};

                alignas(4) float renderDistance{0.0f};

                alignas(4) uint32_t vertexOffset{0};
                alignas(4) uint32_t numIndices{0};
                alignas(4) uint32_t firstIndex{0};
            };

            struct MeshPayload
            {
                alignas(4) uint32_t hasCullVolume{0};
                alignas(16) glm::vec3 cullVolumeMin{0};
                alignas(16) glm::vec3 cullVolumeMax{0};

                alignas(4) uint32_t numBones{0};

                MeshLODPayload lodData[MESH_MAX_LOD]{};
            };

        private:

            Global* m_pGlobal;

            ItemBuffer<MeshVertex> m_staticMeshVerticesBuffer{};
            ItemBuffer<uint32_t> m_staticMeshIndicesBuffer{};

            ItemBuffer<BoneMeshVertex> m_boneMeshVerticesBuffer{};
            ItemBuffer<uint32_t> m_boneMeshIndicesBuffer{};

            ItemBuffer<MeshPayload> m_meshPayloadsBuffer{};

            std::unordered_map<MeshId, LoadedMesh> m_meshes;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_MESHES_H
