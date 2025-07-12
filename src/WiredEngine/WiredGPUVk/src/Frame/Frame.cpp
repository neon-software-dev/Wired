/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Frame.h"

#include "../Global.h"
#include "../Usages.h"

#include "../Image/Images.h"
#include "../Sampler/VkSamplers.h"

#include "../Vulkan/VulkanDebugUtil.h"

#include <NEON/Common/Log/ILogger.h>

#ifdef WIRED_IMGUI
    #include <imgui_impl_vulkan.h>
#endif

#include <format>
#include <algorithm>

namespace Wired::GPU
{

Frame::Frame(Global* _pGlobal, uint32_t _frameIndex)
    : m_pGlobal(_pGlobal)
    , m_frameIndex(_frameIndex)
{

}

bool Frame::Create()
{
    //
    // Create semaphores
    //
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    m_pGlobal->vk.vkCreateSemaphore(m_pGlobal->device.GetVkDevice(), &semaphoreInfo, nullptr, &m_swapChainImageAvailableSemaphore);
    SetDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_swapChainImageAvailableSemaphore,
                 std::format("Semaphore-Frame{}-ImageAvailable", m_frameIndex));

    m_pGlobal->vk.vkCreateSemaphore(m_pGlobal->device.GetVkDevice(), &semaphoreInfo, nullptr, &m_presentWorkFinishedSemaphore);
    SetDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_presentWorkFinishedSemaphore,
                 std::format("Semaphore-Frame{}-PresentWorkFinished", m_frameIndex));

    //
    // Create frame timestamps
    //

    // If requesting any number of timestamps
    if (m_pGlobal->gpuSettings.numTimestamps > 0)
    {
        // And the command queue supports timestamps
        if (Timestamps::QueueFamilySupportsTimestampQueries(m_pGlobal, m_pGlobal->commandQueue.GetQueueFamilyIndex()))
        {
            // Create timestamps associated with this frame
            auto frameTimestamps = Timestamps::Create(m_pGlobal, std::format("Frame-{}", m_frameIndex));
            if (frameTimestamps)
            {
                m_timestamps = std::move(*frameTimestamps);
            }
        }
    }

    return true;
}

void Frame::Destroy()
{
    //
    // Destroy persistent resources
    //
    if (m_swapChainImageAvailableSemaphore != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_swapChainImageAvailableSemaphore);
        m_pGlobal->vk.vkDestroySemaphore(m_pGlobal->device.GetVkDevice(), m_swapChainImageAvailableSemaphore, nullptr);
        m_swapChainImageAvailableSemaphore = VK_NULL_HANDLE;
    }

    if (m_presentWorkFinishedSemaphore != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)m_presentWorkFinishedSemaphore);
        m_pGlobal->vk.vkDestroySemaphore(m_pGlobal->device.GetVkDevice(), m_presentWorkFinishedSemaphore, nullptr);
        m_presentWorkFinishedSemaphore = VK_NULL_HANDLE;
    }

    if (m_timestamps)
    {
        (*m_timestamps)->Destroy();
        m_timestamps = std::nullopt;
    }

    //
    // Reset runtime state
    //
    m_swapChainPresentIndex = std::nullopt;
    m_associatedCommandBufferIds.clear();
    m_imGuiImageReferencesIncoming.clear();
    m_imGuiImageReferences.clear();
}

void Frame::SetSwapChainPresentIndex(uint32_t swapChainImageIndex)
{
    m_swapChainPresentIndex = swapChainImageIndex;
}

void Frame::ResetSwapChainPresentIndex()
{
    m_swapChainPresentIndex = std::nullopt;
}

void Frame::AssociateCommandBuffer(CommandBufferId commandBufferId)
{
    m_associatedCommandBufferIds.insert(commandBufferId);

    // Report a usage of the command buffer. We don't want the CommandBuffers system to destroy
    // the command buffer until this frame has had a chance to wait for its work to finish. Note
    // that before we didn't care if CommandBuffers cleaned it up, as we assumed that if a
    // CommandBuffer was destroyed then it must have been finished, but that breaks when command
    // buffer ids are re-used; this frame might then be associated with the wrong (but same id)
    // command buffer.
    m_pGlobal->pUsages->commandBuffers.IncrementGPUUsage(commandBufferId);
}

void Frame::UnAssociateCommandBuffer(CommandBufferId commandBufferId)
{
    if (!m_associatedCommandBufferIds.contains(commandBufferId))
    {
        return;
    }

    m_associatedCommandBufferIds.erase(commandBufferId);
    m_pGlobal->pUsages->commandBuffers.DecrementGPUUsage(commandBufferId);
}

void Frame::ClearAssociatedCommandBuffers()
{
    // Release usages of the associated command buffers
    for (const auto& associatedCommandBufferId : m_associatedCommandBufferIds)
    {
        m_pGlobal->pUsages->commandBuffers.DecrementGPUUsage(associatedCommandBufferId);
    }
    m_associatedCommandBufferIds.clear();
}

std::optional<Timestamps*> Frame::GetTimestamps() const
{
    if (!m_timestamps)
    {
        return std::nullopt;
    }

    return m_timestamps->get();
}

std::optional<uint64_t > Frame::CreateImGuiImageReference(ImageId imageId, SamplerId samplerId)
{
    if (!m_pGlobal->imGuiActive) { return std::nullopt; }

    #ifdef WIRED_IMGUI
        const auto image = m_pGlobal->pImages->GetImage(imageId, false);
        if (!image)
        {
            m_pGlobal->pLogger->Error("Frame::CreateImGuiTextureReference: No such image exists: {}", imageId.id);
            return std::nullopt;
        }

        const auto vkImage = image->imageData.vkImage;
        const auto vkImageView = image->imageViewDatas.at(0).vkImageView;

        const auto sampler = m_pGlobal->pSamplers->GetSampler(samplerId);
        if (!sampler)
        {
            m_pGlobal->pLogger->Error("Frame::CreateImGuiTextureReference: No such sampler exists: {}", samplerId.id);
            return std::nullopt;
        }

        const auto vkSampler = sampler->GetVkSampler();

        const auto vkDescriptorSet = ImGui_ImplVulkan_AddTexture(
            sampler->GetVkSampler(),
            image->imageViewDatas.at(0).vkImageView,
            VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL
        );

        // This is a little strange. ImGui references are created by the engine, while the engine is preparing data
        // for a frame render, so they're created BEFORE the next frame is started. So we insert any created references
        // into our "incoming" set, to differentiate them from references that are "old", from the last time the frame
        // was rendered, since we need to clean up the latter when a frame starts, but not the former.
        m_imGuiImageReferencesIncoming.insert(
            ImGuiImageReference{
                .imageId = imageId,
                .vkImage = vkImage,
                .vkImageView = vkImageView,
                .vkSampler = vkSampler,
                .vkDescriptorSet = vkDescriptorSet
            }
        );

        // Also, since ImGui creates and submits their own command buffers out of our control, we can't make use of the
        // normal CommandBuffer usage tracking. Instead, the best we can do is track the usage of the resources with
        // regard to the frame itself, which also works since ImGui work is only performed as part of a frame's work.
        // We manually record usage of the resource on a per-frame level. Note that we don't track the descriptor set
        // usage, as ImGui owns that, and we don't use it otherwise.
        m_pGlobal->pUsages->images.IncrementGPUUsage(vkImage);
        m_pGlobal->pUsages->imageViews.IncrementGPUUsage(vkImageView);
        m_pGlobal->pUsages->samplers.IncrementGPUUsage(sampler->GetVkSampler());

        return (ImTextureID)vkDescriptorSet;
    #else
        (void)imageId; (void)samplerId;
        return std::nullopt;
    #endif
}

void Frame::ClearImGuiImageReferences()
{
    if (!m_pGlobal->imGuiActive) { return; }

    #ifdef WIRED_IMGUI
        // Clean up old imgui references from the previous time this frame was rendered
        for (const auto& imGuiRef : m_imGuiImageReferences)
        {
            // Record the frame as no longer using the ImGui referenced objects
            m_pGlobal->pUsages->images.DecrementGPUUsage(imGuiRef.vkImage);
            m_pGlobal->pUsages->imageViews.DecrementGPUUsage(imGuiRef.vkImageView);
            m_pGlobal->pUsages->samplers.DecrementGPUUsage(imGuiRef.vkSampler);

            // Tell ImGui it can now free/return the descriptor set it created
            ImGui_ImplVulkan_RemoveTexture(imGuiRef.vkDescriptorSet);
        }
        m_imGuiImageReferences.clear();

        // Any incoming references are now considered old, so they can be cleaned up the
        // next time this frame is rendered
        m_imGuiImageReferences = m_imGuiImageReferencesIncoming;
        m_imGuiImageReferencesIncoming.clear();
    #endif
}

}
