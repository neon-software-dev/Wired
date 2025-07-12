/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ObjectBoneDataStore.h"

#include "../Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

ObjectBoneDataStore::ObjectBoneDataStore(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

ObjectBoneDataStore::~ObjectBoneDataStore()
{
    m_pGlobal = nullptr;
}

void ObjectBoneDataStore::ShutDown()
{
    for (const auto& it : m_boneTransformsBuffers)
    {
        m_pGlobal->pGPU->DestroyBuffer(it.second.GetBufferId());
    }

    for (const auto& it : m_boneMappingBuffers)
    {
        m_pGlobal->pGPU->DestroyBuffer(it.second.GetBufferId());
    }

    m_boneTransformsBuffers.clear();
    m_boneMappingBuffers.clear();
    m_objectToMesh.clear();
    m_objectToBoneStartIndex.clear();
    m_availBoneTransformsIndices.clear();
}

void ObjectBoneDataStore::Add(GPU::CopyPass copyPass, const ObjectRenderable& objectRenderable)
{
    assert(objectRenderable.boneTransforms);
    if (!objectRenderable.boneTransforms) { return; }

    //
    // Get or create the bone transforms buffer for the object's mesh
    //
    auto boneTransformsBufferIt = m_boneTransformsBuffers.find(objectRenderable.meshId);
    if (boneTransformsBufferIt == m_boneTransformsBuffers.cend())
    {
        ItemBuffer<glm::mat4> boneTransformsBuffer{};
        if (!boneTransformsBuffer.Create(m_pGlobal,
                                         {GPU::BufferUsageFlag::GraphicsStorageRead},
                                         128,
                                         false, // TODO Perf: Dedicated?
                                         std::format("BoneTransforms:{}", objectRenderable.meshId.id)))
        {
            m_pGlobal->pLogger->Error("ObjectBoneDataStore::Add: Failed to create mesh bone transforms buffer");
            return;
        }

        boneTransformsBufferIt = m_boneTransformsBuffers.insert({objectRenderable.meshId, boneTransformsBuffer}).first;
    }

    //
    // Get or create the bone mapping buffer for the object's mesh
    //
    auto boneMappingsBufferIt = m_boneMappingBuffers.find(objectRenderable.meshId);
    if (boneMappingsBufferIt == m_boneMappingBuffers.cend())
    {
        ItemBuffer<uint32_t> boneMappingsBuffer{};
        if (!boneMappingsBuffer.Create(m_pGlobal,
                                       {GPU::BufferUsageFlag::GraphicsStorageRead},
                                       32,
                                       false, // TODO Perf: Dedicated?
                                       std::format("BoneMappings:{}", objectRenderable.meshId.id)))
        {
            m_pGlobal->pLogger->Error("ObjectBoneDataStore::Add: Failed to create mesh bone mappings buffer");
            return;
        }

        boneMappingsBufferIt = m_boneMappingBuffers.insert({objectRenderable.meshId, boneMappingsBuffer}).first;
    }

    //
    // Resize the bone mapping buffer, if needed, so a mapping for this object id can be added
    //
    if (boneMappingsBufferIt->second.GetItemSize() < objectRenderable.id.id + 1)
    {
        if (!boneMappingsBufferIt->second.Resize(copyPass, objectRenderable.id.id + 1))
        {
            m_pGlobal->pLogger->Error("ObjectBoneDataStore::Add: Failed to resize bone mappings buffer");
            return;
        }
    }

    //
    // Determine where in the bone transforms buffer to place the object's bone transforms
    //
    std::optional<std::size_t> placeIndex;

    const auto availableIndicesIt = m_availBoneTransformsIndices.find(objectRenderable.meshId);
    if (availableIndicesIt != m_availBoneTransformsIndices.cend())
    {
        if (!availableIndicesIt->second.empty())
        {
            placeIndex = *availableIndicesIt->second.begin();
            availableIndicesIt->second.erase(*placeIndex);
        }
    }

    //
    // Update/Insert bone data into the bone buffer
    //
    if (placeIndex)
    {
        std::vector<ItemUpdate<glm::mat4>> itemUpdates;
        itemUpdates.reserve(objectRenderable.boneTransforms->size());

        for (unsigned int x = 0; x < objectRenderable.boneTransforms->size(); ++x)
        {
            itemUpdates.push_back(ItemUpdate<glm::mat4>{
                .item = objectRenderable.boneTransforms->at(x),
                .index = *placeIndex + x
            });
        }

        if (!boneTransformsBufferIt->second.Update("ObjectBonesTransfer", copyPass, itemUpdates))
        {
            m_pGlobal->pLogger->Error("ObjectBoneDataStore::Add: Failed to update bone transforms");
            return;
        }

        if (!boneMappingsBufferIt->second.Update("ObjectBonesMapping", copyPass, {ItemUpdate<uint32_t>{
            .item = (uint32_t)*placeIndex,
            .index = objectRenderable.id.id
        }}))
        {
            m_pGlobal->pLogger->Error("ObjectBoneDataStore::Add: Failed to update bone mapping");
            return;
        }

        m_objectToBoneStartIndex.insert({objectRenderable.id, *placeIndex});
    }
    else
    {
        const auto currentBoneTransformsItemSize = boneTransformsBufferIt->second.GetItemSize();

        if (!boneTransformsBufferIt->second.PushBack("ObjectBonesTransfer", copyPass, *objectRenderable.boneTransforms))
        {
            m_pGlobal->pLogger->Error("ObjectBoneDataStore::Add: Failed to push bone transforms");
            return;
        }

        if (!boneMappingsBufferIt->second.Update("ObjectBonesMapping", copyPass, {ItemUpdate<uint32_t>{
            .item = (uint32_t)currentBoneTransformsItemSize,
            .index = objectRenderable.id.id
        }}))
        {
            m_pGlobal->pLogger->Error("ObjectBoneDataStore::Add: Failed to update bone mapping");
            return;
        }

        m_objectToBoneStartIndex.insert({objectRenderable.id, currentBoneTransformsItemSize});
    }

    m_objectToMesh.insert({objectRenderable.id, objectRenderable.meshId});
}

void ObjectBoneDataStore::Erase(GPU::CopyPass, ObjectId objectId)
{
    const auto meshIt = m_objectToMesh.find(objectId);
    if (meshIt == m_objectToMesh.cend())
    {
        return;
    }

    const auto meshId = meshIt->second;

    auto boneMappingsBufferIt = m_boneMappingBuffers.find(meshId);
    if (boneMappingsBufferIt == m_boneMappingBuffers.cend())
    {
        m_pGlobal->pLogger->Error("ObjectBoneDataStore::Erase: Bone mappings buffer doesn't exist for mesh: {}", meshId.id);
        return;
    }

    // Technically don't need to zero out the bone data, just leave it there to be overwritten later; nothing
    // should ever use the old data after mappings are updated
    /*if (!boneMappingsBufferIt->second.Update("ObjectBonesMapping", pCopyPass, {ItemUpdate<uint32_t>{
        .item = (uint32_t)0,
        .index = objectId.id
    }}))
    {
        LogError("ObjectBoneDataStore::Erase: Failed to update bone mapping");
        return;
    }*/

    const auto previousStartIndexIt = m_objectToBoneStartIndex.find(objectId);
    if (previousStartIndexIt != m_objectToBoneStartIndex.cend())
    {
        m_availBoneTransformsIndices[meshId].insert(previousStartIndexIt->second);
        m_objectToBoneStartIndex.erase(previousStartIndexIt);
    }

    m_objectToMesh.erase(meshIt);
}

GPU::BufferId ObjectBoneDataStore::GetBoneTransformsBuffer(MeshId meshId) const
{
    return m_boneTransformsBuffers.at(meshId).GetBufferId();
}

GPU::BufferId ObjectBoneDataStore::GetBoneMappingBuffer(MeshId meshId) const
{
    return m_boneMappingBuffers.at(meshId).GetBufferId();
}

}
