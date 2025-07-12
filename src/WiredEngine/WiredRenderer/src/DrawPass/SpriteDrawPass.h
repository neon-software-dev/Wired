/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_SPRITEDRAWPASS_H
#define WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_SPRITEDRAWPASS_H
#include "DrawPass.h"

#include "../ItemBuffer.h"

#include "../Renderer/RendererCommon.h"

#include <Wired/Render/Renderable/SpriteRenderable.h>

#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Wired::Render
{
    struct Global;

    class SpriteDrawPass : public DrawPass
    {
        public:

            struct RenderBatch
            {
                uint32_t batchId{0};
                TextureId textureId;
            };

        public:

            SpriteDrawPass(Global* pGlobal,
                           std::string groupName,
                           std::string name,
                           const DataStores* pDataStores);

            [[nodiscard]] bool StartUp() override;
            void ShutDown() override;

            [[nodiscard]] DrawPassType GetDrawPassType() const noexcept override { return DrawPassType::Sprite; };
            [[nodiscard]] std::string GetTag() const noexcept override;

            void ApplyInitialUpdate(GPU::CopyPass copyPass) override;
            void ApplyStateUpdate(GPU::CopyPass copyPass, const StateUpdate& stateUpdate) override;
            void ComputeDrawCalls(GPU::CommandBufferId commandBufferId) override;

            void OnRenderSettingsChanged() override;

            [[nodiscard]] std::string GetName() const noexcept { return m_name; }
            [[nodiscard]] std::size_t GetNumSprites() const noexcept { return m_spriteToBatch.size(); }
            [[nodiscard]] std::vector<RenderBatch> GetRenderBatches() const;

            [[nodiscard]] GPU::BufferId GetDrawDataBuffer() const { return m_drawDataBuffer.GetBufferId(); }
            [[nodiscard]] GPU::BufferId GetDrawCommandsBuffer() const { return m_drawCommandsBuffer.GetBufferId(); }
            [[nodiscard]] GPU::BufferId GetDrawCountsBuffer() const { return m_drawCountsBuffer.GetBufferId(); }

        private:

            using BatchId = uint32_t;
            using BatchKey = std::size_t;

            struct SpriteBatch
            {
                uint32_t batchId{0};
                BatchKey batchKey{0};
                bool isValid{false};
                TextureId textureId;
                std::unordered_set<SpriteId> sprites;
                uint32_t drawDataOffset{0};
            };

        private:

            void ProcessAddedSprites(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& sprites);
            void ProcessUpdatedSprites(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& sprites);
            void ProcessRemovedSprites(GPU::CopyPass copyPass, const std::unordered_set<SpriteId>& spriteIds);

            [[nodiscard]] BatchId CreateBatchCPUSide(TextureId textureId);

            [[nodiscard]] bool PassesSpriteFilter(const SpriteRenderable& renderable) const;

            [[nodiscard]] static BatchKey GetBatchKey(TextureId textureId);

            [[nodiscard]] bool SyncSpriteBatchPayloads(GPU::CopyPass copyPass, BatchId startingBatchId);

        private:

            std::string m_name;

            std::vector<SpriteBatch> m_batches;
            std::unordered_set<BatchId> m_freeBatchIds;
            std::unordered_map<BatchKey, BatchId> m_batchKeyToBatchId;
            ItemBuffer<SpriteBatchPayload> m_spriteBatchBuffer;

            std::unordered_map<SpriteId, BatchId> m_spriteToBatch;
            ItemBuffer<MembershipPayload> m_membershipBuffer;

            ItemBuffer<DrawDataPayload> m_drawDataBuffer;
            ItemBuffer<GPU::IndirectDrawCommand> m_drawCommandsBuffer;
            ItemBuffer<DrawCountPayload> m_drawCountsBuffer;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_SPRITEDRAWPASS_H
