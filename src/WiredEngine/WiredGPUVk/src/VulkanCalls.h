/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKANCALLS_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKANCALLS_H

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    struct VulkanCalls
    {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr{nullptr};

        //
        // Global calls
        //
        PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion{nullptr};
        PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties{nullptr};
        PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties{nullptr};
        PFN_vkCreateInstance vkCreateInstance{nullptr};

        //
        // Instance calls
        //
        PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr{nullptr};
        PFN_vkDestroyInstance vkDestroyInstance{nullptr};
        PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT{nullptr};
        PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT{nullptr};
        PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices{nullptr};
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties{nullptr};
        PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2{nullptr};
        PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2{nullptr};
        PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR{nullptr};
        PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR{nullptr};
        PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR{nullptr};
        PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties{nullptr};
        PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2{nullptr};
        PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties{nullptr};
        PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties{nullptr};
        PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR{nullptr};
        PFN_vkGetPhysicalDeviceFormatProperties2 vkGetPhysicalDeviceFormatProperties2{nullptr};
        PFN_vkCreateDevice vkCreateDevice{nullptr};

        //
        // Device calls
        //
        PFN_vkDestroyDevice vkDestroyDevice{nullptr};
        PFN_vkGetDeviceQueue vkGetDeviceQueue{nullptr};
        PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR{nullptr};
        PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR{nullptr};
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR{nullptr};
        PFN_vkCreateImageView vkCreateImageView{nullptr};
        PFN_vkDestroyImageView vkDestroyImageView{nullptr};
        PFN_vkDestroyImage vkDestroyImage{nullptr};
        PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT{nullptr};
        PFN_vkCreateCommandPool vkCreateCommandPool{nullptr};
        PFN_vkDestroyCommandPool vkDestroyCommandPool{nullptr};
        PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers{nullptr};
        PFN_vkFreeCommandBuffers vkFreeCommandBuffers{nullptr};
        PFN_vkResetCommandBuffer vkResetCommandBuffer{nullptr};
        PFN_vkResetCommandPool vkResetCommandPool{nullptr};
        PFN_vkQueueSubmit2 vkQueueSubmit2{nullptr};
        PFN_vkQueueBeginDebugUtilsLabelEXT vkQueueBeginDebugUtilsLabelEXT{nullptr};
        PFN_vkQueueEndDebugUtilsLabelEXT vkQueueEndDebugUtilsLabelEXT{nullptr};
        PFN_vkDeviceWaitIdle vkDeviceWaitIdle{nullptr};
        PFN_vkCreateFence vkCreateFence{nullptr};
        PFN_vkDestroyFence vkDestroyFence{nullptr};
        PFN_vkCreateSemaphore vkCreateSemaphore{nullptr};
        PFN_vkDestroySemaphore vkDestroySemaphore{nullptr};
        PFN_vkBeginCommandBuffer vkBeginCommandBuffer{nullptr};
        PFN_vkEndCommandBuffer vkEndCommandBuffer{nullptr};
        PFN_vkWaitForFences vkWaitForFences{nullptr};
        PFN_vkResetFences vkResetFences{nullptr};
        PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR{nullptr};
        PFN_vkQueuePresentKHR vkQueuePresentKHR{nullptr};
        PFN_vkCmdClearColorImage vkCmdClearColorImage{nullptr};
        PFN_vkAllocateMemory vkAllocateMemory{nullptr};
        PFN_vkFreeMemory vkFreeMemory{nullptr};
        PFN_vkMapMemory vkMapMemory{nullptr};
        PFN_vkUnmapMemory vkUnmapMemory{nullptr};
        PFN_vkFlushMappedMemoryRanges vkFlushMappedMemoryRanges{nullptr};
        PFN_vkInvalidateMappedMemoryRanges vkInvalidateMappedMemoryRanges{nullptr};
        PFN_vkBindBufferMemory vkBindBufferMemory{nullptr};
        PFN_vkBindImageMemory vkBindImageMemory{nullptr};
        PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements{nullptr};
        PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements{nullptr};
        PFN_vkCreateBuffer vkCreateBuffer{nullptr};
        PFN_vkDestroyBuffer vkDestroyBuffer{nullptr};
        PFN_vkCreateImage vkCreateImage{nullptr};
        PFN_vkCmdCopyBuffer vkCmdCopyBuffer{nullptr};
        PFN_vkCmdCopyBuffer2 vkCmdCopyBuffer2{nullptr};
        PFN_vkGetBufferMemoryRequirements2 vkGetBufferMemoryRequirements2{nullptr};
        PFN_vkGetImageMemoryRequirements2 vkGetImageMemoryRequirements2{nullptr};
        PFN_vkBindBufferMemory2 vkBindBufferMemory2{nullptr};
        PFN_vkBindImageMemory2 vkBindImageMemory2{nullptr};
        PFN_vkGetDeviceBufferMemoryRequirements vkGetDeviceBufferMemoryRequirements{nullptr};
        PFN_vkGetDeviceImageMemoryRequirements vkGetDeviceImageMemoryRequirements{nullptr};
        PFN_vkCmdCopyImage vkCmdCopyImage{nullptr};
        PFN_vkCmdPipelineBarrier2 vkCmdPipelineBarrier2{nullptr};
        PFN_vkCmdExecuteCommands vkCmdExecuteCommands{nullptr};
        PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT{nullptr};
        PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT{nullptr};
        PFN_vkGetFenceStatus vkGetFenceStatus{nullptr};
        PFN_vkCmdBlitImage vkCmdBlitImage{nullptr};
        PFN_vkCmdCopyBufferToImage2 vkCmdCopyBufferToImage2{nullptr};
        PFN_vkCreateShaderModule vkCreateShaderModule{nullptr};
        PFN_vkDestroyShaderModule vkDestroyShaderModule{nullptr};
        PFN_vkCmdBeginRendering vkCmdBeginRendering{nullptr};
        PFN_vkCmdEndRendering vkCmdEndRendering{nullptr};
        PFN_vkCreateSampler vkCreateSampler{nullptr};
        PFN_vkDestroySampler vkDestroySampler{nullptr};
        PFN_vkCreatePipelineLayout vkCreatePipelineLayout{nullptr};
        PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines{nullptr};
        PFN_vkCreateComputePipelines vkCreateComputePipelines{nullptr};
        PFN_vkDestroyPipeline vkDestroyPipeline{nullptr};
        PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout{nullptr};
        PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout{nullptr};
        PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout{nullptr};
        PFN_vkCmdBindPipeline vkCmdBindPipeline{nullptr};
        PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers{nullptr};
        PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer{nullptr};
        PFN_vkCmdDrawIndexed vkCmdDrawIndexed{nullptr};
        PFN_vkCreateDescriptorPool vkCreateDescriptorPool{nullptr};
        PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets{nullptr};
        PFN_vkFreeDescriptorSets vkFreeDescriptorSets{nullptr};
        PFN_vkResetDescriptorPool vkResetDescriptorPool{nullptr};
        PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool{nullptr};
        PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets{nullptr};
        PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets{nullptr};
        PFN_vkCmdDispatch vkCmdDispatch{nullptr};
        PFN_vkCmdDrawIndexedIndirect vkCmdDrawIndexedIndirect{nullptr};
        PFN_vkCmdDrawIndexedIndirectCount vkCmdDrawIndexedIndirectCount{nullptr};
        PFN_vkCmdSetDepthTestEnable vkCmdSetDepthTestEnable{nullptr};
        PFN_vkCmdSetDepthWriteEnable vkCmdSetDepthWriteEnable{nullptr};
        PFN_vkCreateQueryPool vkCreateQueryPool{nullptr};
        PFN_vkDestroyQueryPool vkDestroyQueryPool{nullptr};
        PFN_vkCmdResetQueryPool vkCmdResetQueryPool{nullptr};
        PFN_vkCmdWriteTimestamp2 vkCmdWriteTimestamp2{nullptr};
        PFN_vkGetQueryPoolResults vkGetQueryPoolResults{nullptr};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKANCALLS_H
