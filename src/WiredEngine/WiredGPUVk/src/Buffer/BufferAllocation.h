/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERALLOCATION_H
#define WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERALLOCATION_H

#include "../VMA.h"

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    struct BufferAllocation
    {
        VmaAllocationCreateInfo vmaAllocationCreateInfo{};
        VmaAllocation vmaAllocation{nullptr};
        VmaAllocationInfo vmaAllocationInfo{};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_BUFFER_BUFFERALLOCATION_H
