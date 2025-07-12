/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_UTIL_SYNCPRIMITIVES_H
#define WIREDENGINE_WIREDGPUVK_SRC_UTIL_SYNCPRIMITIVES_H

#include <vulkan/vulkan.h>

#include <vector>

namespace Wired::GPU
{
    //
    // Barriers
    //
    struct ImageBarrier
    {
        VkImage vkImage{};
        VkImageSubresourceRange subresourceRange{};
        VkPipelineStageFlags2 srcStageMask{};
        VkAccessFlags2 srcAccessMask{};
        VkPipelineStageFlags2 dstStageMask{};
        VkAccessFlags2 dstAccessMask{};
        VkImageLayout oldLayout{};
        VkImageLayout newLayout{};
        uint32_t srcQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};
        uint32_t dstQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};
    };

    struct BufferBarrier
    {
        VkBuffer vkBuffer{};
        std::size_t byteOffset{0};
        std::size_t byteSize{0};
        VkPipelineStageFlags2 srcStageMask{};
        VkAccessFlags2 srcAccessMask{};
        VkPipelineStageFlags2 dstStageMask{};
        VkAccessFlags2 dstAccessMask{};
        uint32_t srcQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};
        uint32_t dstQueueFamilyIndex{VK_QUEUE_FAMILY_IGNORED};
    };

    struct Barrier
    {
        std::vector<ImageBarrier> imageBarriers;
        std::vector<BufferBarrier> bufferBarriers;
    };

    //
    // Semaphores
    //
    struct SemaphoreOp
    {
        SemaphoreOp(VkSemaphore _semaphore, VkPipelineStageFlags2 _stageMask);

        VkSemaphore semaphore{VK_NULL_HANDLE};
        VkPipelineStageFlags2 stageMask{0};
    };

    struct WaitOn
    {
        explicit WaitOn(std::vector<SemaphoreOp> _semaphores);

        static WaitOn None() { return WaitOn({}); }

        std::vector<SemaphoreOp> semaphores;
    };

    struct SignalOn
    {
        explicit SignalOn(std::vector<SemaphoreOp> _semaphores);

        static SignalOn None() { return SignalOn({}); }

        std::vector<SemaphoreOp> semaphores;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_UTIL_SYNCPRIMITIVES_H
