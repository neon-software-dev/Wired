/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_WIREDGPUVK_H
#define WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_WIREDGPUVK_H

#include "Wired/GPU/WiredGPU.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <utility>
#include <expected>

namespace Wired::GPU
{
    struct WiredGPUVkInput
    {
        std::string applicationName;
        std::tuple<uint32_t, uint32_t, uint32_t> applicationVersion;
        std::vector<std::string> requiredInstanceExtensions;
        bool supportSurfaceOutput;
        PFN_vkGetInstanceProcAddr pfnVkGetInstanceProcAddr;
    };

    class WiredGPUVk : public WiredGPU
    {
        public:

            ~WiredGPUVk() override = default;

            [[nodiscard]] virtual VkInstance GetVkInstance() const = 0;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_WIREDGPUVK_H
