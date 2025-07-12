/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanCallsUtil.h"

namespace Wired::GPU
{

#define FIND_GLOBAL_CALL_REQ(name) \
    vulkanCalls.name = (PFN_##name)vulkanCalls.vkGetInstanceProcAddr(nullptr, #name); \
    if (vulkanCalls.name == VK_NULL_HANDLE) { return false; } \

#define FIND_INSTANCE_CALL_REQ(name) \
    vulkanCalls.name = (PFN_##name)vulkanCalls.vkGetInstanceProcAddr(vkInstance, #name); \
    if (vulkanCalls.name == VK_NULL_HANDLE) { return false; } \

#define FIND_INSTANCE_CALL_OPT(name) \
    vulkanCalls.name = (PFN_##name)vulkanCalls.vkGetInstanceProcAddr(vkInstance, #name); \

#define FIND_DEVICE_CALL_REQ(name) \
    vulkanCalls.name = (PFN_##name)vulkanCalls.vkGetDeviceProcAddr(vkDevice, #name); \
    if (vulkanCalls.name == VK_NULL_HANDLE) { return false; } \

#define FIND_DEVICE_CALL_OPT(name) \
    vulkanCalls.name = (PFN_##name)vulkanCalls.vkGetDeviceProcAddr(vkDevice, #name); \

bool ResolveGlobalCalls(VulkanCalls& vulkanCalls)
{
    FIND_GLOBAL_CALL_REQ(vkEnumerateInstanceVersion)
    FIND_GLOBAL_CALL_REQ(vkEnumerateInstanceExtensionProperties)
    FIND_GLOBAL_CALL_REQ(vkEnumerateInstanceLayerProperties)
    FIND_GLOBAL_CALL_REQ(vkCreateInstance)

    return true;
}

bool ResolveInstanceCalls(VulkanCalls& vulkanCalls, VkInstance vkInstance)
{
    FIND_INSTANCE_CALL_OPT(vkCreateDebugUtilsMessengerEXT)
    FIND_INSTANCE_CALL_OPT(vkDestroyDebugUtilsMessengerEXT)
    FIND_INSTANCE_CALL_OPT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
    FIND_INSTANCE_CALL_OPT(vkGetPhysicalDeviceSurfaceFormatsKHR)
    FIND_INSTANCE_CALL_OPT(vkGetPhysicalDeviceSurfacePresentModesKHR)
    FIND_INSTANCE_CALL_OPT(vkGetPhysicalDeviceSurfaceSupportKHR)

    FIND_INSTANCE_CALL_REQ(vkGetDeviceProcAddr)
    FIND_INSTANCE_CALL_REQ(vkDestroyInstance)
    FIND_INSTANCE_CALL_REQ(vkEnumeratePhysicalDevices)
    FIND_INSTANCE_CALL_REQ(vkGetPhysicalDeviceProperties)
    FIND_INSTANCE_CALL_REQ(vkGetPhysicalDeviceProperties2)
    FIND_INSTANCE_CALL_REQ(vkGetPhysicalDeviceFeatures2)
    FIND_INSTANCE_CALL_REQ(vkGetPhysicalDeviceMemoryProperties)
    FIND_INSTANCE_CALL_REQ(vkGetPhysicalDeviceMemoryProperties2)
    FIND_INSTANCE_CALL_REQ(vkGetPhysicalDeviceQueueFamilyProperties)
    FIND_INSTANCE_CALL_REQ(vkEnumerateDeviceExtensionProperties)
    FIND_INSTANCE_CALL_REQ(vkGetPhysicalDeviceFormatProperties2)
    FIND_INSTANCE_CALL_REQ(vkCreateDevice)

    return true;
}

bool ResolveDeviceCalls(VulkanCalls& vulkanCalls, VkDevice vkDevice)
{
    FIND_DEVICE_CALL_OPT(vkCreateSwapchainKHR)
    FIND_DEVICE_CALL_OPT(vkDestroySwapchainKHR)
    FIND_DEVICE_CALL_OPT(vkGetSwapchainImagesKHR)
    FIND_DEVICE_CALL_OPT(vkSetDebugUtilsObjectNameEXT)
    FIND_DEVICE_CALL_OPT(vkQueueBeginDebugUtilsLabelEXT)
    FIND_DEVICE_CALL_OPT(vkQueueEndDebugUtilsLabelEXT)
    FIND_DEVICE_CALL_OPT(vkCmdBeginDebugUtilsLabelEXT)
    FIND_DEVICE_CALL_OPT(vkCmdEndDebugUtilsLabelEXT)
    FIND_DEVICE_CALL_OPT(vkAcquireNextImageKHR)
    FIND_DEVICE_CALL_OPT(vkQueuePresentKHR)

    FIND_DEVICE_CALL_REQ(vkDestroyDevice)
    FIND_DEVICE_CALL_REQ(vkGetDeviceQueue)
    FIND_DEVICE_CALL_REQ(vkCreateImageView)
    FIND_DEVICE_CALL_REQ(vkDestroyImageView)
    FIND_DEVICE_CALL_REQ(vkDestroyImage)
    FIND_DEVICE_CALL_REQ(vkCreateCommandPool)
    FIND_DEVICE_CALL_REQ(vkDestroyCommandPool)
    FIND_DEVICE_CALL_REQ(vkAllocateCommandBuffers)
    FIND_DEVICE_CALL_REQ(vkFreeCommandBuffers)
    FIND_DEVICE_CALL_REQ(vkResetCommandBuffer)
    FIND_DEVICE_CALL_REQ(vkResetCommandPool)
    FIND_DEVICE_CALL_REQ(vkQueueSubmit2)
    FIND_DEVICE_CALL_REQ(vkDeviceWaitIdle)
    FIND_DEVICE_CALL_REQ(vkCreateFence)
    FIND_DEVICE_CALL_REQ(vkDestroyFence)
    FIND_DEVICE_CALL_REQ(vkCreateSemaphore)
    FIND_DEVICE_CALL_REQ(vkDestroySemaphore)
    FIND_DEVICE_CALL_REQ(vkBeginCommandBuffer)
    FIND_DEVICE_CALL_REQ(vkEndCommandBuffer)
    FIND_DEVICE_CALL_REQ(vkWaitForFences)
    FIND_DEVICE_CALL_REQ(vkResetFences)
    FIND_DEVICE_CALL_REQ(vkCmdClearColorImage)
    FIND_DEVICE_CALL_REQ(vkAllocateMemory)
    FIND_DEVICE_CALL_REQ(vkFreeMemory)
    FIND_DEVICE_CALL_REQ(vkMapMemory)
    FIND_DEVICE_CALL_REQ(vkUnmapMemory)
    FIND_DEVICE_CALL_REQ(vkFlushMappedMemoryRanges)
    FIND_DEVICE_CALL_REQ(vkInvalidateMappedMemoryRanges)
    FIND_DEVICE_CALL_REQ(vkBindBufferMemory)
    FIND_DEVICE_CALL_REQ(vkBindImageMemory)
    FIND_DEVICE_CALL_REQ(vkGetBufferMemoryRequirements)
    FIND_DEVICE_CALL_REQ(vkGetImageMemoryRequirements)
    FIND_DEVICE_CALL_REQ(vkCreateBuffer)
    FIND_DEVICE_CALL_REQ(vkDestroyBuffer)
    FIND_DEVICE_CALL_REQ(vkCreateImage)
    FIND_DEVICE_CALL_REQ(vkCmdCopyBuffer)
    FIND_DEVICE_CALL_REQ(vkCmdCopyBuffer2)
    FIND_DEVICE_CALL_REQ(vkGetBufferMemoryRequirements2)
    FIND_DEVICE_CALL_REQ(vkGetImageMemoryRequirements2)
    FIND_DEVICE_CALL_REQ(vkBindBufferMemory2)
    FIND_DEVICE_CALL_REQ(vkBindImageMemory2)
    FIND_DEVICE_CALL_REQ(vkGetDeviceBufferMemoryRequirements)
    FIND_DEVICE_CALL_REQ(vkGetDeviceImageMemoryRequirements)
    FIND_DEVICE_CALL_REQ(vkCmdCopyImage)
    FIND_DEVICE_CALL_REQ(vkCmdPipelineBarrier2)
    FIND_DEVICE_CALL_REQ(vkCmdExecuteCommands)
    FIND_DEVICE_CALL_REQ(vkGetFenceStatus)
    FIND_DEVICE_CALL_REQ(vkCmdBlitImage)
    FIND_DEVICE_CALL_REQ(vkCmdCopyBufferToImage2)
    FIND_DEVICE_CALL_REQ(vkCreateShaderModule)
    FIND_DEVICE_CALL_REQ(vkDestroyShaderModule)
    FIND_DEVICE_CALL_REQ(vkCmdBeginRendering)
    FIND_DEVICE_CALL_REQ(vkCmdEndRendering)
    FIND_DEVICE_CALL_REQ(vkCreateSampler)
    FIND_DEVICE_CALL_REQ(vkDestroySampler)
    FIND_DEVICE_CALL_REQ(vkCreatePipelineLayout)
    FIND_DEVICE_CALL_REQ(vkCreateGraphicsPipelines)
    FIND_DEVICE_CALL_REQ(vkCreateComputePipelines)
    FIND_DEVICE_CALL_REQ(vkDestroyPipeline)
    FIND_DEVICE_CALL_REQ(vkDestroyPipelineLayout)
    FIND_DEVICE_CALL_REQ(vkCreateDescriptorSetLayout)
    FIND_DEVICE_CALL_REQ(vkDestroyDescriptorSetLayout)
    FIND_DEVICE_CALL_REQ(vkCmdBindPipeline)
    FIND_DEVICE_CALL_REQ(vkCmdBindVertexBuffers)
    FIND_DEVICE_CALL_REQ(vkCmdBindIndexBuffer)
    FIND_DEVICE_CALL_REQ(vkCmdDrawIndexed)
    FIND_DEVICE_CALL_REQ(vkCreateDescriptorPool)
    FIND_DEVICE_CALL_REQ(vkAllocateDescriptorSets)
    FIND_DEVICE_CALL_REQ(vkFreeDescriptorSets)
    FIND_DEVICE_CALL_REQ(vkResetDescriptorPool)
    FIND_DEVICE_CALL_REQ(vkDestroyDescriptorPool)
    FIND_DEVICE_CALL_REQ(vkCmdBindDescriptorSets)
    FIND_DEVICE_CALL_REQ(vkUpdateDescriptorSets)
    FIND_DEVICE_CALL_REQ(vkCmdDispatch)
    FIND_DEVICE_CALL_REQ(vkCmdDrawIndexedIndirect)
    FIND_DEVICE_CALL_REQ(vkCmdDrawIndexedIndirectCount)
    FIND_DEVICE_CALL_REQ(vkCmdSetDepthTestEnable)
    FIND_DEVICE_CALL_REQ(vkCmdSetDepthWriteEnable)
    FIND_DEVICE_CALL_REQ(vkCreateQueryPool)
    FIND_DEVICE_CALL_REQ(vkDestroyQueryPool)
    FIND_DEVICE_CALL_REQ(vkCmdResetQueryPool)
    FIND_DEVICE_CALL_REQ(vkCmdWriteTimestamp2)
    FIND_DEVICE_CALL_REQ(vkGetQueryPoolResults)

    return true;
}

}
