/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_ITEMBUFFER_H
#define WIREDENGINE_WIREDRENDERER_SRC_ITEMBUFFER_H

#include "GPUBuffer.h"

#include <format>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace Wired::Render
{
    struct Global;

    template <typename T>
    struct ItemUpdate
    {
        T item{};
        std::size_t index{0};
    };

    /**
     * Item-based vector-like wrapper around a GPU storage buffer. Respects shader
     * alignment requirements across items. It's up to the user to maintain proper
     * alignment requirements within each item's bytes.
     */
    template <typename T>
    class ItemBuffer
    {
        public:

            [[nodiscard]] bool Create(Global* pGlobal,
                                      const GPU::BufferUsageFlags& usage,
                                      std::size_t itemCapacity,
                                      bool dedicatedMemory,
                                      std::string_view userTag);

            void Destroy();

            [[nodiscard]] bool PushBack(const std::string& transferKey,
                                        GPU::CopyPass copyPass,
                                        const std::vector<T>& items);

            [[nodiscard]] bool Update(const std::string& transferKey,
                                      GPU::CopyPass copyPass,
                                      const std::vector<ItemUpdate<T>>& updates);

            [[nodiscard]] bool Resize(GPU::CopyPass copyPass, const std::size_t& itemCount);
            [[nodiscard]] bool ResizeAtLeast(GPU::CopyPass copyPass, const std::size_t& itemCount);

            [[nodiscard]] bool Reserve(GPU::CopyPass copyPass, const std::size_t& itemCount);

            [[nodiscard]] GPU::BufferId GetBufferId() const noexcept { return m_dataBuffer.GetBufferId(); }
            [[nodiscard]] std::size_t GetItemSize() const noexcept { return m_itemSize; }
            [[nodiscard]] std::size_t GetItemCapacity() const noexcept { return m_dataBuffer.GetByteSize() / sizeof(T); }

        private:

            [[nodiscard]] bool ChangeCapacity(const std::optional<GPU::CopyPass>& copyPass, const std::size_t& itemCount);

        private:

            GPUBuffer m_dataBuffer;

            std::size_t m_itemSize{0};
    };

    template <typename T>
    bool ItemBuffer<T>::Create(Global* pGlobal,
                               const GPU::BufferUsageFlags& usage,
                               std::size_t requestedItemCapacity,
                               bool dedicatedMemory,
                               std::string_view userTag)
    {
        const auto itemCapacity = std::max(requestedItemCapacity, (std::size_t)1U);
        const auto byteCapacity = itemCapacity * sizeof(T);

        return m_dataBuffer.Create(pGlobal,
                                   usage,
                                   byteCapacity,
                                   dedicatedMemory,
                                   std::format("Item:{}", userTag));
    }

    template <typename T>
    void ItemBuffer<T>::Destroy()
    {
        m_dataBuffer.Destroy();
        m_itemSize = 0;
    }

    template <typename T>
    bool ItemBuffer<T>::PushBack(const std::string& transferKey,
                                 GPU::CopyPass copyPass,
                                 const std::vector<T>& items)
    {
        if (items.empty()) { return true; }

        const auto newItemSize = m_itemSize + items.size();

        if (newItemSize > GetItemCapacity())
        {
            if (!ChangeCapacity(copyPass, newItemSize * 2))
            {
                return false;
            }
        }

        const auto dataUpdate = GPUBuffer::DataUpdate{
            .data = {
                .pData = items.data(),
                .byteSize = items.size() * sizeof(T)
            },
            .destByteOffset = m_itemSize * sizeof(T)
        };

        if (!m_dataBuffer.Update(copyPass, transferKey, {dataUpdate}))
        {
            return false;
        }

        m_itemSize = newItemSize;

        return true;
    }

    template <typename T>
    struct ItemGroup
    {
        std::size_t index{0};
        std::vector<T> items;
    };

    template <typename T>
    std::vector<ItemGroup<T>> GroupUp(const std::vector<ItemUpdate<T>>& updates)
    {
        std::vector<ItemGroup<T>> groups;
        std::optional<std::size_t> currentIndex;

        for (const auto& update : updates)
        {
            if (!currentIndex || ((update.index - *currentIndex) != 1))
            {
                currentIndex = update.index;
                groups.push_back(ItemGroup<T>{.index = update.index, .items = {update.item}});
                continue;
            }

            currentIndex = update.index;
            groups.back().items.push_back(update.item);
        }

        return std::move(groups);
    }

    template <typename T>
    bool ItemBuffer<T>::Update(const std::string& transferKey,
                               GPU::CopyPass copyPass,
                               const std::vector<ItemUpdate<T>>& updates)
    {
        if (updates.empty()) { return true; }

        const auto groups = GroupUp(updates);

        std::vector<GPUBuffer::DataUpdate> dataUpdates;
        dataUpdates.reserve(groups.size());

        for (const auto& group : groups)
        {
            dataUpdates.push_back(GPUBuffer::DataUpdate{
                .data = {
                    .pData = group.items.data(),
                    .byteSize = group.items.size() * sizeof(T)
                },
                .destByteOffset = (group.index * sizeof(T))}
            );
        }

        return m_dataBuffer.Update(copyPass, transferKey, dataUpdates);
    }

    template<typename T>
    bool ItemBuffer<T>::Resize(GPU::CopyPass copyPass, const std::size_t& itemCount)
    {
        if (m_itemSize == itemCount)
        {
            return true;
        }
        else if (m_itemSize > itemCount)
        {
            m_itemSize = itemCount;

            if (m_itemSize < GetItemCapacity() / 4)
            {
                (void)ChangeCapacity(copyPass, GetItemCapacity() / 2);
            }

            return true;
        }
        else
        {
            if (!ChangeCapacity(copyPass, itemCount * 2))
            {
                return false;
            }

            m_itemSize = itemCount;

            return true;
        }
    }

    template<typename T>
    bool ItemBuffer<T>::ResizeAtLeast(GPU::CopyPass copyPass, const std::size_t& itemCount)
    {
        if (GetItemSize() < itemCount)
        {
            return Resize(copyPass, itemCount);
        }

        return true;
    }

    template<typename T>
    bool ItemBuffer<T>::Reserve(GPU::CopyPass copyPass, const std::size_t& itemCount)
    {
        if (GetItemCapacity() >= itemCount) { return true; }

        const auto newBufferByteSize = itemCount * sizeof(T);

        return ChangeCapacity(copyPass, newBufferByteSize);
    }

    template<typename T>
    bool ItemBuffer<T>::ChangeCapacity(const std::optional<GPU::CopyPass>& copyPass, const std::size_t& itemCount)
    {
        // Enforce a minimum of 64 bytes of capacity, don't allow zero or ridiculously low capacities
        const auto newBufferByteSize = std::max(itemCount * sizeof(T), (std::size_t)64U);

        if (copyPass)
        {
            return m_dataBuffer.ResizeRetaining(*copyPass, newBufferByteSize);
        }
        else
        {
            return m_dataBuffer.ResizeDiscarding(newBufferByteSize);
        }
    }
}


#endif //WIREDENGINE_WIREDRENDERER_SRC_ITEMBUFFER_H
