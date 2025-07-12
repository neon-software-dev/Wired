/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ObjectDrawPass.h"

#include "../Global.h"
#include "../Materials.h"
#include "../Pipelines.h"

#include "../DataStore/DataStores.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

ObjectDrawPass::ObjectDrawPass(Global* pGlobal,
                               std::string groupName,
                               std::string name,
                               const DataStores* pDataStores,
                               ObjectDrawPassType objectDrawPassType)
    : DrawPass(pGlobal, std::move(groupName), pDataStores)
    , m_objectDrawPassType(objectDrawPassType)
    , m_name(std::move(name))
{

}

bool ObjectDrawPass::StartUp()
{
    if (!m_membershipBuffer.Create(m_pGlobal,
                                   {GPU::BufferUsageFlag::ComputeStorageRead},
                                   64,
                                   false,
                                   std::format("ObjectMembership-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::StartUp: Failed to create membership buffer");
        return false;
    }

    if (!m_objectBatchBuffer.Create(m_pGlobal,
                                    {GPU::BufferUsageFlag::ComputeStorageRead},
                                    8,
                                    false,
                                    std::format("ObjectBatches-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::StartUp: Failed to create batch data buffer");
        return false;
    }

    if (!m_drawDataBuffer.Create(m_pGlobal,
                                 {GPU::BufferUsageFlag::ComputeStorageReadWrite, GPU::BufferUsageFlag::GraphicsStorageRead},
                                 64,
                                 false,
                                 std::format("ObjectDrawData-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::StartUp: Failed to create draw data buffer");
        return false;
    }

    if (!m_drawCommandsBuffer.Create(m_pGlobal,
                                     {GPU::BufferUsageFlag::ComputeStorageReadWrite, GPU::BufferUsageFlag::Indirect},
                                     64,
                                     false,
                                     std::format("ObjectDrawCommands-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::StartUp: Failed to create draw commands buffer");
        return false;
    }

    if (!m_drawCountsBuffer.Create(m_pGlobal,
                                  {GPU::BufferUsageFlag::ComputeStorageReadWrite, GPU::BufferUsageFlag::Indirect},
                                  64,
                                  false,
                                   std::format("ObjectDrawCounts-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::StartUp: Failed to create draw counts buffer");
        return false;
    }

    return true;
}

void ObjectDrawPass::ShutDown()
{
    m_drawCountsBuffer.Destroy();
    m_drawCommandsBuffer.Destroy();
    m_drawDataBuffer.Destroy();
    m_objectBatchBuffer.Destroy();
    m_membershipBuffer.Destroy();
}

void ObjectDrawPass::ApplyInitialUpdate(GPU::CopyPass copyPass)
{
    std::vector<ObjectRenderable> objects;
    objects.reserve(m_pDataStores->objects.GetInstances().size());

    for (const auto& instance : m_pDataStores->objects.GetInstances())
    {
        if (instance.isValid)
        {
            objects.push_back(instance.instance);
        }
    }

    ProcessAddedObjects(copyPass, objects);
}

void ObjectDrawPass::ApplyStateUpdate(GPU::CopyPass copyPass, const StateUpdate& stateUpdate)
{
    ProcessAddedObjects(copyPass, stateUpdate.toAddObjectRenderables);
    ProcessUpdatedObjects(copyPass, stateUpdate.toUpdateObjectRenderables);
    ProcessRemovedObjects(copyPass, stateUpdate.toDeleteObjectRenderables);
}

void ObjectDrawPass::ProcessAddedObjects(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objects)
{
    if (objects.empty()) { return; }

    std::optional<BatchId> lowestModifiedBatchId;
    std::optional<ObjectId> highestProcessedObjectId{0};
    bool anyObjectAdded = false;

    std::vector<ItemUpdate<MembershipPayload>> membershipUpdates;

    //
    // For each object sort it into its appropriate CPU-side batch
    //
    for (const auto& object : objects)
    {
        highestProcessedObjectId = highestProcessedObjectId ? std::max(*highestProcessedObjectId, object.id) : object.id;

        //
        // If the draw pass doesn't accept this object, enqueue a membership update which
        // explicitly marks the object as not being in a batch. This prevents there being
        // uninitialized/random data in the membership buffer.
        //
        if (!PassesObjectFilter(object))
        {
            membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
                .item = MembershipPayload{.isValid = false, .batchId = 0},
                .index = object.id.id
            });

            continue;
        }

        //
        // Otherwise, add the object to the appropriate batch
        //

        const auto batchKey = GetBatchKey(object.materialId, object.meshId);
        const auto batchIdIt = m_batchKeyToBatchId.find(batchKey);

        //
        // Create or fetch the batch the object belongs to
        //
        BatchId batchId{0};

        if (batchIdIt == m_batchKeyToBatchId.cend())
        {
            batchId = CreateBatchCPUSide(object.materialId, object.meshId);
        }
        else
        {
            batchId = batchIdIt->second;
        }

        // Add the object to its batch
        m_batches.at(batchId).objects.insert(object.id);

        // Keep an internal/CPU mapping of which batch the object belongs to
        m_objectToBatch.insert({object.id, batchId});

        // Enqueue a membership update to mark the object being part of its batch
        membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
            .item = MembershipPayload{.isValid = true, .batchId = batchId},
            .index = object.id.id
        });

        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, batchId) : batchId;
        anyObjectAdded = true;
    }

    //
    // If any batch was modified to have an object added, we need to update the GPU batch payloads
    // for that batch and any batch following it, since the payload values are dependent on which
    // objects are in which batch
    //
    if (anyObjectAdded)
    {
        if (!SyncObjectBatchPayloads(copyPass, *lowestModifiedBatchId))
        {
            m_pGlobal->pLogger->Error("ObjectDrawPass::ProcessAddedObjects: Failed to sync object batch payload data");
        }
    }

    //
    // Submit membership updates
    //
    if (!membershipUpdates.empty())
    {
        if (!m_membershipBuffer.ResizeAtLeast(copyPass, highestProcessedObjectId->id + 1))
        {
            m_pGlobal->pLogger->Error("ObjectDrawPass::ProcessAddedObjects: Failed to increase membership buffer size");
        }

        if (!m_membershipBuffer.Update("ObjectMembershipUpdate", copyPass, membershipUpdates))
        {
            m_pGlobal->pLogger->Error("ObjectDrawPass::ProcessAddedObjects: Failed to update membership buffer");
        }
    }

    //
    // If any object was added to a batch, invalidate our draw calls
    //
    if (anyObjectAdded)
    {
        MarkDrawCallsInvalidated();
    }
}

void ObjectDrawPass::ProcessUpdatedObjects(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objects)
{
    if (objects.empty()) { return; }

    bool anyObjectUpdated{false};

    std::optional<BatchId> lowestModifiedBatchId;
    std::vector<ItemUpdate<MembershipPayload>> membershipUpdates;

    for (const auto& object : objects)
    {
        const auto objectToBatchIt = m_objectToBatch.find(object.id);

        // Ignore objects not in this draw pass
        if (objectToBatchIt == m_objectToBatch.cend())
        {
            continue;
        }

        anyObjectUpdated = true;

        // The details of the batch the object currently belongs to
        const auto currentBatchId = objectToBatchIt->second;
        auto& currentBatch = m_batches.at(currentBatchId);
        const auto currentBatchKey = currentBatch.batchKey;

        // The batch that the update belongs to as of this update
        const auto latestBatchKey = GetBatchKey(object.materialId, object.meshId);

        // If the object still belongs to the same batch, nothing to do
        if (latestBatchKey == currentBatchKey)
        {
            continue;
        }

        // Otherwise, we need to switch the object's batch

        // Get or create the new/latest batch
        BatchId latestBatchId{0};

        const auto latestBatchIdIt = m_batchKeyToBatchId.find(latestBatchKey);

        if (latestBatchIdIt == m_batchKeyToBatchId.cend())
        {
            latestBatchId = CreateBatchCPUSide(object.materialId, object.meshId);
        }
        else
        {
            latestBatchId = latestBatchIdIt->second;
        }

        auto& latestBatch = m_batches.at(latestBatchId);

        // Remove the object from its current batch
        currentBatch.objects.erase(object.id);

        // Invalidate the current batch if it's now empty
        if (currentBatch.objects.empty())
        {
            currentBatch.isValid = false;
            m_batchKeyToBatchId.erase(currentBatchKey);
            m_freeBatchIds.insert(currentBatchId);
        }

        latestBatch.objects.insert(object.id);

        m_objectToBatch.insert_or_assign(object.id, latestBatchId);
        membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
            .item = MembershipPayload{.isValid = true, .batchId = latestBatchId},
            .index = object.id.id
        });

        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, currentBatchId) : currentBatchId;
        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, latestBatchId) : latestBatchId;
    }

    //
    // Update objectBatchBuffer
    //
    if (lowestModifiedBatchId)
    {
        if (!SyncObjectBatchPayloads(copyPass, *lowestModifiedBatchId))
        {
            m_pGlobal->pLogger->Error("ObjectDrawPass::ProcessUpdatedObjects: Failed to sync object batch payload data");
        }
    }

    //
    // Update membershipBuffer
    //
    if (!membershipUpdates.empty())
    {
        if (!m_membershipBuffer.Update("ObjectMembershipUpdate", copyPass, membershipUpdates))
        {
            m_pGlobal->pLogger->Error("ObjectDrawPass::ProcessUpdatedObjects: Failed to update membership buffer");
        }
    }

    //
    // Mark our draw calls as invalidated if any object in this draw pass was updated
    //
    if (lowestModifiedBatchId || anyObjectUpdated)
    {
        MarkDrawCallsInvalidated();
    }
}

void ObjectDrawPass::ProcessRemovedObjects(GPU::CopyPass copyPass, const std::unordered_set<ObjectId>& objectIds)
{
    if (objectIds.empty()) { return; }

    bool anyObjectRemoved{false};

    std::optional<BatchId> lowestModifiedBatchId;
    std::vector<ItemUpdate<MembershipPayload>> membershipUpdates;

    for (const auto& objectId : objectIds)
    {
        const auto objectToBatchIt = m_objectToBatch.find(objectId);

        // Ignore objects not in this draw pass
        if (objectToBatchIt == m_objectToBatch.cend())
        {
            continue;
        }

        anyObjectRemoved = true;

        // The details of the batch the object currently belongs to
        const auto currentBatchId = objectToBatchIt->second;
        auto& currentBatch = m_batches.at(currentBatchId);
        const auto currentBatchKey = currentBatch.batchKey;

        // Remove the object from its current batch
        currentBatch.objects.erase(objectId);

        // Invalidate the current batch if it's now empty
        if (currentBatch.objects.empty())
        {
            currentBatch.isValid = false;
            m_batchKeyToBatchId.erase(currentBatchKey);
            m_freeBatchIds.insert(currentBatchId);
        }

        m_objectToBatch.erase(objectId);
        membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
            .item = MembershipPayload{.isValid = false, .batchId = 0},
            .index = objectId.id
        });

        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, currentBatchId) : currentBatchId;
    }

    //
    // Update objectBatchBuffer
    //
    if (lowestModifiedBatchId)
    {
        if (!SyncObjectBatchPayloads(copyPass, *lowestModifiedBatchId))
        {
            m_pGlobal->pLogger->Error("ObjectDrawPass::ProcessRemovedObjects: Failed to sync object batch payload data");
        }
    }

    //
    // Update membershipBuffer
    //
    if (!membershipUpdates.empty())
    {
        if (!m_membershipBuffer.Update("ObjectMembershipUpdate", copyPass, membershipUpdates))
        {
            m_pGlobal->pLogger->Error("ObjectDrawPass::ProcessRemovedObjects: Failed to update membership buffer");
        }
    }

    //
    // Mark our draw calls as invalidated if any object in this draw pass was removed
    //
    if (lowestModifiedBatchId || anyObjectRemoved)
    {
        MarkDrawCallsInvalidated();
    }
}

ObjectDrawPass::BatchId ObjectDrawPass::CreateBatchCPUSide(MaterialId materialId, MeshId meshId)
{
    const auto batchKey = GetBatchKey(materialId, meshId);

    BatchId batchId{};

    // Attempt to re-use a previously freed batch id, first
    if (!m_freeBatchIds.empty())
    {
        batchId = *m_freeBatchIds.cbegin();
        m_freeBatchIds.erase(batchId);
    }
    // Fallback to creating a new batch id
    else
    {
        batchId = (BatchId)m_batches.size();
    }

    auto objectBatch = ObjectBatch{
        .batchId = batchId,
        .batchKey = batchKey,
        .isValid = true,
        .materialId = materialId,
        .meshId = meshId,
        .objects = {},
        .drawDataOffset = 0
    };

    //
    // Add the batch to the m_batches vector
    //
    if (m_batches.size() < objectBatch.batchId + 1)
    {
        m_batches.resize(objectBatch.batchId + 1);
    }

    m_batches.at(objectBatch.batchId) = objectBatch;

    m_batchKeyToBatchId.insert_or_assign(batchKey, batchId);

    return batchId;
}

bool ObjectDrawPass::SyncObjectBatchPayloads(GPU::CopyPass copyPass, BatchId startingBatchId)
{
    //
    // Make sure the batch payload buffer is large enough to hold data for all our batches
    //
    if (!m_objectBatchBuffer.ResizeAtLeast(copyPass, m_batches.size()))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::ResyncObjectBatchPayloads: Failed to resize object batch buffer");
        return false;
    }

    //
    // Loop through all batches and put together item updates for each batch with an id of at least lowestChangedBatchId
    //
    std::vector<ItemUpdate<ObjectBatchPayload>> batchPayloadUpdates;

    std::size_t drawDataOffset = 0;

    for (BatchId batchId = 0; batchId < m_batches.size(); ++batchId)
    {
        const auto& batch = m_batches.at(batchId);

        if (batchId >= startingBatchId)
        {
            batchPayloadUpdates.push_back(ItemUpdate<ObjectBatchPayload>{
                .item = ObjectBatchPayload{
                    .isValid = batch.isValid,
                    .meshId = batch.meshId.id,
                    .numMembers = (uint32_t)batch.objects.size(),
                    .drawDataOffset = (uint32_t)drawDataOffset,
                    .lodInstanceCounts = {0},
                },
                .index = batchId,
            });
        }

        if (batch.isValid)
        {
            drawDataOffset += batch.objects.size() * MESH_MAX_LOD;
        }
    }

    if (!m_objectBatchBuffer.Update("ObjectBatchDataUpdate", copyPass, batchPayloadUpdates))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::ResyncObjectBatchPayloads: Failed to update batch buffer");
    }

    //
    // Ensure our draw data buffer is large enough to hold all draw datas for all lod for all batches
    //
    if (!m_drawDataBuffer.ResizeAtLeast(copyPass, drawDataOffset))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::ResyncObjectBatchPayloads: Failed to resize draw data buffer");
    }

    //
    // Ensure our draw commands buffer is large enough to hold all draw commands for all batches
    //
    if (!m_drawCommandsBuffer.ResizeAtLeast(copyPass, m_batches.size() * MESH_MAX_LOD))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::ResyncObjectBatchPayloads: Failed to resize draw commands buffer");
    }

    //
    // Ensure our draw counts buffer is large enough to hold all draw counts for all batches
    //
    if (!m_drawCountsBuffer.ResizeAtLeast(copyPass, m_batches.size()))
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::ResyncObjectBatchPayloads: Failed to resize draw count buffer");
    }

    return true;
}

bool ObjectDrawPass::PassesObjectFilter(const ObjectRenderable& renderable) const
{
    const auto loadedMaterial = m_pGlobal->pMaterials->GetMaterial(renderable.materialId);
    if (!loadedMaterial)
    {
        m_pGlobal->pLogger->Error("ObjectDrawPass::PassesPassFilter: No such material exists: {}", renderable.materialId.id);
        return false;
    }

    switch (m_objectDrawPassType)
    {
        case ObjectDrawPassType::Opaque:
            return  !loadedMaterial->alphaMode ||
                    (*loadedMaterial->alphaMode == MaterialAlphaMode::Opaque) ||
                    (*loadedMaterial->alphaMode == MaterialAlphaMode::Mask);

        case ObjectDrawPassType::Translucent:
            return loadedMaterial->alphaMode && (*loadedMaterial->alphaMode == MaterialAlphaMode::Blend);

        case ObjectDrawPassType::ShadowCaster:
            return renderable.castsShadows;
    }

    assert(false);
    return false;
}

ObjectDrawPass::BatchKey ObjectDrawPass::GetBatchKey(MaterialId materialId, MeshId meshId)
{
    return NCommon::Hash(materialId, meshId);
}

void ObjectDrawPass::ComputeDrawCalls(GPU::CommandBufferId commandBufferId)
{
    if (GetNumObjects() == 0)
    {
        return;
    }

    //
    // Write compute commands
    //
    {
        CullInputParamsUniformPayload cullInputParamsPayload{
            .numGroupInstances = (uint32_t)m_pDataStores->objects.GetInstanceCount()
        };

        // Modify the view projection far plane, so we cull objects further than maxRenderDistance/objectsMaxRenderDistance
        auto viewProjection = *GetViewProjection();
        const float desiredRenderDistance = std::min(m_pGlobal->renderSettings.maxRenderDistance, m_pGlobal->renderSettings.objectsMaxRenderDistance);
        ReduceFarPlaneDistanceToNoFartherThan(viewProjection, desiredRenderDistance);

        const auto viewProjectionPayload = ViewProjectionPayloadFromViewProjection(viewProjection);

        // Fetch pipeline
        GPU::ComputePipelineParams computePipelineParams{
            .shaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("object_cull.comp")
        };

        const auto computePipelineId = m_pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
        if (!computePipelineId) { return; }

        const auto computePass = m_pGlobal->pGPU->BeginComputePass(commandBufferId, "ObjectCull");

            m_pGlobal->pGPU->CmdBindPipeline(*computePass, *computePipelineId);

            // ReadWrite storage buffers
            m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_drawDatas", m_drawDataBuffer.GetBufferId());
            m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_batchData", m_objectBatchBuffer.GetBufferId());

            // Read storage buffers
            m_pGlobal->pGPU->CmdBindStorageReadBuffer(*computePass, "i_objectInstances", m_pDataStores->objects.GetInstancePayloadsBuffer());
            m_pGlobal->pGPU->CmdBindStorageReadBuffer(*computePass, "i_membership", m_membershipBuffer.GetBufferId());
            m_pGlobal->pGPU->CmdBindStorageReadBuffer(*computePass, "i_meshPayloads", m_pGlobal->pMeshes->GetMeshPayloadsBuffer());

            // Uniform buffers
            m_pGlobal->pGPU->CmdBindUniformData(*computePass, "u_inputParams", &cullInputParamsPayload, sizeof(CullInputParamsUniformPayload));
            m_pGlobal->pGPU->CmdBindUniformData(*computePass, "u_viewProjectionData", &viewProjectionPayload, sizeof(ViewProjectionUniformPayload));

            const uint32_t workGroupSize = 256; // Must be synced to parameter in shader
            const uint32_t numWorkGroups = (cullInputParamsPayload.numGroupInstances + workGroupSize - 1) / workGroupSize;
            m_pGlobal->pGPU->CmdDispatch(*computePass, numWorkGroups, 1, 1);

        m_pGlobal->pGPU->EndComputePass(*computePass);
    }

    {
        DrawInputParamsUniformPayload drawInputParamsPayload{
            .numBatches = (uint32_t)m_batches.size()
        };

        // Fetch pipeline
        GPU::ComputePipelineParams computePipelineParams{
            .shaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("object_draw.comp")
        };

        const auto computePipelineId = m_pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
        if (!computePipelineId) { return; }

        const auto computePass = m_pGlobal->pGPU->BeginComputePass(commandBufferId, "ObjectDraw");

            m_pGlobal->pGPU->CmdBindPipeline(*computePass, *computePipelineId);

            // ReadWrite storage buffers
            m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_batchData", m_objectBatchBuffer.GetBufferId());
            m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_drawCommands", m_drawCommandsBuffer.GetBufferId());
            m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_drawCounts", m_drawCountsBuffer.GetBufferId());

            // Read storage buffers
            m_pGlobal->pGPU->CmdBindStorageReadBuffer(*computePass, "i_meshPayloads", m_pGlobal->pMeshes->GetMeshPayloadsBuffer());

            // Uniforms
            m_pGlobal->pGPU->CmdBindUniformData(*computePass, "u_inputParams", &drawInputParamsPayload, sizeof(DrawInputParamsUniformPayload));

            const uint32_t workGroupSize = 256; // Must be synced to parameter in shader
            const uint32_t numWorkGroups = (drawInputParamsPayload.numBatches + workGroupSize - 1) / workGroupSize;
            m_pGlobal->pGPU->CmdDispatch(*computePass, numWorkGroups, 1, 1);

        m_pGlobal->pGPU->EndComputePass(*computePass);
    }
}

void ObjectDrawPass::OnRenderSettingsChanged()
{
    // Render distance might have changed, which affects culling, so we need to re-compute draw calls
    MarkDrawCallsInvalidated();
}

std::string ObjectDrawPass::GetTag() const noexcept
{
    switch (m_objectDrawPassType)
    {
        case ObjectDrawPassType::Opaque: return std::format("{}:{}:ObjectOpaque", m_groupName, m_name);
        case ObjectDrawPassType::Translucent: return std::format("{}:{}:ObjectTranslucent", m_groupName, m_name);
        case ObjectDrawPassType::ShadowCaster: return std::format("{}:{}:ObjectShadowCaster", m_groupName, m_name);
    }

    assert(false);
    return "";
}

std::vector<ObjectDrawPass::RenderBatch> ObjectDrawPass::GetRenderBatches() const
{
    std::vector<ObjectDrawPass::RenderBatch> renderBatches;

    for (const auto& batch : m_batches)
    {
        if (batch.isValid)
        {
            renderBatches.emplace_back(batch.batchId, batch.materialId, batch.meshId);
        }
    }

    return renderBatches;
}

}
