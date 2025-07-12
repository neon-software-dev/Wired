/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_COMMON_H
#define WIREDENGINE_WIREDGPUVK_SRC_COMMON_H

#include "Buffer/GPUBuffer.h"
#include "Image/GPUImage.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <optional>

namespace Wired::GPU
{
    static constexpr auto REQUIRED_VULKAN_INSTANCE_VERSION = VK_API_VERSION_1_3;
    static constexpr auto REQUIRED_VULKAN_DEVICE_VERSION = VK_API_VERSION_1_3;

    struct VkBufferBinding
    {
        GPUBuffer gpuBuffer{};
        VkDescriptorType vkDescriptorType{};
        bool shaderWriteable{false};
        std::size_t byteOffset{0};
        std::size_t byteSize{0};
        std::optional<uint32_t> dynamicByteOffset;
    };

    struct VkImageViewSamplerBinding
    {
        GPUImage gpuImage{};
        uint32_t imageViewIndex{0};
        VkSampler vkSampler{VK_NULL_HANDLE};
    };

    struct VkImageViewBinding
    {
        GPUImage gpuImage{};
        uint32_t imageViewIndex{0};
        bool shaderWriteable{false};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_COMMON_H
