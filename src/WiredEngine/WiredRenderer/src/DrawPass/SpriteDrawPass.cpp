/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SpriteDrawPass.h"

#include "../Global.h"
#include "../Materials.h"
#include "../Pipelines.h"

#include "../DataStore/DataStores.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

SpriteDrawPass::SpriteDrawPass(Global* pGlobal,
                               std::string groupName,
                               std::string name,
                               const DataStores* pDataStores)
    : DrawPass(pGlobal, std::move(groupName), pDataStores)
    , m_name(std::move(name))
{

}

bool SpriteDrawPass::StartUp()
{
    if (!m_membershipBuffer.Create(m_pGlobal,
                                   {GPU::BufferUsageFlag::ComputeStorageRead},
                                   64,
                                   false,
                                   std::format("SpriteMembership-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::StartUp: Failed to create membership buffer");
        return false;
    }

    if (!m_spriteBatchBuffer.Create(m_pGlobal,
                                    {GPU::BufferUsageFlag::ComputeStorageReadWrite},
                                    8,
                                    false,
                                    std::format("SpriteBatches-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::StartUp: Failed to create batch data buffer");
        return false;
    }

    if (!m_drawDataBuffer.Create(m_pGlobal,
                                 {GPU::BufferUsageFlag::ComputeStorageReadWrite, GPU::BufferUsageFlag::GraphicsStorageRead},
                                 64,
                                 false,
                                 std::format("SpriteDrawData-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::StartUp: Failed to create draw data buffer");
        return false;
    }

    if (!m_drawCommandsBuffer.Create(m_pGlobal,
                                     {GPU::BufferUsageFlag::ComputeStorageReadWrite, GPU::BufferUsageFlag::Indirect},
                                     64,
                                     false,
                                     std::format("SpriteDrawCommands-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::StartUp: Failed to create draw commands buffer");
        return false;
    }

    if (!m_drawCountsBuffer.Create(m_pGlobal,
                                   {GPU::BufferUsageFlag::ComputeStorageReadWrite, GPU::BufferUsageFlag::Indirect},
                                   64,
                                   false,
                                   std::format("SpriteDrawCounts-{}", m_name)))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::StartUp: Failed to create draw counts buffer");
        return false;
    }

    return true;
}

void SpriteDrawPass::ShutDown()
{
    m_drawCountsBuffer.Destroy();
    m_drawCommandsBuffer.Destroy();
    m_drawDataBuffer.Destroy();
    m_spriteBatchBuffer.Destroy();
    m_membershipBuffer.Destroy();
}

void SpriteDrawPass::ApplyInitialUpdate(GPU::CopyPass copyPass)
{
    std::vector<SpriteRenderable> sprites;
    sprites.reserve(m_pDataStores->sprites.GetInstances().size());

    for (const auto& instance : m_pDataStores->sprites.GetInstances())
    {
        if (instance.isValid)
        {
            sprites.push_back(instance.instance);
        }
    }

    ProcessAddedSprites(copyPass, sprites);
}

void SpriteDrawPass::ApplyStateUpdate(GPU::CopyPass copyPass, const StateUpdate& stateUpdate)
{
    ProcessAddedSprites(copyPass, stateUpdate.toAddSpriteRenderables);
    ProcessUpdatedSprites(copyPass, stateUpdate.toUpdateSpriteRenderables);
    ProcessRemovedSprites(copyPass, stateUpdate.toDeleteSpriteRenderables);
}

void SpriteDrawPass::ProcessAddedSprites(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& sprites)
{
    if (sprites.empty()) { return; }

    std::optional<BatchId> lowestModifiedBatchId;
    std::optional<SpriteId> highestProcessedSpriteId{0};
    bool anySpriteAdded = false;

    std::vector<ItemUpdate<MembershipPayload>> membershipUpdates;

    //
    // For each sprite sort it into its appropriate CPU-side batch
    //
    for (const auto& sprite : sprites)
    {
        highestProcessedSpriteId = highestProcessedSpriteId ? std::max(*highestProcessedSpriteId, sprite.id) : sprite.id;

        //
        // If the draw pass doesn't accept this sprite, enqueue a membership update which
        // explicitly marks the sprite as not being in a batch. This prevents there being
        // uninitialized/random data in the membership buffer.
        //
        if (!PassesSpriteFilter(sprite))
        {
            membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
                .item = MembershipPayload{.isValid = false, .batchId = 0},
                .index = sprite.id.id
            });

            continue;
        }

        //
        // Otherwise, add the sprite to the appropriate batch
        //

        const auto batchKey = GetBatchKey(sprite.textureId);
        const auto batchIdIt = m_batchKeyToBatchId.find(batchKey);

        //
        // Create or fetch the batch the sprite belongs to
        //
        BatchId batchId{0};

        if (batchIdIt == m_batchKeyToBatchId.cend())
        {
            batchId = CreateBatchCPUSide(sprite.textureId);
        }
        else
        {
            batchId = batchIdIt->second;
        }

        // Add the sprite to its batch
        m_batches.at(batchId).sprites.insert(sprite.id);

        // Keep an internal/CPU mapping of which batch the sprite belongs to
        m_spriteToBatch.insert({sprite.id, batchId});

        // Enqueue a membership update to mark the sprite being part of its batch
        membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
            .item = MembershipPayload{.isValid = true, .batchId = batchId},
            .index = sprite.id.id
        });

        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, batchId) : batchId;
        anySpriteAdded = true;
    }

    //
    // If any batch was modified to have an sprite added, we need to update the GPU batch payloads
    // for that batch and any batch following it, since the payload values are dependent on which
    // sprites are in which batch
    //
    if (anySpriteAdded)
    {
        if (!SyncSpriteBatchPayloads(copyPass, *lowestModifiedBatchId))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessAddedSprites: Failed to sync sprite batch payload data");
        }
    }

    //
    // Submit membership updates
    //
    if (!membershipUpdates.empty())
    {
        if (!m_membershipBuffer.ResizeAtLeast(copyPass, highestProcessedSpriteId->id + 1))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessAddedSprites: Failed to increase membership buffer size");
        }

        if (!m_membershipBuffer.Update("SpriteMembershipUpdate", copyPass, membershipUpdates))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessAddedSprites: Failed to update membership buffer");
        }
    }

    //
    // If any sprite was added to a batch, invalidate our draw calls
    //
    if (anySpriteAdded)
    {
        MarkDrawCallsInvalidated();
    }
}

void SpriteDrawPass::ProcessUpdatedSprites(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& sprites)
{
    if (sprites.empty()) { return; }

    bool anySpriteUpdated{false};

    std::optional<BatchId> lowestModifiedBatchId;
    std::vector<ItemUpdate<MembershipPayload>> membershipUpdates;

    for (const auto& sprite : sprites)
    {
        const auto spriteToBatchIt = m_spriteToBatch.find(sprite.id);

        // Ignore sprites not in this draw pass
        if (spriteToBatchIt == m_spriteToBatch.cend())
        {
            continue;
        }

        anySpriteUpdated = true;

        // The details of the batch the sprite currently belongs to
        const auto currentBatchId = spriteToBatchIt->second;
        auto& currentBatch = m_batches.at(currentBatchId);
        const auto currentBatchKey = currentBatch.batchKey;

        // The batch that the update belongs to as of this update
        const auto latestBatchKey = GetBatchKey(sprite.textureId);

        // If the sprite still belongs to the same batch, nothing to do
        if (latestBatchKey == currentBatchKey)
        {
            continue;
        }

        // Otherwise, we need to switch the sprite's batch

        // Get or create the new/latest batch
        BatchId latestBatchId{0};

        const auto latestBatchIdIt = m_batchKeyToBatchId.find(latestBatchKey);

        if (latestBatchIdIt == m_batchKeyToBatchId.cend())
        {
            latestBatchId = CreateBatchCPUSide(sprite.textureId);
        }
        else
        {
            latestBatchId = latestBatchIdIt->second;
        }

        auto& latestBatch = m_batches.at(latestBatchId);

        // Remove the sprite from its current batch
        currentBatch.sprites.erase(sprite.id);

        // Invalidate the current batch if it's now empty
        if (currentBatch.sprites.empty())
        {
            currentBatch.isValid = false;
            m_batchKeyToBatchId.erase(currentBatchKey);
            m_freeBatchIds.insert(currentBatchId);
        }

        latestBatch.sprites.insert(sprite.id);

        m_spriteToBatch.insert_or_assign(sprite.id, latestBatchId);
        membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
            .item = MembershipPayload{.isValid = true, .batchId = latestBatchId},
            .index = sprite.id.id
        });

        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, currentBatchId) : currentBatchId;
        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, latestBatchId) : latestBatchId;
    }

    //
    // Update spriteBatchBuffer
    //
    if (lowestModifiedBatchId)
    {
        if (!SyncSpriteBatchPayloads(copyPass, *lowestModifiedBatchId))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessUpdatedSprites: Failed to sync sprite batch payload data");
        }
    }

    //
    // Update membershipBuffer
    //
    if (!membershipUpdates.empty())
    {
        if (!m_membershipBuffer.Update("SpriteMembershipUpdate", copyPass, membershipUpdates))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessUpdatedSprites: Failed to update membership buffer");
        }
    }

    //
    // Mark our draw calls as invalidated if any sprite in this draw pass was updated
    //
    if (lowestModifiedBatchId || anySpriteUpdated)
    {
        MarkDrawCallsInvalidated();
    }
}

void SpriteDrawPass::ProcessRemovedSprites(GPU::CopyPass copyPass, const std::unordered_set<SpriteId>& spriteIds)
{
    if (spriteIds.empty()) { return; }

    bool anySpriteRemoved{false};

    std::optional<BatchId> lowestModifiedBatchId;
    std::vector<ItemUpdate<MembershipPayload>> membershipUpdates;

    for (const auto& spriteId : spriteIds)
    {
        const auto spriteToBatchIt = m_spriteToBatch.find(spriteId);

        // Ignore sprites not in this draw pass
        if (spriteToBatchIt == m_spriteToBatch.cend())
        {
            continue;
        }

        anySpriteRemoved = true;

        // The details of the batch the sprite currently belongs to
        const auto currentBatchId = spriteToBatchIt->second;
        auto& currentBatch = m_batches.at(currentBatchId);
        const auto currentBatchKey = currentBatch.batchKey;

        // Remove the sprite from its current batch
        currentBatch.sprites.erase(spriteId);

        // Invalidate the current batch if it's now empty
        if (currentBatch.sprites.empty())
        {
            currentBatch.isValid = false;
            m_batchKeyToBatchId.erase(currentBatchKey);
            m_freeBatchIds.insert(currentBatchId);
        }

        m_spriteToBatch.erase(spriteId);
        membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
            .item = MembershipPayload{.isValid = false, .batchId = 0},
            .index = spriteId.id
        });

        lowestModifiedBatchId = lowestModifiedBatchId ? std::min(*lowestModifiedBatchId, currentBatchId) : currentBatchId;
    }

    //
    // Update spriteBatchBuffer
    //
    if (lowestModifiedBatchId)
    {
        if (!SyncSpriteBatchPayloads(copyPass, *lowestModifiedBatchId))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessRemovedSprites: Failed to sync sprite batch payload data");
        }
    }

    //
    // Update membershipBuffer
    //
    if (!membershipUpdates.empty())
    {
        if (!m_membershipBuffer.Update("SpriteMembershipUpdate", copyPass, membershipUpdates))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessRemovedSprites: Failed to update membership buffer");
        }
    }

    //
    // Mark our draw calls as invalidated if any sprite in this draw pass was removed
    //
    if (lowestModifiedBatchId || anySpriteRemoved)
    {
        MarkDrawCallsInvalidated();
    }
}

SpriteDrawPass::BatchId SpriteDrawPass::CreateBatchCPUSide(TextureId textureId)
{
    const auto batchKey = GetBatchKey(textureId);

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

    auto spriteBatch = SpriteBatch{
        .batchId = batchId,
        .batchKey = batchKey,
        .isValid = true,
        .textureId = textureId,
        .sprites = {},
        .drawDataOffset = 0
    };

    //
    // Add the batch to the m_batches vector
    //
    if (m_batches.size() < spriteBatch.batchId + 1)
    {
        m_batches.resize(spriteBatch.batchId + 1);
    }

    m_batches.at(spriteBatch.batchId) = spriteBatch;

    m_batchKeyToBatchId.insert_or_assign(batchKey, batchId);

    return batchId;
}

bool SpriteDrawPass::SyncSpriteBatchPayloads(GPU::CopyPass copyPass, BatchId startingBatchId)
{
    //
    // Make sure the batch payload buffer is large enough to hold data for all our batches
    //
    if (!m_spriteBatchBuffer.ResizeAtLeast(copyPass, m_batches.size()))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::ResyncSpriteBatchPayloads: Failed to resize sprite batch buffer");
        return false;
    }

    //
    // Loop through all batches and put together item updates for each batch with an id of at least lowestChangedBatchId
    //
    std::vector<ItemUpdate<SpriteBatchPayload>> batchPayloadUpdates;

    std::size_t drawDataOffset = 0;

    for (BatchId batchId = 0; batchId < m_batches.size(); ++batchId)
    {
        const auto& batch = m_batches.at(batchId);

        if (batchId >= startingBatchId)
        {
            batchPayloadUpdates.push_back(ItemUpdate<SpriteBatchPayload>{
                .item = SpriteBatchPayload{
                    .isValid = batch.isValid,
                    .meshId = m_pGlobal->spriteMeshId.id,
                    .numMembers = (uint32_t)batch.sprites.size(),
                    .drawDataOffset = (uint32_t)drawDataOffset,
                },
                .index = batchId,
            });
        }

        if (batch.isValid)
        {
            drawDataOffset += batch.sprites.size();
        }
    }

    if (!m_spriteBatchBuffer.Update("SpriteBatchDataUpdate", copyPass, batchPayloadUpdates))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::ResyncSpriteBatchPayloads: Failed to update batch buffer");
    }

    //
    // Ensure our draw data buffer is large enough to hold all draw datas for all lod for all batches
    //
    if (!m_drawDataBuffer.ResizeAtLeast(copyPass, drawDataOffset))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::ResyncSpriteBatchPayloads: Failed to resize draw data buffer");
    }

    //
    // Ensure our draw commands buffer is large enough to hold all draw commands for all batches
    //
    if (!m_drawCommandsBuffer.ResizeAtLeast(copyPass, m_batches.size() * MESH_MAX_LOD))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::ResyncSpriteBatchPayloads: Failed to resize draw commands buffer");
    }

    //
    // Ensure our draw counts buffer is large enough to hold all draw counts for all batches
    //
    if (!m_drawCountsBuffer.ResizeAtLeast(copyPass, m_batches.size()))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::ResyncSpriteBatchPayloads: Failed to resize draw count buffer");
    }

    return true;
}

bool SpriteDrawPass::PassesSpriteFilter(const SpriteRenderable& renderable) const
{
    (void)renderable;
    return true;
}

SpriteDrawPass::BatchKey SpriteDrawPass::GetBatchKey(TextureId textureId)
{
    return NCommon::Hash(textureId);
}

void SpriteDrawPass::ComputeDrawCalls(GPU::CommandBufferId commandBufferId)
{
    if (GetNumSprites() == 0)
    {
        return;
    }

    //
    // Write compute commands
    //
    {
        CullInputParamsUniformPayload cullInputParamsPayload{
            .numGroupInstances = (uint32_t)m_pDataStores->sprites.GetInstanceCount()
        };

        const auto viewProjectionPayload = ViewProjectionPayloadFromViewProjection(*GetViewProjection());

        // Fetch pipeline
        GPU::ComputePipelineParams computePipelineParams{
            .shaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("sprite_cull.comp")
        };

        const auto computePipelineId = m_pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
        if (!computePipelineId) { return; }

        const auto computePass = m_pGlobal->pGPU->BeginComputePass(commandBufferId, "SpriteCull");

        m_pGlobal->pGPU->CmdBindPipeline(*computePass, *computePipelineId);

        // ReadWrite storage buffers
        m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_drawDatas", m_drawDataBuffer.GetBufferId());
        m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_batchData", m_spriteBatchBuffer.GetBufferId());

        // Read storage buffers
        m_pGlobal->pGPU->CmdBindStorageReadBuffer(*computePass, "i_spriteInstances", m_pDataStores->sprites.GetInstancePayloadsBuffer());
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
            .shaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("sprite_draw.comp")
        };

        const auto computePipelineId = m_pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
        if (!computePipelineId) { return; }

        const auto computePass = m_pGlobal->pGPU->BeginComputePass(commandBufferId, "SpriteDraw");

        m_pGlobal->pGPU->CmdBindPipeline(*computePass, *computePipelineId);

        // ReadWrite storage buffers
        m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_batchData", m_spriteBatchBuffer.GetBufferId());
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

void SpriteDrawPass::OnRenderSettingsChanged()
{
    // Render distance might have changed, which affects culling, so we need to re-compute draw calls
    MarkDrawCallsInvalidated();
}

std::string SpriteDrawPass::GetTag() const noexcept
{
    return std::format("{}:{}", m_groupName, m_name);
}

std::vector<SpriteDrawPass::RenderBatch> SpriteDrawPass::GetRenderBatches() const
{
    std::vector<SpriteDrawPass::RenderBatch> renderBatches;

    for (const auto& batch : m_batches)
    {
        if (batch.isValid)
        {
            renderBatches.emplace_back(batch.batchId, batch.textureId);
        }
    }

    return renderBatches;
}

/*
void SpriteDrawPass::SyncBatchDataBuffer(GPU::CopyPass copyPass, BatchId lowestChangedBatchId)
{
    //
    // If the batch data buffer isn't large enough to hold data for all of our batches, resize it
    //
    if (m_spriteBatchBuffer.GetItemSize() < m_batches.size())
    {
        if (!m_spriteBatchBuffer.Resize(copyPass, m_batches.size()))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::SyncBatchDataBuffer: Failed to resize batch data buffer");
        }
    }

    //
    // Loop through batches and put together item updates for each batch with an id of at
    // least lowestChangedBatchId
    //
    std::vector<ItemUpdate<SpriteBatchPayload>> batchDataUpdates;

    std::size_t drawDataOffset = 0;

    for (BatchId x = 0; x < m_batches.size(); ++x)
    {
        const auto& batch = m_batches.at(x);

        if (x >= lowestChangedBatchId)
        {
            batchDataUpdates.push_back(ItemUpdate<SpriteBatchPayload>{
                .item = SpriteBatchPayload{
                    .isValid = batch.isValid,
                    .meshId = m_pGlobal->spriteMeshId.id,
                    .drawDataOffset = (uint32_t)drawDataOffset,
                    .coordInstanceIndex = 0
                },
                .index = x,
            });
        }

        if (batch.isValid)
        {
            drawDataOffset += batch.sprites.size();
        }
    }

    if (!m_spriteBatchBuffer.Update("SpriteBatchDataUpdate", copyPass, batchDataUpdates))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::SyncBatchDataBuffer: Failed to update batch data buffer");
    }
}

void SpriteDrawPass::ProcessUpdatedSprites(GPU::CopyPass, const std::vector<SpriteRenderable>& sprites)
{
    if (sprites.empty()) { return; }

    for (const auto& sprite : sprites)
    {
        // If any sprite that belongs to this draw pass is updated, then we mark our draw calls as invalidated
        // so that the cull flow can be run again
        if (m_spriteToBatch.contains(sprite.id))
        {
            MarkDrawCallsInvalidated();
            return;
        }
    }
}

void SpriteDrawPass::ProcessRemovedSprites(GPU::CopyPass copyPass, const std::unordered_set<SpriteId>& spriteIds)
{
    if (spriteIds.empty()) { return; }

    BatchId lowestInvalidatedBatchId{(BatchId)m_batches.size() - 1};

    std::vector<ItemUpdate<MembershipPayload>> membershipUpdates;

    for (const auto& spriteId : spriteIds)
    {
        const auto batchIt = m_spriteToBatch.find(spriteId);
        if (batchIt == m_spriteToBatch.cend())
        {
            continue;
        }

        const BatchId batchId = batchIt->second;
        auto& batch = m_batches.at(batchId);

        batch.sprites.erase(spriteId);

        if (batch.sprites.empty())
        {
            batch.isValid = false;
            m_freeBatchIds.insert(batchId);
        }

        lowestInvalidatedBatchId = std::min(lowestInvalidatedBatchId, batchId);

        m_spriteToBatch.erase(spriteId);

        membershipUpdates.push_back(ItemUpdate<MembershipPayload>{
            .item = MembershipPayload{.isValid = false, .batchId = batchId},
            .index = spriteId.id
        });
    }

    if (membershipUpdates.empty())
    {
        return;
    }

    if (!m_membershipBuffer.Update("SpriteMembershipUpdate", copyPass, membershipUpdates))
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::ProcessRemovedSprites: Failed to update sprite membership buffer");
    }

    SyncBatchDataBuffer(copyPass, lowestInvalidatedBatchId);

    MarkDrawCallsInvalidated();
}

void SpriteDrawPass::ComputeDrawCalls(GPU::CommandBufferId commandBufferId)
{
    if (GetNumSprites() == 0)
    {
        return;
    }

    const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(commandBufferId, std::format("SpriteDrawPass:ComputeDrawCalls"));
    if (!copyPass)
    {
        m_pGlobal->pLogger->Error("SpriteDrawPass::ComputeDrawCalls: Failed to begin copy pass");
        return;
    }

    //
    // Ensure our draw data buffer is large enough to hold entries
    // for every item in this pass
    //
    if (m_drawDataBuffer.GetItemSize() < GetNumSprites())
    {
        if (!m_drawDataBuffer.Resize(*copyPass, GetNumSprites()))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ComputeDrawCalls: Failed to resize draw data buffer");
            return;
        }
    }

    //
    // Ensure our draw commands buffer is large enough to hold entries
    // for every batch in this pass
    //
    if (m_drawCommandsBuffer.GetItemSize() < m_batches.size())
    {
        if (!m_drawCommandsBuffer.Resize(*copyPass, m_batches.size()))
        {
            m_pGlobal->pLogger->Error("SpriteDrawPass::ComputeDrawCalls: Failed to resize draw commands buffer");
            return;
        }
    }

    m_pGlobal->pGPU->EndCopyPass(*copyPass);

    //
    // Write compute commands
    //
    {
        CullInputParamsUniformPayload cullInputParamsPayload{
            .numGroupInstances = (uint32_t)m_pDataStores->sprites.GetInstanceCount()
        };

        const auto viewProjectionPayload = ViewProjectionPayloadFromViewProjection(*GetViewProjection());

        // Fetch pipeline
        GPU::ComputePipelineParams computePipelineParams{
            .shaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("sprite_cull.comp")
        };

        const auto computePipelineId = m_pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
        if (!computePipelineId) { return; }

        const auto computePass = m_pGlobal->pGPU->BeginComputePass(commandBufferId, "SpriteCull");

        m_pGlobal->pGPU->CmdBindPipeline(*computePass, *computePipelineId);

        // ReadWrite storage buffers
        m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_drawDatas", m_drawDataBuffer.GetBufferId());
        m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_batchData", m_spriteBatchBuffer.GetBufferId());

        // Read storage buffers
        m_pGlobal->pGPU->CmdBindStorageReadBuffer(*computePass, "i_spriteInstances", m_pDataStores->sprites.GetInstancePayloadsBuffer());
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
            .shaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("sprite_draw.comp")
        };

        const auto computePipelineId = m_pGlobal->pPipelines->GetOrCreatePipeline(computePipelineParams);
        if (!computePipelineId) { return; }

        const auto computePass = m_pGlobal->pGPU->BeginComputePass(commandBufferId, "SpriteDraw");

        m_pGlobal->pGPU->CmdBindPipeline(*computePass, *computePipelineId);

        // ReadWrite storage buffers
        m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_batchData", m_spriteBatchBuffer.GetBufferId());
        m_pGlobal->pGPU->CmdBindStorageReadWriteBuffer(*computePass, "o_drawCommands", m_drawCommandsBuffer.GetBufferId());

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

void SpriteDrawPass::OnRenderSettingsChanged()
{
    // Render distance might have changed, which affects culling, so we need to re-compute draw calls
    MarkDrawCallsInvalidated();
}

std::string SpriteDrawPass::GetTag() const noexcept
{
    return std::format("{}:{}:Sprite", m_groupName, m_name);
}*/

}
