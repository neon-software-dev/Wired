/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDebugUtil.h"

#include "../Global.h"

namespace Wired::GPU
{

static bool isDebugExtensionAvailable = false;

void MarkDebugExtensionAvailable(bool isAvailable)
{
    isDebugExtensionAvailable = isAvailable;
}

bool IsDebugUtilActive()
{
    // Only activate debug util functionality if the debug extension is active and if we're a dev build

    #ifdef WIRED_DEV_BUILD
        return isDebugExtensionAvailable;
    #endif

    return false;
}

void SetDebugName(const VulkanCalls& vk, const VulkanDevice& device, VkObjectType objType, uint64_t obj, const std::string& name)
{
    if (!IsDebugUtilActive()) { return; }

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.pNext = nullptr;
    nameInfo.objectType = objType;
    nameInfo.objectHandle = obj;
    nameInfo.pObjectName = name.c_str();

    vk.vkSetDebugUtilsObjectNameEXT(device.GetVkDevice(), &nameInfo);
}

void RemoveDebugName(const VulkanCalls& vk, const VulkanDevice& device, VkObjectType objType, uint64_t obj)
{
    if (!IsDebugUtilActive()) { return; }

    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.pNext = nullptr;
    nameInfo.objectType = objType;
    nameInfo.objectHandle = obj;
    nameInfo.pObjectName = nullptr;

    vk.vkSetDebugUtilsObjectNameEXT(device.GetVkDevice(), &nameInfo);
}

void BeginCommandBufferSection(Global* pGlobal, const VkCommandBuffer& vkCmdBuffer, const std::string& sectionName)
{
    if (!IsDebugUtilActive()) { return; }

    VkDebugUtilsLabelEXT labelInfo{};
    labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pLabelName =  sectionName.c_str();
    labelInfo.color[0] = 0.0f; labelInfo.color[1] = 0.5f; labelInfo.color[2] = 0.5f; labelInfo.color[3] = 1.0f;

    pGlobal->vk.vkCmdBeginDebugUtilsLabelEXT(vkCmdBuffer, &labelInfo);
}

void EndCommandBufferSection(Global* pGlobal, const VkCommandBuffer& vkCmdBuffer)
{
    if (!IsDebugUtilActive()) { return; }

    pGlobal->vk.vkCmdEndDebugUtilsLabelEXT(vkCmdBuffer);
}

QueueSectionLabel::QueueSectionLabel(Global* pGlobal, const VkQueue& vkQueue, const std::string& sectionName)
    : m_pGlobal(pGlobal)
    , m_vkQueue(vkQueue)
{
    if (!IsDebugUtilActive()) { return; }

    VkDebugUtilsLabelEXT labelInfo{};
    labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
    labelInfo.pLabelName = sectionName.c_str();
    labelInfo.color[0] = 0.0f; labelInfo.color[1] = 0.0f; labelInfo.color[2] = 1.0f; labelInfo.color[3] = 1.0f;

    m_pGlobal->vk.vkQueueBeginDebugUtilsLabelEXT(m_vkQueue, &labelInfo);
}

QueueSectionLabel::~QueueSectionLabel()
{
    if (!IsDebugUtilActive()) { return; }

    m_pGlobal->vk.vkQueueEndDebugUtilsLabelEXT(m_vkQueue);
}

CmdBufferSectionLabel::CmdBufferSectionLabel(Global* pGlobal, const VkCommandBuffer& vkCmdBuffer, const std::string& sectionName)
    : m_pGlobal(pGlobal)
    , m_vkCmdBuffer(vkCmdBuffer)
{
    BeginCommandBufferSection(pGlobal, m_vkCmdBuffer, sectionName);
}

CmdBufferSectionLabel::~CmdBufferSectionLabel()
{
    EndCommandBufferSection(m_pGlobal, m_vkCmdBuffer);
}

}
