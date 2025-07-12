/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_USAGETRACKER_H
#define WIREDENGINE_WIREDGPUVK_SRC_USAGETRACKER_H

#include <atomic>
#include <unordered_map>
#include <mutex>
#include <unordered_set>
#include <algorithm>

namespace Wired::GPU
{
    /**
     * Tracks usages of a given resource. There's two types of usages that are tracked:
     *
     * - GPU Usages: Any command buffer submitted to the GPU records a usage of the resources
     * it touches/modifies, and those usages are removed when the CommandBuffers system cleans
     * up each finished command buffer. Allows us to know when a resource is / will be touched
     * by the GPU. Mainly used for delaying deletion of resources that the GPU is still using,
     * and also for resource cycling purposes.
     *
     * - Locks: Internal systems can hold locks on resources to indicate that they're "using"
     * the resource (but in this case it's CPU, not GPU, work that's "using" it). This is mostly
     * used for DescriptorSets system to prevent resources from being deleted that are bound to
     * an active descriptor set. This prevents the horrible scenario where, for example, a buffer
     * is bound to a set, is deleted, then a new buffer created, the driver returns the same
     * VkBuffer for the new buffer, and so the old set, which was bound/keyed to the old VkBuffer,
     * gets re-used when something tries to use the new buffer, rather than a new set created.
     */
    template <typename T>
    class UsageTracker
    {
        public:

            void IncrementGPUUsage(const T& t) { ModifyGPUUsage(t, 1); }
            void DecrementGPUUsage(const T& t) { ModifyGPUUsage(t, -1); }

            void IncrementLock(const T& t) { ModifyLock(t, 1); }
            void DecrementLock(const T& t) { ModifyLock(t, -1); }

            [[nodiscard]] int GetGPUUsageCount(const T& t) const
            {
                std::lock_guard<std::recursive_mutex> lock(m_gpuUsageMutex);

                const auto it = m_gpuUsages.find(t);
                if (it == m_gpuUsages.cend())
                {
                    return 0;
                }

                return it->second;
            }

            [[nodiscard]] int GetLockCount(const T& t) const
            {
                std::lock_guard<std::recursive_mutex> lock(m_locksMutex);

                const auto it = m_locks.find(t);
                if (it == m_locks.cend())
                {
                    return 0;
                }

                return it->second;
            }

            void ForgetZeroCountEntries()
            {
                {
                    std::lock_guard<std::recursive_mutex> lock(m_gpuUsageMutex);

                    std::erase_if(m_gpuUsages, [](const auto& it) {
                        return it.second == 0;
                    });
                }

                {
                    std::lock_guard<std::recursive_mutex> lock(m_locksMutex);

                    std::erase_if(m_locks, [](const auto& it) {
                        return it.second == 0;
                    });
                }
            }

            void Reset()
            {
                {
                    std::lock_guard<std::recursive_mutex> lock(m_gpuUsageMutex);
                    m_gpuUsages.clear();
                }

                {
                    std::lock_guard<std::recursive_mutex> lock(m_locksMutex);
                    m_locks.clear();
                }
            }

        private:

            void ModifyGPUUsage(const T& t, int val)
            {
                std::lock_guard<std::recursive_mutex> lock(m_gpuUsageMutex);

                const auto it = m_gpuUsages.find(t);
                if (it == m_gpuUsages.cend())
                {
                    m_gpuUsages.emplace(t, val);
                    return;
                }

                auto& currentVal = it->second;
                currentVal += val;

                assert(currentVal >= 0);
            }

            void ModifyLock(const T& t, int val)
            {
                std::lock_guard<std::recursive_mutex> lock(m_locksMutex);

                const auto it = m_locks.find(t);
                if (it == m_locks.cend())
                {
                    m_locks.emplace(t, val);
                    return;
                }

                auto& currentVal = it->second;
                currentVal += val;

                assert(currentVal >= 0);
            }

        private:

            std::unordered_map<T, int> m_gpuUsages;
            mutable std::recursive_mutex m_gpuUsageMutex;

            std::unordered_map<T, int> m_locks;
            mutable std::recursive_mutex m_locksMutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_USAGETRACKER_H
