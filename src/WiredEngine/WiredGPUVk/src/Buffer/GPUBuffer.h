/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_BUFFER_GPUBUFFER_H
#define WIREDENGINE_WIREDGPUVK_SRC_BUFFER_GPUBUFFER_H

#include "BufferAllocation.h"
#include "BufferDef.h"
#include "BufferCommon.h"

#include <Wired/GPU/GPUCommon.h>

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    struct GPUBuffer
    {
        VkBuffer vkBuffer{VK_NULL_HANDLE};
        BufferDef bufferDef{};
        BufferAllocation bufferAllocation{};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_BUFFER_GPUBUFFER_H
