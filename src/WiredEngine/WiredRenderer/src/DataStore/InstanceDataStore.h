/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_INSTANCEDATASTORE_H
#define WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_INSTANCEDATASTORE_H

#include "../ItemBuffer.h"
#include "../Global.h"

#include <Wired/Render/Id.h>
#include <Wired/Render/StateUpdate.h>
#include "Wired/GPU/WiredGPU.h"

#include <NEON/Common/Log/ILogger.h>

#include <vector>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Render
{
    template <typename RenderableType, typename PayloadType>
    class InstanceDataStore
    {
        public:

            struct InstanceData
            {
                bool isValid{false};
                RenderableType instance{};
            };

        public:

            explicit InstanceDataStore(Global* pGlobal)
                : m_pGlobal(pGlobal)
            { }

            [[nodiscard]] virtual bool StartUp();
            virtual void ShutDown();

            void ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate);

            [[nodiscard]] std::size_t GetInstanceCount() const noexcept;
            [[nodiscard]] GPU::BufferId GetInstancePayloadsBuffer() const noexcept { return m_instancePayloadsBuffer.GetBufferId(); }

            [[nodiscard]] const std::vector<InstanceData>& GetInstances() const noexcept { return m_instances; }

        protected:

            [[nodiscard]] virtual std::string GetTag() const noexcept = 0;

            virtual void ApplyStateUpdateInternal(GPU::CopyPass copyPass, const StateUpdate& stateUpdate) = 0;

            [[nodiscard]] virtual std::expected<PayloadType, bool> PayloadFrom(const RenderableType& renderableType) const = 0;

            void AddOrUpdate(GPU::CopyPass copyPass, const std::vector<RenderableType>& instances);
            void Remove(GPU::CopyPass copyPass, const std::vector<RenderableId>& ids);

        protected:

            Global* m_pGlobal;

        private:

            void AddOrUpdateInstancePayloadsBuffer(GPU::CopyPass copyPass, const std::vector<PayloadType>& payloads);

        private:

            ItemBuffer<PayloadType> m_instancePayloadsBuffer;

            std::vector<InstanceData> m_instances;
    };

    template <typename RenderableType, typename PayloadType>
    bool InstanceDataStore<RenderableType, PayloadType>::StartUp()
    {
        if (!m_instancePayloadsBuffer.Create(m_pGlobal,
                                             {GPU::BufferUsageFlag::GraphicsStorageRead},
                                             64,
                                             false, // TODO Perf: Dedicated?
                                             GetTag()))
        {
            return false;
        }

        return true;
    }

    template <typename RenderableType, typename PayloadType>
    void InstanceDataStore<RenderableType, PayloadType>::ShutDown()
    {
        m_instancePayloadsBuffer.Destroy();
    }

    template <typename RenderableType, typename PayloadType>
    void InstanceDataStore<RenderableType, PayloadType>::ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate)
    {
        const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(commandBufferId, std::format("InstanceStateUpdate-{}", GetTag()));
        if (!copyPass)
        {
            m_pGlobal->pLogger->Error("InstanceDataStore::ApplyStateUpdate: Failed to begin copy pass");
            return;
        }

        //
        // Apply state updates
        //
        ApplyStateUpdateInternal(*copyPass, stateUpdate);

        //
        // Finish
        //
        m_pGlobal->pGPU->EndCopyPass(*copyPass);
    }

    template <typename RenderableType, typename PayloadType>
    std::size_t InstanceDataStore<RenderableType, PayloadType>::GetInstanceCount() const noexcept
    {
        const auto itemSize = m_instancePayloadsBuffer.GetItemSize();

        // Any item at index 0 is the default/invalid renderable id, ignore it
        if (itemSize <= 1) { return 0; }

        return itemSize - 1;
    }

    template <typename RenderableType, typename PayloadType>
    void InstanceDataStore<RenderableType, PayloadType>::AddOrUpdate(GPU::CopyPass copyPass, const std::vector<RenderableType>& instances)
    {
        if (instances.empty()) { return; }

        NCommon::IdTypeIntegral highestId{};

        std::vector<PayloadType> payloads;

        for (const auto& instance : instances)
        {
            auto payload = PayloadFrom(instance);
            if (!payload) { continue; }

            highestId = std::max(highestId, payload->id);

            payloads.push_back(std::move(*payload));
        }

        AddOrUpdateInstancePayloadsBuffer(copyPass, payloads);

        //
        // Upload local state with new data
        //
        if (m_instances.size() < highestId + 1)
        {
            m_instances.resize(highestId + 1);
        }

        for (const auto& instance : instances)
        {
            auto& instanceData = m_instances.at(instance.id.id);
            instanceData.isValid = true;
            instanceData.instance = instance;
        }
    }

    template <typename RenderableType, typename PayloadType>
    void InstanceDataStore<RenderableType, PayloadType>::AddOrUpdateInstancePayloadsBuffer(GPU::CopyPass copyPass, const std::vector<PayloadType>& payloads)
    {
        if (payloads.empty()) { return; }

        std::vector<ItemUpdate<PayloadType>> updates;
        updates.reserve(payloads.size());

        NCommon::IdTypeIntegral highestId{};

        for (const auto& payload : payloads)
        {
            updates.push_back(ItemUpdate<PayloadType>{
                .item = payload,
                .index = payload.id
            });

            highestId = std::max(highestId, payload.id);
        }

        // Sort the items by index so that ItemBuffer can efficiently batch neighboring updates together
        std::ranges::sort(updates, [](const ItemUpdate<PayloadType>& a, const ItemUpdate<PayloadType>& b){
            return a.index < b.index;
        });

        //
        // Update GPU buffer with new data
        //
        const auto currentInstancesSize = m_instancePayloadsBuffer.GetItemSize();
        if (currentInstancesSize < highestId + 1)
        {
            if (!m_instancePayloadsBuffer.Resize(copyPass, highestId + 1))
            {
                m_pGlobal->pLogger->Error("InstanceDataStore::AddOrUpdate: Failed to resize instances buffer for: {}", GetTag());
                return;
            }
        }

        if (!m_instancePayloadsBuffer.Update(std::format("InstancesSync:{}", GetTag()), copyPass, updates))
        {
            m_pGlobal->pLogger->Error("InstanceDataStore::AddOrUpdate: Failed to update instances buffer for: {}", GetTag());
            return;
        }
    }

    template <typename RenderableType, typename PayloadType>
    void InstanceDataStore<RenderableType, PayloadType>::Remove(GPU::CopyPass copyPass, const std::vector<RenderableId>& ids)
    {
        std::vector<PayloadType> removePayloads;

        for (const auto& id : ids)
        {
            PayloadType removePayload{};
            removePayload.isValid = false;
            removePayload.id = id.id;

            removePayloads.push_back(removePayload);

            m_instances.at(id.id).isValid = false;
        }

        AddOrUpdateInstancePayloadsBuffer(copyPass, removePayloads);
    }
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_INSTANCEDATASTORE_H
