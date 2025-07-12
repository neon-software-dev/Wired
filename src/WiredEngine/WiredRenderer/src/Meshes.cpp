/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Meshes.h"

#include "Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <Wired/Render/Mesh/StaticMeshData.h>
#include <Wired/Render/Mesh/BoneMeshData.h>

#include <NEON/Common/Log/ILogger.h>

#include <algorithm>
#include <functional>

namespace Wired::Render
{

Meshes::Meshes(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Meshes::~Meshes()
{
    m_pGlobal = nullptr;
}

bool Meshes::StartUp()
{
    m_pGlobal->pLogger->Info("Meshes: Starting Up");

    // TODO Perf: Dedicated memory for these buffers? Though VMA complains if the buffer sizes are too small

    //
    // Create persistent mesh data buffers
    //
    if (!m_staticMeshVerticesBuffer.Create(m_pGlobal, {GPU::BufferUsageFlag::Vertex}, 1024, false, "StaticVertices"))
    {
        return false;
    }
    if (!m_staticMeshIndicesBuffer.Create(m_pGlobal, {GPU::BufferUsageFlag::Index}, 1024, false, "StaticIndices"))
    {
        return false;
    }
    if (!m_boneMeshVerticesBuffer.Create(m_pGlobal, {GPU::BufferUsageFlag::Vertex}, 1024, false, "BoneVertices"))
    {
        return false;
    }
    if (!m_boneMeshIndicesBuffer.Create(m_pGlobal, {GPU::BufferUsageFlag::Index}, 1024, false, "BoneIndices"))
    {
        return false;
    }
    if (!m_meshPayloadsBuffer.Create(m_pGlobal, {GPU::BufferUsageFlag::GraphicsStorageRead, GPU::BufferUsageFlag::ComputeStorageRead}, 64, false, "MeshPayloads"))
    {
        return false;
    }

    return true;
}

void Meshes::ShutDown()
{
    m_pGlobal->pLogger->Info("Meshes: Shutting down");

    while (!m_meshes.empty())
    {
        DestroyMesh(m_meshes.cbegin()->first);
    }

    m_staticMeshVerticesBuffer.Destroy();
    m_staticMeshIndicesBuffer.Destroy();
    m_boneMeshVerticesBuffer.Destroy();
    m_boneMeshIndicesBuffer.Destroy();
    m_meshPayloadsBuffer.Destroy();
}

std::expected<std::vector<MeshId>, bool> Meshes::CreateMeshes(const std::vector<const Mesh*>& meshes)
{
    if (meshes.empty()) { return {}; }

    std::vector<MeshVertex> staticMeshVertices;
    std::vector<uint32_t> staticMeshIndices;
    std::vector<BoneMeshVertex> boneMeshVertices;
    std::vector<uint32_t> boneMeshIndices;

    std::vector<MeshPayload> meshPayloads;
    std::vector<LoadedMesh> loadedMeshes;
    std::vector<MeshId> meshIds;
    MeshId highestMeshId{};

    //
    // Fill the above vectors with data about the meshes being created
    //
    std::size_t staticMeshVertexOffset = m_staticMeshVerticesBuffer.GetItemSize();
    std::size_t firstStaticMeshIndex = m_staticMeshIndicesBuffer.GetItemSize();

    std::size_t boneMeshVertexOffset = m_boneMeshVerticesBuffer.GetItemSize();
    std::size_t firstBoneMeshIndex = m_boneMeshIndicesBuffer.GetItemSize();

    for (const auto& mesh : meshes)
    {
        switch (mesh->type)
        {
            case MeshType::Static:
            {
                const StaticMeshData* lod0MeshData{nullptr};

                {
                    const auto& meshLOD = mesh->lodData.at(0);
                    if (!meshLOD.isValid)
                    {
                        m_pGlobal->pLogger->Error("Meshes::CreateMeshes: Mesh must have at least LOD 0 provided");
                        return std::unexpected(false);
                    }

                    lod0MeshData = dynamic_cast<const StaticMeshData*>(meshLOD.pMeshData.get());
                }

                //
                // Populate LoadedMesh and MeshPayload from lod0 which must exist
                //
                auto loadedMesh = LoadedMesh{};
                loadedMesh.meshType = MeshType::Static;
                loadedMesh.cullVolume_modelSpace = lod0MeshData->cullVolume;
                loadedMesh.numBones = 0;

                auto meshPayload = MeshPayload{};
                meshPayload.hasCullVolume = lod0MeshData->cullVolume.has_value();
                if (lod0MeshData->cullVolume.has_value())
                {
                    meshPayload.cullVolumeMin = lod0MeshData->cullVolume->min;
                    meshPayload.cullVolumeMax = lod0MeshData->cullVolume->max;
                }
                meshPayload.numBones = 0;

                //
                // Populate MeshPayloadData for each lod
                //
                for (unsigned int lod = 0; lod < MESH_MAX_LOD; ++lod)
                {
                    const auto& meshLOD = mesh->lodData.at(lod);
                    if (!meshLOD.isValid)
                    {
                        continue;
                    }

                    const auto pLODMesh = dynamic_cast<const StaticMeshData*>(meshLOD.pMeshData.get());

                    meshPayload.lodData[lod].isValid = true;
                    meshPayload.lodData[lod].renderDistance = meshLOD.renderDistance;
                    meshPayload.lodData[lod].vertexOffset = (uint32_t)staticMeshVertexOffset;
                    meshPayload.lodData[lod].numIndices = (uint32_t)pLODMesh->indices.size();
                    meshPayload.lodData[lod].firstIndex = (uint32_t)firstStaticMeshIndex;

                    loadedMesh.lodData[lod] = LoadedMeshLOD{
                        .isValid = true,
                        .renderDistance = meshPayload.lodData[lod].renderDistance,
                        .vertexOffset = meshPayload.lodData[lod].vertexOffset,
                        .numIndices = meshPayload.lodData[lod].numIndices,
                        .firstIndex = meshPayload.lodData[lod].firstIndex
                    };

                    staticMeshVertices.insert(staticMeshVertices.end(), pLODMesh->vertices.cbegin(), pLODMesh->vertices.cend());
                    staticMeshIndices.insert(staticMeshIndices.end(), pLODMesh->indices.cbegin(), pLODMesh->indices.cend());

                    staticMeshVertexOffset += pLODMesh->vertices.size();
                    firstStaticMeshIndex += pLODMesh->indices.size();
                }

                meshPayloads.push_back(meshPayload);
                loadedMeshes.push_back(loadedMesh);
            }
            break;
            case MeshType::Bone:
            {
                const BoneMeshData* lod0MeshData{nullptr};

                {
                    const auto& meshLOD = mesh->lodData.at(0);
                    if (!meshLOD.isValid)
                    {
                        m_pGlobal->pLogger->Error("Meshes::CreateMeshes: Mesh must have at least LOD 0 provided");
                        return std::unexpected(false);
                    }

                    lod0MeshData = dynamic_cast<const BoneMeshData*>(meshLOD.pMeshData.get());
                }

                //
                // Populate LoadedMesh and MeshPayload from lod0 which must exist
                //
                auto loadedMesh = LoadedMesh{};
                loadedMesh.meshType = MeshType::Bone;
                loadedMesh.cullVolume_modelSpace = lod0MeshData->cullVolume;
                loadedMesh.numBones = lod0MeshData->numBones;

                auto meshPayload = MeshPayload{};
                meshPayload.hasCullVolume = lod0MeshData->cullVolume.has_value();
                if (lod0MeshData->cullVolume.has_value())
                {
                    meshPayload.cullVolumeMin = lod0MeshData->cullVolume->min;
                    meshPayload.cullVolumeMax = lod0MeshData->cullVolume->max;
                }
                meshPayload.numBones = lod0MeshData->numBones;

                //
                // Populate MeshPayloadData for each lod
                //
                for (unsigned int lod = 0; lod < MESH_MAX_LOD; ++lod)
                {
                    const auto& meshLOD = mesh->lodData.at(lod);
                    if (!meshLOD.isValid)
                    {
                        continue;
                    }

                    const auto pLODMesh = dynamic_cast<const BoneMeshData*>(meshLOD.pMeshData.get());

                    meshPayload.lodData[lod].isValid = true;
                    meshPayload.lodData[lod].renderDistance = meshLOD.renderDistance;
                    meshPayload.lodData[lod].vertexOffset = (uint32_t)boneMeshVertexOffset;
                    meshPayload.lodData[lod].numIndices = (uint32_t)pLODMesh->indices.size();
                    meshPayload.lodData[lod].firstIndex = (uint32_t)firstBoneMeshIndex;

                    loadedMesh.lodData[lod] = LoadedMeshLOD{
                        .isValid = true,
                        .renderDistance = meshPayload.lodData[lod].renderDistance,
                        .vertexOffset = meshPayload.lodData[lod].vertexOffset,
                        .numIndices = meshPayload.lodData[lod].numIndices,
                        .firstIndex = meshPayload.lodData[lod].firstIndex
                    };

                    boneMeshVertices.insert(boneMeshVertices.end(), pLODMesh->vertices.cbegin(), pLODMesh->vertices.cend());
                    boneMeshIndices.insert(boneMeshIndices.end(), pLODMesh->indices.cbegin(), pLODMesh->indices.cend());

                    boneMeshVertexOffset += pLODMesh->vertices.size();
                    firstBoneMeshIndex += pLODMesh->indices.size();
                }

                meshPayloads.push_back(meshPayload);
                loadedMeshes.push_back(loadedMesh);
            }
            break;
        }

        const auto meshId = m_pGlobal->ids.meshIds.GetId();
        meshIds.push_back(meshId);
        highestMeshId = MeshId(std::max(highestMeshId.id, meshId.id));
    }

    //
    // Upload data to the GPU
    //
    const auto cmdBuffer = m_pGlobal->pGPU->AcquireCommandBuffer(true, "CreateMeshes");
    if (!cmdBuffer)
    {
        m_pGlobal->pLogger->Error("Meshes::CreateMeshes: Failed to acquire command buffer");
        std::ranges::for_each(meshIds, [&](const MeshId& meshId){ m_pGlobal->ids.meshIds.ReturnId(meshId); });
        return std::unexpected(false);
    }

    const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(*cmdBuffer, "MeshDataTransfer");

    bool allSuccessful = true;

    // Upload vertices
    if (!staticMeshVertices.empty())
    {
        allSuccessful &= m_staticMeshVerticesBuffer.PushBack("StaticVertexUpload", *copyPass, staticMeshVertices);
    }
    if (!boneMeshVertices.empty())
    {
        allSuccessful &= m_boneMeshVerticesBuffer.PushBack("BoneVertexUpload", *copyPass, boneMeshVertices);
    }

    // Upload indices
    if (!staticMeshIndices.empty())
    {
        allSuccessful &= m_staticMeshIndicesBuffer.PushBack("StaticIndexUpload", *copyPass, staticMeshIndices);
    }
    if (!boneMeshIndices.empty())
    {
        allSuccessful &= m_boneMeshIndicesBuffer.PushBack("BoneIndexUpload", *copyPass, boneMeshIndices);
    }

    if (!allSuccessful)
    {
        m_pGlobal->pLogger->Error("Meshes::CreateMeshes: Failed to upload vertex/index data");
        m_pGlobal->pGPU->CancelCommandBuffer(*cmdBuffer);
        std::ranges::for_each(meshIds, [&](const MeshId& meshId){ m_pGlobal->ids.meshIds.ReturnId(meshId); });
        return std::unexpected(false);
    }

    // Upload mesh payloads
    if (m_meshPayloadsBuffer.GetItemSize() < (highestMeshId.id + 1))
    {
        if (!m_meshPayloadsBuffer.Resize(*copyPass, (highestMeshId.id + 1)))
        {
            m_pGlobal->pLogger->Error("Meshes::CreateMeshes: Failed to resize mesh payloads");
            m_pGlobal->pGPU->CancelCommandBuffer(*cmdBuffer);
            std::ranges::for_each(meshIds, [&](const MeshId& meshId){ m_pGlobal->ids.meshIds.ReturnId(meshId); });
            return std::unexpected(false);
        }
    }

    // Note that we can't push_back the payloads since the mesh ids we got might not have been contiguous,
    // it can give previously returned, now-unused, mesh ids. We need to update each spot instead.
    std::vector<ItemUpdate<MeshPayload>> payloadsUpdates;
    payloadsUpdates.reserve(meshPayloads.size());

    for (std::size_t x = 0; x < meshPayloads.size(); ++x)
    {
        const auto& meshPayload = meshPayloads.at(x);
        const auto& meshId = meshIds.at(x);

        payloadsUpdates.push_back({.item = meshPayload, .index = meshId.id});
    }

    if (!m_meshPayloadsBuffer.Update("MeshPayloadUpload", *copyPass, payloadsUpdates))
    {
        m_pGlobal->pLogger->Error("Meshes::CreateMeshes: Failed to resize mesh payloads");
        m_pGlobal->pGPU->CancelCommandBuffer(*cmdBuffer);
        std::ranges::for_each(meshIds, [&](const MeshId& meshId){ m_pGlobal->ids.meshIds.ReturnId(meshId); });
        return std::unexpected(false);
    }

    m_pGlobal->pGPU->EndCopyPass(*copyPass);
    (void)m_pGlobal->pGPU->SubmitCommandBuffer(*cmdBuffer);

    //
    // Record state
    //
    for (std::size_t x = 0; x < loadedMeshes.size(); ++x)
    {
        m_meshes.insert({meshIds.at(x), loadedMeshes.at(x)});
    }

    return meshIds;
}

std::optional<LoadedMesh> Meshes::GetMesh(const MeshId& meshId) const
{
    const auto it = m_meshes.find(meshId);
    if (it == m_meshes.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void Meshes::DestroyMesh(const MeshId& meshId)
{
    m_pGlobal->pLogger->Info("Meshes: Destroying mesh: {}", meshId.id);

    const auto it = m_meshes.find(meshId);
    if (it == m_meshes.cend())
    {
        m_pGlobal->pLogger->Warning("Meshes::DestroyMesh: No such mesh: {}", meshId.id);
        return;
    }

    // TODO: We currently don't really support destroying meshes; it'll work fine
    //  but the associated memory in the big buffers will never be reclaimed.

    /*SDL_ReleaseGPUBuffer(m_pShared->GetSDLGPUDevice(), it->second.pSDLVertexBuffer);
    SDL_ReleaseGPUBuffer(m_pShared->GetSDLGPUDevice(), it->second.pSDLIndexBuffer);

    if (it->second.pSDLMeshDataBuffer)
    {
        SDL_ReleaseGPUBuffer(m_pShared->GetSDLGPUDevice(), *it->second.pSDLMeshDataBuffer);
    }*/

    m_meshes.erase(it);
}

GPU::BufferId Meshes::GetVerticesBuffer(MeshType meshType) const
{
    switch (meshType)
    {
        case MeshType::Static: return m_staticMeshVerticesBuffer.GetBufferId();
        case MeshType::Bone: return m_boneMeshVerticesBuffer.GetBufferId();
    }

    assert(false);
    return {};
}

GPU::BufferId Meshes::GetIndicesBuffer(MeshType meshType) const
{
    switch (meshType)
    {
        case MeshType::Static: return m_staticMeshIndicesBuffer.GetBufferId();
        case MeshType::Bone: return m_boneMeshIndicesBuffer.GetBufferId();
    }

    assert(false);
    return {};
}

GPU::BufferId Meshes::GetMeshPayloadsBuffer() const
{
    return m_meshPayloadsBuffer.GetBufferId();
}

}