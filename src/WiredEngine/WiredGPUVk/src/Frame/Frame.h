/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_FRAME_FRAME_H
#define WIREDENGINE_WIREDGPUVK_SRC_FRAME_FRAME_H

#include <Wired/GPU/GPUId.h>

#include "../Timestamps.h"

#include "../State/CommandBuffer.h"

#include <NEON/Common/Hash.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <unordered_map>
#include <string>
#include <optional>
#include <thread>
#include <cassert>

namespace Wired::GPU
{
    struct Global;

    enum class FrameState
    {
        NotStarted,
        Started,
        Finished
    };

    struct ImGuiImageReference
    {
        ImageId imageId{};

        VkImage vkImage{VK_NULL_HANDLE};
        VkImageView vkImageView{VK_NULL_HANDLE};
        VkSampler vkSampler{VK_NULL_HANDLE};
        VkDescriptorSet vkDescriptorSet{VK_NULL_HANDLE};

        auto operator<=>(const ImGuiImageReference&) const = default;

        struct HashFunction
        {
            std::size_t operator()(const ImGuiImageReference& o) const
            {
                return NCommon::Hash(o.vkImage, o.vkImageView, o.vkSampler, o.vkDescriptorSet);
            }
        };
    };

    struct Frame
    {
        public:

            Frame(Global* pGlobal, uint32_t frameIndex);

            [[nodiscard]] bool Create();
            void Destroy();

            [[nodiscard]] uint32_t GetFrameIndex() const noexcept { return m_frameIndex; }

            [[nodiscard]] FrameState GetFrameState() const noexcept { return m_frameState; }
            [[nodiscard]] bool IsInState(FrameState frameState) const noexcept { return m_frameState == frameState; }
            [[nodiscard]] bool IsActiveState() const noexcept { return m_frameState != FrameState::NotStarted && m_frameState != FrameState::Finished; }
            void SetFrameState(FrameState frameState) { m_frameState = frameState; }

            [[nodiscard]] VkSemaphore GetSwapChainImageAvailableSemaphore() const noexcept { return m_swapChainImageAvailableSemaphore; }
            [[nodiscard]] VkSemaphore GetPresentWorkFinishedSemaphore() const noexcept { return m_presentWorkFinishedSemaphore; }

            void SetSwapChainPresentIndex(uint32_t swapChainImageIndex);
            void ResetSwapChainPresentIndex();
            [[nodiscard]] uint32_t GetSwapChainPresentIndex() const noexcept { assert(m_swapChainPresentIndex); return *m_swapChainPresentIndex; }

            void AssociateCommandBuffer(CommandBufferId commandBufferId);
            void UnAssociateCommandBuffer(CommandBufferId commandBufferId);
            [[nodiscard]] std::unordered_set<CommandBufferId> GetAssociatedCommandBuffers() const noexcept { return m_associatedCommandBufferIds; }
            void ClearAssociatedCommandBuffers();

            [[nodiscard]] std::optional<Timestamps*> GetTimestamps() const;

            [[nodiscard]] std::optional<uint64_t > CreateImGuiImageReference(ImageId imageId, SamplerId samplerId);
            [[nodiscard]] const std::unordered_set<ImGuiImageReference, ImGuiImageReference::HashFunction>&
                GetImGuiImageReferences() const noexcept { return m_imGuiImageReferences; }
            void ClearImGuiImageReferences();

        private:

            Global* m_pGlobal;
            uint32_t m_frameIndex;
            FrameState m_frameState{FrameState::NotStarted};

            //
            // Persistent resources
            //

            // Semaphore triggered when the frame's swap chain image has become available (persistent)
            VkSemaphore m_swapChainImageAvailableSemaphore{VK_NULL_HANDLE};

            // Semaphore triggered when the frame's present command buffer work has finished (persistent)
            VkSemaphore m_presentWorkFinishedSemaphore{VK_NULL_HANDLE};

            std::optional<std::unique_ptr<Timestamps>> m_timestamps;

            //
            // Runtime state
            //

            std::optional<uint32_t> m_swapChainPresentIndex;

            std::unordered_set<CommandBufferId> m_associatedCommandBufferIds;

            std::unordered_set<ImGuiImageReference, ImGuiImageReference::HashFunction> m_imGuiImageReferencesIncoming;
            std::unordered_set<ImGuiImageReference, ImGuiImageReference::HashFunction> m_imGuiImageReferences;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_FRAME_FRAME_H
