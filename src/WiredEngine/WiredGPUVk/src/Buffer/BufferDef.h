/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERDEF_H
#define WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERDEF_H

#include "BufferCommon.h"

#include "../VMA.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <cstddef>

namespace Wired::GPU
{
    struct BufferDef
    {
        bool isTransferBuffer{false};
        BufferUsageMode defaultUsageMode{};

        std::size_t byteSize{0};
        VkBufferUsageFlags vkBufferUsageFlags{};

        VmaMemoryUsage vmaMemoryUsage{VMA_MEMORY_USAGE_AUTO};
        VmaAllocationCreateFlags vmaAllocationCreateFlags{};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERDEF_H
