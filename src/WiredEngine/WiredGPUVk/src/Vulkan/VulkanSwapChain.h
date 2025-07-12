/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSWAPCHAIN_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSWAPCHAIN_H

#include "VulkanPhysicalDevice.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"

#include <Wired/GPU/GPUId.h>

#include <vulkan/vulkan.h>

#include <expected>
#include <optional>
#include <vector>

namespace Wired::GPU
{
    struct Global;

    struct SwapChainConfig
    {
        SwapChainConfig() = default;

        SwapChainConfig(VkSurfaceFormatKHR _surfaceFormat,
                        VkPresentModeKHR _presentMode,
                        VkExtent2D _extent,
                        VkSurfaceTransformFlagBitsKHR _preTransform)
            : surfaceFormat(_surfaceFormat)
            , presentMode(_presentMode)
            , extent(_extent)
            , preTransform(_preTransform)
        { }

        VkSurfaceFormatKHR surfaceFormat{};           // The format of the swap chain images
        VkPresentModeKHR presentMode{};               // The current present mode
        VkExtent2D extent{};                          // The extent of the swap chain images
        VkSurfaceTransformFlagBitsKHR preTransform{}; // Surface pre-transform settings
    };

    class VulkanSwapChain
    {
        public:

            [[nodiscard]] static std::expected<VulkanSwapChain, bool> Create(Global* pGlobal);

        public:

            VulkanSwapChain() = default;
            VulkanSwapChain(Global* pGlobal,
                            VkSwapchainKHR vkSwapChain,
                            const SwapChainConfig& swapChainConfig,
                            std::vector<ImageId> imageIds);
            ~VulkanSwapChain();

            void Destroy(bool isShutDown);

            [[nodiscard]] VkSwapchainKHR GetVkSwapChain() const noexcept { return m_vkSwapChain; }
            [[nodiscard]] SwapChainConfig GetSwapChainConfig() const noexcept { return m_config; }
            [[nodiscard]] ImageId GetImageId(uint32_t index) const noexcept { return m_imageIds.at(index); }
            [[nodiscard]] std::size_t GetImageCount() const noexcept { return m_imageIds.size(); }

        private:

            Global* m_pGlobal{nullptr};
            VkSwapchainKHR m_vkSwapChain{VK_NULL_HANDLE};
            SwapChainConfig m_config{};
            std::vector<ImageId> m_imageIds;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSWAPCHAIN_H
