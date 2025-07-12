/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_CONTAINER_CONCURRENTQUEUE_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_CONTAINER_CONCURRENTQUEUE_H

#include <NEON/Common/SharedLib.h>

#include <memory>
#include <queue>
#include <string>
#include <condition_variable>
#include <optional>
#include <functional>
#include <algorithm>
#include <unordered_set>

namespace NCommon
{
    /**
     * A Queue which has full thread safety when accessed and manipulated by multiple threads.
     *
     * @tparam T The type of data stored in the queue. Must be copy constructable.
     */
    template <class T>
        requires std::is_copy_assignable_v<T>
    class ConcurrentQueue
    {
        public:

            /**
             * Push a new item into the queue.
             *
             * Will block while acquiring the queue mutex.
             */
            void Push(const T& item)
            {
                const std::lock_guard<std::mutex> lock(m_mutex);

                m_data.push(item);

                m_wakeCv.notify_one();
            }

            /**
             * Whether the queue is currently empty at the time of calling.
             *
             * Will block while acquiring the queue mutex.
             */
            [[nodiscard]] bool IsEmpty() const
            {
                const std::lock_guard<std::mutex> lock(m_mutex);

                return m_data.empty();
            }

            /**
             * Gets the size of the queue at the time of calling.
             *
             * @return The size of the queue
             */
            [[nodiscard]] std::size_t Size() const
            {
                const std::lock_guard<std::mutex> lock(m_mutex);

                return m_data.size();
            }

            /**
             * Returns a copy of the item at the top of the queue, if any.
             *
             * @return A copy of the item at the top of the queue, or std::nullopt
             * if the queue is empty at the time of calling.
             */
            [[nodiscard]] std::optional<T> TryPeek()
            {
                const std::lock_guard<std::mutex> lock(m_mutex);

                return m_data.empty() ? std::nullopt : m_data.front();
            }

            /**
             * Tries to pop an item off of the queue, if one exists.
             *
             * Will block while acquiring the queue mutex. Once the mutex is
             * acquired, will return immediately.
             *
             * @param item The popped item, if the method's output was True.
             * @return Whether an item was available to be popped.
             */
            [[nodiscard]] std::optional<T> TryPop()
            {
                const std::lock_guard<std::mutex> lock(m_mutex);

                if (m_data.empty())
                {
                    return std::nullopt;
                }

                auto item = m_data.front();
                m_data.pop();
                return item;
            }


            /**
             * Blocking call that blocks the calling thread until an item can be successfully popped from the
             * queue (or the optional timeout has expired).
             *
             * The blocked thread can be released from its waiting by a call to UnblockPopper() from a different
             * thread, supplying the same identifier.
             *
             * Consumers which are waiting for a new item via BlockingPop are notified of new items in
             * round-robin fashion. Only one consumer is notified when the queue receives a new item.
             *
             * @param identifier A string that uniquely identifies the calling thread
             * @param timeout An optional maximum amount of time to wait for an item to be popped
             *
             * @return The popped item if an item could be popped, or std::nullopt if the timeout was hit
             * or if the wait was interrupted by a call to UnblockPopper.
             */
            [[nodiscard]] std::optional<T> BlockingPop(const std::string& identifier,
                                                       const std::optional<std::chrono::milliseconds>& timeout = std::nullopt)
            {
                // Obtain a unique lock to access m_data
                std::unique_lock<std::mutex> lock(m_mutex);

                // If m_data already has items, no need to wait for new items to arrive, just immediately
                // pop the top item off and return it
                if (!m_data.empty())
                {
                    auto item = m_data.front();
                    m_data.pop();
                    return item;
                }

                // Otherwise, wait until either 1) there's an item available, 2) the wait has been
                // cancelled, or 3) the wait has timed out.
                const auto waitPredicate = [&] {
                    const bool isCancelled = m_unblockSet.contains(identifier);
                    const bool isDataAvailable = !m_data.empty();

                    return isCancelled || isDataAvailable;
                };

                if (timeout.has_value())
                {
                    m_wakeCv.wait_for(lock, *timeout, waitPredicate);
                }
                else
                {
                    m_wakeCv.wait(lock, waitPredicate);
                }

                // Once done waiting, return an item if it exists, or std::nullopt
                // if no item exists, which would be the case in the scenario where
                // the wait timed out or was cancelled
                if (!m_data.empty())
                {
                    auto item = m_data.front();
                    m_data.pop();
                    return item;
                }

                return std::nullopt;
            }

            /**
             * Cancels/unblocks the blocking wait of a thread's previous call to BlockingPop()
             *
             * @param identifier The identifier that was registered in the BlockingPop() call
             */
            void UnblockPopper(const std::string& identifier)
            {
                const std::lock_guard<std::mutex> lock(m_mutex);

                m_unblockSet.insert(identifier);

                m_wakeCv.notify_all();
            }

        private:

            std::queue<T> m_data;                               // The queue of data being managed
            std::unordered_set<std::string> m_unblockSet;       // Entries represent cancelled BlockingPop calls

            std::condition_variable m_wakeCv;                   // Used to notify threads of changes
            mutable std::mutex m_mutex;                         // Used to synchronize access to data
    };
}


#endif //NEONCOMMON_INCLUDE_NEON_COMMON_CONTAINER_CONCURRENTQUEUE_H
