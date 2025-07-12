/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGES_H
#define WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGES_H

#include "ImageDef.h"
#include "ImageViewDef.h"
#include "GPUImage.h"
#include "ImageCommon.h"

#include "../Common.h"
#include "../VMA.h"

#include <Wired/GPU/GPUCommon.h>
#include <Wired/GPU/GPUId.h>

#include <NEON/Common/ImageData.h>
#include <NEON/Common/Space/Size2D.h>

#include <vulkan/vulkan.h>

#include <expected>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <mutex>

namespace Wired::GPU
{
    struct Global;
    class CommandBuffer;

    class Images
    {
        public:

            explicit Images(Global* pGlobal);
            ~Images();

            void Destroy();
            void RunCleanUp();

            [[nodiscard]] std::expected<ImageId, bool> CreateFromParams(
                CommandBuffer* pCommandBuffer,
                const ImageCreateParams& params,
                const std::string& tag
            );

            [[nodiscard]] std::expected<ImageId, bool> CreateFromSwapChainImage(
                uint32_t swapChainImageIndex,
                VkImage vkImage,
                VkSwapchainCreateInfoKHR vkSwapchainCreateInfo
            );

            //

            [[nodiscard]] std::expected<ImageId, bool> CreateImage(
                CommandBuffer* pCommandBuffer,
                const ImageDef& imageDef,
                const ImageUsageMode& defaultUsageMode,
                const std::vector<ImageViewDef>& imageViewDefs,
                const std::string& tag
            );

            [[nodiscard]] std::optional<GPUImage> GetImage(ImageId imageId, bool cycled, std::optional<CommandBuffer*> commandBuffer = std::nullopt);

            void DestroyImage(ImageId imageId, bool destroyImmediately);

            bool BarrierImageRangeForUsage(CommandBuffer* pCommandBuffer, const GPUImage& gpuImage, const VkImageSubresourceRange& vkImageSubresourceRange, ImageUsageMode destUsageMode);
            bool BarrierImageRangeToDefaultUsage(CommandBuffer* pCommandBuffer, const GPUImage& gpuImage, const VkImageSubresourceRange& vkImageSubresourceRange, ImageUsageMode sourceUsageMode);

            bool BarrierWholeImageForUsage(CommandBuffer* pCommandBuffer, const GPUImage& gpuImage, ImageUsageMode destUsageMode);
            bool BarrierWholeImageToDefaultUsage(CommandBuffer* pCommandBuffer, const GPUImage& gpuImage, ImageUsageMode sourceUsageMode);

            [[nodiscard]] static VkImageAspectFlags GetImageAspectFlags(const GPUImage& gpuImage);

        private:

            struct Image
            {
                ImageId id{};
                bool isSwapChainImage{false};
                std::string tag;

                uint32_t activeImageIndex{0};
                std::vector<GPUImage> gpuImages;
            };

        private:

            [[nodiscard]] std::expected<GPUImage, bool> CreateGPUImage(CommandBuffer* pCommandBuffer,
                                                                       const ImageDef& imageDef,
                                                                       const ImageUsageMode& defaultUsageMode,
                                                                       const std::vector<ImageViewDef>& imageViewDefs,
                                                                       const std::string& tag);

            [[nodiscard]] bool CycleImageIfNeeded(CommandBuffer* pCommandBuffer, Image& image);

            void CleanUp_DeletedImages();
            void CleanUp_UnusedImages();

        private:

            [[nodiscard]] VkImageCreateInfo GetVkImageCreateInfo(const ImageDef& imageDef);
            [[nodiscard]] static VmaAllocationCreateInfo GetVmaAllocationCreateInfo(const ImageDef& imageDef);
            [[nodiscard]] static VkImageViewCreateInfo GetVkImageViewCreateInfo(VkImage vkImage, const ImageViewDef& imageViewDef);

            [[nodiscard]] bool CreateVkImage(GPUImage& gpuImage,
                                             const ImageDef& imageDef,
                                             const VmaAllocationCreateInfo& vmaAllocationCreateInfo,
                                             const std::string& tag);

            [[nodiscard]] bool CreateVkImageView(GPUImage& gpuImage,
                                                 const ImageViewDef& imageViewDef,
                                                 const std::string& imageTag,
                                                 const std::string& imageViewTag);

            void DestroyImageObjects(const Image& image);
            void DestroyGPUImageObjects(const GPUImage& gpuImage, bool isSwapChainImage);

            [[nodiscard]] static VkImageSubresourceRange GetWholeImageSubresourceRange(const GPUImage& gpuImage);

        private:

            Global* m_pGlobal;

            std::unordered_map<ImageId, Image> m_images;
            std::unordered_set<ImageId> m_imagesMarkedForDeletion;
            mutable std::recursive_mutex m_imagesMutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGES_H
