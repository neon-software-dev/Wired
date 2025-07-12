/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_OBJECTDRAWPASS_H
#define WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_OBJECTDRAWPASS_H

#include "DrawPass.h"

#include "../ItemBuffer.h"

#include "../Renderer/RendererCommon.h"

#include <Wired/Render/Renderable/ObjectRenderable.h>

#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Wired::Render
{
    struct Global;

    class ObjectDrawPass : public DrawPass
    {
        public:

            struct RenderBatch
            {
                uint32_t batchId{0};
                MaterialId materialId;
                MeshId meshId;
            };

        public:

            ObjectDrawPass(Global* pGlobal,
                           std::string groupName,
                           std::string name,
                           const DataStores* pDataStores,
                           ObjectDrawPassType objectDrawPassType);

            [[nodiscard]] bool StartUp() override;
            void ShutDown() override;

            [[nodiscard]] DrawPassType GetDrawPassType() const noexcept override { return DrawPassType::Object; };
            [[nodiscard]] std::string GetTag() const noexcept override;

            void ApplyInitialUpdate(GPU::CopyPass copyPass) override;
            void ApplyStateUpdate(GPU::CopyPass copyPass, const StateUpdate& stateUpdate) override;
            void ComputeDrawCalls(GPU::CommandBufferId commandBufferId) override;

            void OnRenderSettingsChanged() override;

            [[nodiscard]] std::string GetName() const noexcept { return m_name; }
            [[nodiscard]] ObjectDrawPassType GetObjectDrawPassType() const noexcept { return m_objectDrawPassType; };
            [[nodiscard]] std::size_t GetNumObjects() const noexcept { return m_objectToBatch.size(); }
            [[nodiscard]] std::vector<RenderBatch> GetRenderBatches() const;

            [[nodiscard]] GPU::BufferId GetDrawDataBuffer() const { return m_drawDataBuffer.GetBufferId(); }
            [[nodiscard]] GPU::BufferId GetDrawCommandsBuffer() const { return m_drawCommandsBuffer.GetBufferId(); }
            [[nodiscard]] GPU::BufferId GetDrawCountsBuffer() const { return m_drawCountsBuffer.GetBufferId(); }

        private:

            using BatchId = uint32_t;
            using BatchKey = std::size_t;

            struct ObjectBatch
            {
                uint32_t batchId{0};
                BatchKey batchKey{0};
                bool isValid{false};
                MaterialId materialId;
                MeshId meshId;
                std::unordered_set<ObjectId> objects;
                uint32_t drawDataOffset{0};
            };

        private:

            void ProcessAddedObjects(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objects);
            void ProcessUpdatedObjects(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objects);
            void ProcessRemovedObjects(GPU::CopyPass copyPass, const std::unordered_set<ObjectId>& objects);

            [[nodiscard]] BatchId CreateBatchCPUSide(MaterialId materialId, MeshId meshId);

            [[nodiscard]] bool PassesObjectFilter(const ObjectRenderable& renderable) const;

            [[nodiscard]] static BatchKey GetBatchKey(MaterialId materialId, MeshId meshId);

            [[nodiscard]] bool SyncObjectBatchPayloads(GPU::CopyPass copyPass, BatchId startingBatchId);

        private:

            ObjectDrawPassType m_objectDrawPassType;

            std::string m_name;

            std::vector<ObjectBatch> m_batches;
            std::unordered_set<BatchId> m_freeBatchIds;
            std::unordered_map<BatchKey, BatchId> m_batchKeyToBatchId;
            ItemBuffer<ObjectBatchPayload> m_objectBatchBuffer;

            std::unordered_map<ObjectId, BatchId> m_objectToBatch;
            ItemBuffer<MembershipPayload> m_membershipBuffer;

            ItemBuffer<DrawDataPayload> m_drawDataBuffer;
            ItemBuffer<GPU::IndirectDrawCommand> m_drawCommandsBuffer;
            ItemBuffer<DrawCountPayload> m_drawCountsBuffer;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_OBJECTDRAWPASS_H
