/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDEBUGUTIL_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDEBUGUTIL_H

#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandBuffer.h"

#include "../VulkanCalls.h"

#include <string>

namespace Wired::GPU
{
    void MarkDebugExtensionAvailable(bool isAvailable);

    //
    // Debug names for vulkan objects
    //
    void SetDebugName(const VulkanCalls& vk, const VulkanDevice& device, VkObjectType objType, uint64_t obj, const std::string& name);
    void RemoveDebugName(const VulkanCalls& vk, const VulkanDevice& device, VkObjectType objType, uint64_t obj);

    //
    // Command buffer sections
    //
    void BeginCommandBufferSection(Global* pGlobal, const VkCommandBuffer& vkCmdBuffer, const std::string& sectionName);
    void EndCommandBufferSection(Global* pGlobal, const VkCommandBuffer& vkCmdBuffer);

    //
    // Scoped queue debug section
    //
    struct QueueSectionLabel
    {
            QueueSectionLabel(Global* pGlobal, const VkQueue& vkQueue, const std::string& sectionName);
            ~QueueSectionLabel();

        private:

            Global* m_pGlobal;
            VkQueue m_vkQueue;
    };

    //
    // Scoped command buffer section
    //
    struct CmdBufferSectionLabel
    {
            CmdBufferSectionLabel(Global* pGlobal, const VkCommandBuffer& vkCmdBuffer, const std::string& sectionName);
            ~CmdBufferSectionLabel();

        private:

            Global* m_pGlobal;
            VkCommandBuffer m_vkCmdBuffer;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDEBUGUTIL_H
