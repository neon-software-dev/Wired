/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_USAGES_H
#define WIREDENGINE_WIREDGPUVK_SRC_USAGES_H

#include "UsageTracker.h"

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    struct Usages
    {
        UsageTracker<VkImage> images;
        UsageTracker<VkImageView> imageViews;
        UsageTracker<VkBuffer> buffers;
        UsageTracker<VkSampler> samplers;
        UsageTracker<CommandBufferId> commandBuffers;
        UsageTracker<VkPipeline> pipelines;
        UsageTracker<VkShaderModule> shaders;
        UsageTracker<VkDescriptorSet> descriptorSets;

        void ForgetZeroUsageItems()
        {
            images.ForgetZeroCountEntries();
            imageViews.ForgetZeroCountEntries();
            buffers.ForgetZeroCountEntries();
            samplers.ForgetZeroCountEntries();
            commandBuffers.ForgetZeroCountEntries();
            pipelines.ForgetZeroCountEntries();
            descriptorSets.ForgetZeroCountEntries();
        }

        void Reset()
        {
            images.Reset();
            imageViews.Reset();
            buffers.Reset();
            samplers.Reset();
            commandBuffers.Reset();
            pipelines.Reset();
            descriptorSets.Reset();
        }
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_USAGES_H
