/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DescriptorSets.h"

#include "../Global.h"
#include "../Usages.h"

#include <NEON/Common/Hash.h>

namespace Wired::GPU
{

DescriptorSets::DescriptorSets(Global* pGlobal, std::string tag)
    : m_pGlobal(pGlobal)
    , m_tag(std::move(tag))
    , m_descriptorPools(m_pGlobal)
{

}

DescriptorSets::~DescriptorSets()
{
    m_pGlobal = nullptr;
}

void DescriptorSets::Destroy()
{
    m_pGlobal->pLogger->Info("DescriptorSets: {} - Destroying", m_tag);

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    m_descriptorPools.Destroy();
    m_descriptorSets.clear();
}

std::expected<VulkanDescriptorSet, bool> DescriptorSets::GetVulkanDescriptorSet(const DescriptorSetRequest& request, const std::string& tag)
{
    const auto requestHash = GetHash(request);

    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // Try to find an active descriptor set with the layout and bindings as requested. If found, return it
    {
        const auto it = m_descriptorSets.find(requestHash);
        if (it != m_descriptorSets.cend())
        {
            it->second.cleanUpsWithoutUse = 0;
            return it->second.vulkanDescriptorSet;
        }
    }

    std::optional<VulkanDescriptorSet> vulkanDescriptorSet;

    // If not found, try to pop a descriptor set for the layout from cache
    {
        const auto it = m_cached.find(request.descriptorSetLayout.GetVkDescriptorSetLayout());
        if ((it != m_cached.cend()) && (!it->second.empty()))
        {
            vulkanDescriptorSet = it->second.front();
            it->second.pop();
        }
    }

    // If not found, allocate a new descriptor set
    if (!vulkanDescriptorSet)
    {
        const auto allocatedVulkanDescriptorSet = m_descriptorPools.AllocateDescriptorSet(request.descriptorSetLayout, tag);
        if (!allocatedVulkanDescriptorSet)
        {
            m_pGlobal->pLogger->Error("DescriptorSets::GetDescriptorSet_Concrete: Failed to allocate new descriptor set from pools");
            return std::unexpected(false);
        }

        vulkanDescriptorSet = *allocatedVulkanDescriptorSet;
    }

    // Update the descriptor set with the requested bindings
    vulkanDescriptorSet->Write(request.bindings);

    // Now that we've bound resources to the descriptor set, updates usage tracker
    // so that the descriptor set holds a lock to those resources; we don't want
    // them deleted while the descriptor set still has them bound
    LockDescriptorSetResources(*vulkanDescriptorSet);

    // Update internal state
    m_descriptorSets.insert({requestHash, DescriptorSet{
        .cleanUpsWithoutUse = 0,
        .vkDescriptorSetLayout = request.descriptorSetLayout.GetVkDescriptorSetLayout(),
        .vulkanDescriptorSet = *vulkanDescriptorSet
    }});

    return *vulkanDescriptorSet;
}

void DescriptorSets::RunCleanUp(bool isIdleCleanUp)
{
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    // We want to cache unused sets, but we also don't want to consider idle clean up flows as an "unused" flow, or
    // else all sets will be moved to cached when the app is back grounded and then all have to be brought back out
    // of cache and rebound when the app is restored again. We only want to cache sets if the app is actively
    // rendering and the sets are not being used during those active renders.
    if (!isIdleCleanUp)
    {
        RunCleanUp_CacheUnusedSets();
    }

    //
    // Erase entries for layouts in the cached list which have no cached descriptor sets
    //
    std::erase_if(m_cached, [](const auto& it){ return it.second.empty(); });

    /*std::size_t totalCached{0};
    for (const auto& it : m_cached)
    {
        totalCached += it.second.size();
    }
    m_pGlobal->pLogger->Error("Total Active: {}, Total Cached: {}", m_descriptorSets.size(), totalCached);*/
}

void DescriptorSets::RunCleanUp_CacheUnusedSets()
{
    //
    // Look through all of our active descriptor sets for any that can be moved to the cached list
    //
    std::vector<RequestHash> toEraseHashes;

    for (auto& it : m_descriptorSets)
    {
        const auto usageCount = m_pGlobal->pUsages->descriptorSets.GetGPUUsageCount(it.second.vulkanDescriptorSet.GetVkDescriptorSet());
        const bool setHasAnyUsage = usageCount > 0;

        if (setHasAnyUsage)
        {
            it.second.cleanUpsWithoutUse = 0;
        }
        else
        {
            // Cache the descriptor set if it's gone for 10 clean up flows with no GPU usage of it
            if (it.second.cleanUpsWithoutUse++ >= 10)
            {
                // Unlock the set's resources. If the set is ever used again it'll have new
                // resources bound to it, so no use holds locks to resources just because the
                // set is setting in the cache list with resources still associated with it.
                UnlockDescriptorSetResources(it.second.vulkanDescriptorSet);

                // Move the set to the cache list
                m_cached[it.second.vkDescriptorSetLayout].push(it.second.vulkanDescriptorSet);

                // Erase the set from the active list
                toEraseHashes.push_back(it.first);
            }
        }
    }

    for (const auto& it : toEraseHashes)
    {
        m_descriptorSets.erase(it);
    }
}

void DescriptorSets::LockDescriptorSetResources(const VulkanDescriptorSet& vulkanDescriptorSet)
{
    const auto& setBindings = vulkanDescriptorSet.GetSetBindings();

    for (const auto& it : setBindings.bufferBindings)
    {
        m_pGlobal->pUsages->buffers.IncrementLock(it.second.gpuBuffer.vkBuffer);
    }
    for (const auto& it : setBindings.imageViewBindings)
    {
        m_pGlobal->pUsages->images.IncrementLock(it.second.gpuImage.imageData.vkImage);
        m_pGlobal->pUsages->imageViews.IncrementLock(it.second.gpuImage.imageViewDatas.at(it.second.imageViewIndex).vkImageView);
    }
    for (const auto& it : setBindings.imageViewSamplerBindings)
    {
        for (const auto& binding : it.second.arrayBindings)
        {
            m_pGlobal->pUsages->images.IncrementLock(binding.second.gpuImage.imageData.vkImage);
            m_pGlobal->pUsages->imageViews.IncrementLock(binding.second.gpuImage.imageViewDatas.at(binding.second.imageViewIndex).vkImageView);
        }
    }
}

void DescriptorSets::UnlockDescriptorSetResources(const VulkanDescriptorSet& vulkanDescriptorSet)
{
    const auto& setBindings = vulkanDescriptorSet.GetSetBindings();

    for (const auto& it : setBindings.bufferBindings)
    {
        m_pGlobal->pUsages->buffers.DecrementLock(it.second.gpuBuffer.vkBuffer);
    }
    for (const auto& it : setBindings.imageViewBindings)
    {
        m_pGlobal->pUsages->images.DecrementLock(it.second.gpuImage.imageData.vkImage);
        m_pGlobal->pUsages->imageViews.DecrementLock(it.second.gpuImage.imageViewDatas.at(it.second.imageViewIndex).vkImageView);
    }
    for (const auto& it : setBindings.imageViewSamplerBindings)
    {
        for (const auto& binding : it.second.arrayBindings)
        {
            m_pGlobal->pUsages->images.DecrementLock(binding.second.gpuImage.imageData.vkImage);
            m_pGlobal->pUsages->imageViews.DecrementLock(binding.second.gpuImage.imageViewDatas.at(binding.second.imageViewIndex).vkImageView);
        }
    }
}

DescriptorSets::RequestHash DescriptorSets::GetHash(const DescriptorSetRequest& request)
{
    std::size_t hash = 0;

    NCommon::HashCombine(hash, request.descriptorSetLayout.GetVkDescriptorSetLayout());

    NCommon::HashCombine(hash, 1);
    for (const auto& binding : request.bindings.bufferBindings)
    {
        NCommon::HashCombineVar(hash, binding.first, binding.second.gpuBuffer.vkBuffer, binding.second.byteOffset, binding.second.byteSize);
    }

    NCommon::HashCombine(hash, 2);
    for (const auto& binding : request.bindings.imageViewBindings)
    {
        NCommon::HashCombineVar(hash, binding.first, binding.second.gpuImage.imageData.vkImage, binding.second.imageViewIndex);
    }

    NCommon::HashCombine(hash, 3);
    for (const auto& it : request.bindings.imageViewSamplerBindings)
    {
        for (const auto& binding : it.second.arrayBindings)
        {
            NCommon::HashCombineVar(hash, it.first, binding.first, binding.second.gpuImage.imageData.vkImage, binding.second.imageViewIndex, binding.second.vkSampler);
        }
    }

    return hash;
}

}
