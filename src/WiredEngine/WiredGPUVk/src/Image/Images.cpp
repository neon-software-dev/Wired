/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Images.h"

#include "../Global.h"
#include "../Usages.h"

#include "../State/CommandBuffer.h"
#include "../Vulkan/VulkanDebugUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <format>
#include <algorithm>

namespace Wired::GPU
{

Images::Images(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Images::~Images()
{
    m_pGlobal = nullptr;
}

void Images::Destroy()
{
    m_pGlobal->pLogger->Info("Images: Destroying");

    while (!m_images.empty())
    {
        DestroyImage(m_images.cbegin()->first, true);
    }
}

std::expected<ImageId, bool> Images::CreateFromParams(CommandBuffer* pCommandBuffer,
                                                      const ImageCreateParams& params,
                                                      const std::string& tag)
{
    //
    // Validation
    //
    if (params.imageType == ImageType::ImageCube && params.numLayers < 6)
    {
        m_pGlobal->pLogger->Error("Images::CreateFromParams: Cubic images must have >= 6 layers: {}", tag);
        return std::unexpected(false);
    }

    if ((params.imageType == ImageType::Image2D ||
         params.imageType == ImageType::Image2DArray ||
         params.imageType == ImageType::ImageCube) &&
        params.size.d != 1)
    {
        m_pGlobal->pLogger->Error("Images::CreateFromParams: Non-3D images must have a depth of 1: {}", tag);
        return std::unexpected(false);
    }

    //
    // Create ImageDef
    //
    VkImageType vkImageType{};

    switch (params.imageType)
    {
        case ImageType::Image2D:
        case ImageType::Image2DArray:
        case ImageType::ImageCube:
            vkImageType = VK_IMAGE_TYPE_2D;
        break;
        case ImageType::Image3D:
            vkImageType = VK_IMAGE_TYPE_3D;
        break;
    }

    //
    // Determine default usage mode
    //
    ImageUsageMode defaultUsageMode{};

    // Order matters here, note: else-ifs
    if (params.usageFlags.contains(ImageUsageFlag::DepthStencilTarget))
    {
        defaultUsageMode = ImageUsageMode::GraphicsSampled;
    }
    else if (params.usageFlags.contains(ImageUsageFlag::ColorTarget))
    {
        defaultUsageMode = ImageUsageMode::GraphicsSampled;
    }
    else if (params.usageFlags.contains(ImageUsageFlag::GraphicsSampled))
    {
        defaultUsageMode = ImageUsageMode::GraphicsSampled;
    }
    else if (params.usageFlags.contains(ImageUsageFlag::ComputeSampled))
    {
        defaultUsageMode = ImageUsageMode::ComputeSampled;
    }
    else if (params.usageFlags.contains(ImageUsageFlag::ComputeStorageRead) ||
             params.usageFlags.contains(ImageUsageFlag::ComputeStorageReadWrite))
    {
        defaultUsageMode = ImageUsageMode::ComputeStorageRead;
    }
    else if (params.usageFlags.contains(ImageUsageFlag::TransferSrc))
    {
        defaultUsageMode = ImageUsageMode::TransferSrc;
    }
    else
    {
        m_pGlobal->pLogger->Error("Images::CreateFromParams: Unsupported usage flags");
        return std::unexpected(false);
    }

    //
    // Determine aspect flags
    //
    VkImageAspectFlags vkImageAspectFlags{};
    if (params.usageFlags.contains(ImageUsageFlag::DepthStencilTarget))
    {
        vkImageAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
        vkImageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    //
    // Determine format of the image
    //
    VkFormat vkImageFormat = {};
    VmaAllocationCreateFlags vmaAllocationCreateFlags{};

    if (params.usageFlags.contains(ImageUsageFlag::DepthStencilTarget))
    {
        const auto vkDepthFormat = m_pGlobal->physicalDevice.GetDepthBufferFormat();
        if (!vkDepthFormat)
        {
            m_pGlobal->pLogger->Error("Images::CreateFromParams: Failed to find a supported depth buffer format");
            return std::unexpected(false);
        }

        vkImageFormat = *vkDepthFormat;
        vmaAllocationCreateFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }
    else if (params.usageFlags.contains(ImageUsageFlag::ColorTarget) ||
             params.usageFlags.contains(ImageUsageFlag::PostProcess))
    {
        vkImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        vmaAllocationCreateFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    }
    else
    {
        switch (params.colorSpace)
        {
            case ColorSpace::SRGB: vkImageFormat = VK_FORMAT_B8G8R8A8_SRGB; break;
            case ColorSpace::Linear: vkImageFormat = VK_FORMAT_B8G8R8A8_UNORM; break;
        }
    }

    //
    // Determine usage of the image
    //
    VkImageUsageFlags vkImageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    // Note: no else-ifs
    if (params.usageFlags.contains(ImageUsageFlag::GraphicsSampled) ||
        params.usageFlags.contains(ImageUsageFlag::ComputeSampled))
    {
        vkImageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (params.usageFlags.contains(ImageUsageFlag::ColorTarget))
    {
        vkImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (params.usageFlags.contains(ImageUsageFlag::DepthStencilTarget))
    {
        vkImageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (params.usageFlags.contains(ImageUsageFlag::TransferSrc))
    {
        vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (params.usageFlags.contains(ImageUsageFlag::TransferDst))
    {
        vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (params.usageFlags.contains(ImageUsageFlag::GraphicsStorageRead) ||
        params.usageFlags.contains(ImageUsageFlag::ComputeStorageRead) ||
        params.usageFlags.contains(ImageUsageFlag::ComputeStorageReadWrite))
    {
        vkImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (params.numMipLevels > 1)
    {
        // Need transfer usages to generate mip levels
        vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        vkImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    const ImageDef imageDef = {
        .vkImageType = vkImageType,
        .vkFormat = vkImageFormat,
        .vkExtent = VkExtent3D{
            .width = params.size.GetWidth(),
            .height = params.size.GetHeight(),
            .depth = params.size.GetDepth()
        },
        .numMipLevels = params.numMipLevels,
        .numLayers = params.numLayers,
        .cubeCompatible = params.imageType == ImageType::ImageCube,
        .vkImageUsage = vkImageUsage,
        .vmaMemoryUsage = VMA_MEMORY_USAGE_AUTO,
        .vmaAllocationCreateFlags = vmaAllocationCreateFlags
    };

    //
    // Create ImageViewDefs
    //
    std::vector<ImageViewDef> imageViewDefs;

    VkImageViewType vkImageViewType{};

    switch (params.imageType)
    {
        case ImageType::Image2D: vkImageViewType = VK_IMAGE_VIEW_TYPE_2D; break;
        case ImageType::Image2DArray: vkImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; break;
        case ImageType::Image3D: vkImageViewType = VK_IMAGE_VIEW_TYPE_3D; break;
        case ImageType::ImageCube: vkImageViewType = VK_IMAGE_VIEW_TYPE_CUBE; break;
    }

    // First ImageView encompasses the entire resource
    imageViewDefs.push_back(ImageViewDef{
        .vkImageViewType = vkImageViewType,
        .vkFormat = vkImageFormat,
        .vkImageSubresourceRange = {
            .aspectMask = vkImageAspectFlags,
            .baseMipLevel = 0,
            .levelCount = params.numMipLevels,
            .baseArrayLayer = 0,
            .layerCount = params.numLayers
        }
    });

    // If more than one layer, create additional 2D ImageViews which span each specific layer
    if (params.numLayers > 1)
    {
        for (unsigned int layerIndex = 0; layerIndex < params.numLayers; ++layerIndex)
        {
            imageViewDefs.push_back(ImageViewDef{
                .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D,
                .vkFormat = vkImageFormat,
                .vkImageSubresourceRange = {
                    .aspectMask = vkImageAspectFlags,
                    .baseMipLevel = 0,
                    .levelCount = params.numMipLevels,
                    .baseArrayLayer = layerIndex,
                    .layerCount = 1
                }
            });
        }
    }

    return CreateImage(pCommandBuffer, imageDef, defaultUsageMode, imageViewDefs, tag);
}

std::expected<ImageId, bool> Images::CreateFromSwapChainImage(uint32_t swapChainImageIndex,
                                                              VkImage vkImage,
                                                              VkSwapchainCreateInfoKHR vkSwapchainCreateInfo)
{
    Image image{};
    image.isSwapChainImage = true;
    image.tag = std::format("Swapchain-{}", swapChainImageIndex);

    GPUImage gpuImage{};
    gpuImage.defaultUsageMode = ImageUsageMode::ColorAttachment;

    gpuImage.imageData.vkImage = vkImage;

    // Since we don't actually create the swap chain images, we don't have vkImageCreateInfos for them, so
    // just fake it by filling in relevant fields that we use later, with the data from the swap chain creation
    gpuImage.imageData.imageDef.vkFormat = vkSwapchainCreateInfo.imageFormat;
    gpuImage.imageData.imageDef.vkExtent = VkExtent3D(
        vkSwapchainCreateInfo.imageExtent.width,
        vkSwapchainCreateInfo.imageExtent.height,
        1
    );
    gpuImage.imageData.imageDef.numMipLevels = 1;
    gpuImage.imageData.imageDef.numLayers = 1;

    // Create an image view for accessing the swap chain image
    const ImageViewDef imageViewDef{
        .vkImageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .vkFormat = vkSwapchainCreateInfo.imageFormat,
        .vkImageSubresourceRange = OneLayerOneMipColorResource
    };

    if (!CreateVkImageView(gpuImage, imageViewDef, image.tag, "ImageView"))
    {
        m_pGlobal->pLogger->Error("Images::CreateFromSwapChainImage: Call to CreateVkImageView() failed");
        return std::unexpected(false);
    }

    //
    // Record results
    //
    std::lock_guard<std::recursive_mutex> lock(m_imagesMutex);

    image.id = m_pGlobal->ids.imageIds.GetId();
    image.gpuImages.push_back(gpuImage);

    m_images.insert({image.id, image});

    return image.id;
}

std::expected<ImageId, bool> Images::CreateImage(CommandBuffer* pCommandBuffer,
                                                 const ImageDef& imageDef,
                                                 const ImageUsageMode& defaultUsageMode,
                                                 const std::vector<ImageViewDef>& imageViewDefs,
                                                 const std::string& tag)
{
    const auto gpuImage = CreateGPUImage(pCommandBuffer, imageDef, defaultUsageMode, imageViewDefs, tag);
    if (!gpuImage)
    {
        m_pGlobal->pLogger->Error("Images::CreateImage: Failed to create GPU image");
        return std::unexpected(false);
    }

    //
    // Record results
    //
    std::lock_guard<std::recursive_mutex> lock(m_imagesMutex);

    Image image{};
    image.id = m_pGlobal->ids.imageIds.GetId();
    image.isSwapChainImage = false;
    image.tag = tag;
    image.gpuImages.push_back(*gpuImage);

    m_images.insert({image.id, image});

    return image.id;
}

std::expected<GPUImage, bool> Images::CreateGPUImage(CommandBuffer* pCommandBuffer,
                                                     const ImageDef& imageDef,
                                                     const ImageUsageMode& defaultUsageMode,
                                                     const std::vector<ImageViewDef>& imageViewDefs,
                                                     const std::string& tag)
{
    GPUImage gpuImage{};
    gpuImage.defaultUsageMode = defaultUsageMode;

    //
    // Create VkImage
    //
    const auto vmaAllocationCreateInfo = GetVmaAllocationCreateInfo(imageDef);

    if (!CreateVkImage(gpuImage, imageDef, vmaAllocationCreateInfo, tag))
    {
        m_pGlobal->pLogger->Error("Images::CreateGPUImage: Call to CreateVkImage() failed");
        return std::unexpected(false);
    }

    //
    // Create VkImageViews
    //
    for (uint32_t x = 0; x < imageViewDefs.size(); ++x)
    {
        if (!CreateVkImageView(gpuImage, imageViewDefs.at(x), tag, std::format("{}", x)))
        {
            m_pGlobal->pLogger->Error("Images::CreateGPUImage: Call to CreateVkImageView() failed");
            return std::unexpected(false);
        }
    }

    //
    // Transition the image to its default usage state so that whenever it's first used
    // we don't need to keep track of whether it's in Undefined or Default layout.
    //
    {
        // Doing it in two steps so validation doesn't complain that there's no point
        // in transitioning directly from Undefined to Sampled since there would be nothing
        // to sample from
        pCommandBuffer->CmdImagePipelineBarrier(gpuImage, GetWholeImageSubresourceRange(gpuImage), ImageUsageMode::Undefined, ImageUsageMode::TransferDst);
        pCommandBuffer->CmdImagePipelineBarrier(gpuImage, GetWholeImageSubresourceRange(gpuImage), ImageUsageMode::TransferDst, defaultUsageMode);
    }

    return gpuImage;
}

VkImageCreateInfo Images::GetVkImageCreateInfo(const ImageDef& imageDef)
{
    VkImageCreateFlags vkImageCreateFlags{0};

    if (imageDef.cubeCompatible)
    {
        if (imageDef.numLayers < 6)
        {
            m_pGlobal->pLogger->Error("Images::GetVkImageCreateInfo: Image specified as cube compatible, but doesn't have at least six layers, ignoring");
        }
        else
        {
            vkImageCreateFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }
    }

    VkImageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext = nullptr;
    info.imageType = imageDef.vkImageType;
    info.format = imageDef.vkFormat;
    info.extent = imageDef.vkExtent;
    info.mipLevels = imageDef.numMipLevels;
    info.arrayLayers = imageDef.numLayers;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = imageDef.vkImageUsage;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.flags = vkImageCreateFlags;

    return info;
}

VmaAllocationCreateInfo Images::GetVmaAllocationCreateInfo(const ImageDef& imageDef)
{
    VmaAllocationCreateInfo vmaAllocationCreateInfo{};
    vmaAllocationCreateInfo.usage = imageDef.vmaMemoryUsage;
    vmaAllocationCreateInfo.flags = imageDef.vmaAllocationCreateFlags;

    return vmaAllocationCreateInfo;
}

VkImageViewCreateInfo Images::GetVkImageViewCreateInfo(VkImage vkImage, const ImageViewDef& imageViewDef)
{
    VkImageViewCreateInfo vkImageViewCreateInfo{};
    vkImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vkImageViewCreateInfo.flags = {};
    vkImageViewCreateInfo.image = vkImage;
    vkImageViewCreateInfo.viewType = imageViewDef.vkImageViewType;
    vkImageViewCreateInfo.format = imageViewDef.vkFormat;
    vkImageViewCreateInfo.subresourceRange = imageViewDef.vkImageSubresourceRange;

    return vkImageViewCreateInfo;
}

bool Images::CreateVkImage(GPUImage& gpuImage,
                           const ImageDef& imageDef,
                           const VmaAllocationCreateInfo& vmaAllocationCreateInfo,
                           const std::string& tag)
{
    const auto vkImageCreateInfo = GetVkImageCreateInfo(imageDef);

    VkImage vkImage{VK_NULL_HANDLE};
    VmaAllocation vmaAllocation{VK_NULL_HANDLE};
    VmaAllocationInfo vmaAllocationInfo{};

    const auto result = vmaCreateImage(
        m_pGlobal->vma,
        &vkImageCreateInfo,
        &vmaAllocationCreateInfo,
        &vkImage,
        &vmaAllocation,
        &vmaAllocationInfo
    );
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("Images::CreateVkImage: vmaCreateImage() failure, result code: {}", (uint32_t)result);
        return false;
    }

    SetDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)vkImage, std::format("Image-{}", tag));

    gpuImage.imageData = {
        .vkImage = vkImage,
        .imageDef = imageDef,
        .imageAllocation = {
            .vmaAllocationCreateInfo = vmaAllocationCreateInfo,
            .vmaAllocation = vmaAllocation,
            .vmaAllocationInfo = vmaAllocationInfo
        }
    };

    return true;
}

bool Images::CreateVkImageView(GPUImage& gpuImage,
                               const ImageViewDef& imageViewDef,
                               const std::string& imageTag,
                               const std::string& imageViewTag)
{
    const auto vkImageViewCreateInfo = GetVkImageViewCreateInfo(gpuImage.imageData.vkImage, imageViewDef);

    VkImageView vkImageView{VK_NULL_HANDLE};

    auto result = m_pGlobal->vk.vkCreateImageView(m_pGlobal->device.GetVkDevice(), &vkImageViewCreateInfo, nullptr, &vkImageView);
    if (result != VK_SUCCESS)
    {
        m_pGlobal->pLogger->Error("Images::CreateVkImageView: vkCreateImageView() call failed, result code: {}", (uint32_t)result);
        return false;
    }

    SetDebugName(m_pGlobal->vk,
                 m_pGlobal->device,
                 VK_OBJECT_TYPE_IMAGE_VIEW,
                 (uint64_t)vkImageView,
                 std::format("ImageView-{}-{}", imageTag, imageViewTag));

    gpuImage.imageViewDatas.push_back(GPUImageViewData{
        .vkImageView = vkImageView,
        .imageViewDef = imageViewDef
    });

    return true;
}

std::optional<GPUImage> Images::GetImage(ImageId imageId, bool cycled, std::optional<CommandBuffer*> commandBuffer)
{
    std::lock_guard<std::recursive_mutex> lock(m_imagesMutex);

    auto it = m_images.find(imageId);
    if (it == m_images.cend())
    {
        return std::nullopt;
    }

    if (m_imagesMarkedForDeletion.contains(imageId))
    {
        m_pGlobal->pLogger->Warning("Images::GetImage: Image was marked for deletion, not returning it: {}", imageId.id);
        return std::nullopt;
    }

    if (cycled && it->second.isSwapChainImage)
    {
        m_pGlobal->pLogger->Error("Images::GetImage: Can't cycle a swap chain image");
        return it->second.gpuImages.at(it->second.activeImageIndex);
    }

    if (cycled && !commandBuffer)
    {
        m_pGlobal->pLogger->Error("Images::GetImage: If cycled, must provide a command buffer");
        return std::nullopt;
    }

    if (cycled)
    {
        if (!CycleImageIfNeeded(*commandBuffer, it->second))
        {
            m_pGlobal->pLogger->Error("Images::GetImage: Failed to cycle the image");
            return std::nullopt;
        }
    }

    return it->second.gpuImages.at(it->second.activeImageIndex);
}

bool Images::CycleImageIfNeeded(CommandBuffer* pCommandBuffer, Image& image)
{
    // If the active GPU image is unused, return it
    const auto activeUsageCount = m_pGlobal->pUsages->images.GetGPUUsageCount(image.gpuImages.at(image.activeImageIndex).imageData.vkImage);
    if (activeUsageCount == 0)
    {
        return true;
    }

    // Otherwise, try to find an existing GPU image which is unused
    for (uint32_t x = 0; x < image.gpuImages.size(); ++x)
    {
        // Already tested the active image above
        if (x == image.activeImageIndex) { continue; }

        const auto usageCount = m_pGlobal->pUsages->images.GetGPUUsageCount(image.gpuImages.at(x).imageData.vkImage);
        if (usageCount == 0)
        {
            image.activeImageIndex = x;
            return true;
        }
    }

    // Otherwise, create a new GPU image
    const auto sampleGPUImage = image.gpuImages.at(0);

    std::vector<ImageViewDef> imageViewDefs;

    std::ranges::transform(sampleGPUImage.imageViewDatas, std::back_inserter(imageViewDefs), [](const auto& gpuImageView){
        return gpuImageView.imageViewDef;
    });

    const auto gpuImage = CreateGPUImage(
        pCommandBuffer,
        sampleGPUImage.imageData.imageDef,
        sampleGPUImage.defaultUsageMode,
        imageViewDefs,
        image.tag
    );
    if (!gpuImage)
    {
        m_pGlobal->pLogger->Error("Images::CycleImageIfNeeded: Failed to create new image for cycling");
        return false;
    }

    image.gpuImages.push_back(*gpuImage);
    image.activeImageIndex = (uint32_t)image.gpuImages.size() - 1;

    return true;
}

bool Images::BarrierImageRangeForUsage(CommandBuffer* pCommandBuffer,
                                       const GPUImage& gpuImage,
                                       const VkImageSubresourceRange& vkImageSubresourceRange,
                                       ImageUsageMode destUsageMode)
{
    pCommandBuffer->CmdImagePipelineBarrier(gpuImage, vkImageSubresourceRange, gpuImage.defaultUsageMode, destUsageMode);

    return true;
}

bool Images::BarrierImageRangeToDefaultUsage(CommandBuffer* pCommandBuffer,
                                             const GPUImage& gpuImage,
                                             const VkImageSubresourceRange& vkImageSubresourceRange,
                                             ImageUsageMode sourceUsageMode)
{
    pCommandBuffer->CmdImagePipelineBarrier(gpuImage, vkImageSubresourceRange, sourceUsageMode, gpuImage.defaultUsageMode);

    return true;
}

bool Images::BarrierWholeImageForUsage(CommandBuffer* pCommandBuffer, const GPUImage& gpuImage, ImageUsageMode destUsageMode)
{
    pCommandBuffer->CmdImagePipelineBarrier(gpuImage, GetWholeImageSubresourceRange(gpuImage), gpuImage.defaultUsageMode, destUsageMode);

    return true;
}

bool Images::BarrierWholeImageToDefaultUsage(CommandBuffer* pCommandBuffer, const GPUImage& gpuImage, ImageUsageMode sourceUsageMode)
{
    pCommandBuffer->CmdImagePipelineBarrier(gpuImage, GetWholeImageSubresourceRange(gpuImage), sourceUsageMode, gpuImage.defaultUsageMode);

    return true;
}

void Images::DestroyImage(ImageId imageId, bool destroyImmediately)
{
    std::lock_guard<std::recursive_mutex> lock(m_imagesMutex);

    const auto it = m_images.find(imageId);
    if (it == m_images.cend())
    {
        m_pGlobal->pLogger->Warning("Images::DestroyImage: No such image exists: {}", imageId.id);
        return;
    }

    if (destroyImmediately)
    {
        DestroyImageObjects(it->second);

        m_images.erase(imageId);
        m_pGlobal->ids.imageIds.ReturnId(imageId);
    }
    else
    {
        m_imagesMarkedForDeletion.insert(imageId);
    }
}

void Images::DestroyImageObjects(const Image& image)
{
    m_pGlobal->pLogger->Debug("Images: Destroying image objects: {}", image.id.id);

    for (const auto& gpuImage : image.gpuImages)
    {
        DestroyGPUImageObjects(gpuImage, image.isSwapChainImage);
    }
}

void Images::DestroyGPUImageObjects(const GPUImage& gpuImage, bool isSwapChainImage)
{
    for (const auto& imageViewData: gpuImage.imageViewDatas)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t) imageViewData.vkImageView);
        m_pGlobal->vk.vkDestroyImageView(m_pGlobal->device.GetVkDevice(), imageViewData.vkImageView, nullptr);
    }

    if (!isSwapChainImage)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_IMAGE, (uint64_t) gpuImage.imageData.vkImage);
        vmaDestroyImage(m_pGlobal->vma, gpuImage.imageData.vkImage, gpuImage.imageData.imageAllocation.vmaAllocation);
    }
}

VkImageSubresourceRange Images::GetWholeImageSubresourceRange(const GPUImage& gpuImage)
{
    return VkImageSubresourceRange {
        .aspectMask = GetImageAspectFlags(gpuImage),
        .baseMipLevel = 0,
        .levelCount = gpuImage.imageData.imageDef.numMipLevels,
        .baseArrayLayer = 0,
        .layerCount = gpuImage.imageData.imageDef.numLayers
    };
}

VkImageAspectFlags Images::GetImageAspectFlags(const GPUImage& gpuImage)
{
    const bool depthImage = (gpuImage.imageData.imageDef.vkImageUsage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0;
    return depthImage ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
}

void Images::RunCleanUp()
{
    // Clean up images that are marked as deleted which no longer have any references/usages
    CleanUp_DeletedImages();

    // Clean up GPU images which aren't used; try to collapse images back to one GPU image
    CleanUp_UnusedImages();
}

void Images::CleanUp_DeletedImages()
{
    std::lock_guard<std::recursive_mutex> lock(m_imagesMutex);

    std::unordered_set<ImageId> noLongerMarkedForDeletion;

    for (const auto& imageId : m_imagesMarkedForDeletion)
    {
        const auto image = m_images.find(imageId);
        if (image == m_images.cend())
        {
            m_pGlobal->pLogger->Error("Images::CleanUp_DeletedImages: Image marked for deletion doesn't exist: {}", imageId.id);
            noLongerMarkedForDeletion.insert(imageId);
            continue;
        }

        // To destroy the image, all of its gpuImages have to both be unused by any command buffer and no system
        // exists with a lock on it
        const bool allGPUImagesUnused = std::ranges::all_of(image->second.gpuImages, [this](const auto& gpuImage){
            const bool noUsages = m_pGlobal->pUsages->images.GetGPUUsageCount(gpuImage.imageData.vkImage) == 0;
            const bool noLocks = m_pGlobal->pUsages->images.GetLockCount(gpuImage.imageData.vkImage) == 0;

            return noUsages && noLocks;
        });

        if (allGPUImagesUnused)
        {
            DestroyImage(imageId, true);
            noLongerMarkedForDeletion.insert(imageId);
        }
    }

    for (const auto& imageId : noLongerMarkedForDeletion)
    {
        m_imagesMarkedForDeletion.erase(imageId);
    }
}

void Images::CleanUp_UnusedImages()
{
    // TODO Perf
}

}
