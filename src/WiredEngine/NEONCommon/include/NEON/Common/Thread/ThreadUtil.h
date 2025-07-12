/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_THREADUTIL_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_THREADUTIL_H

#include <NEON/Common/SharedLib.h>
#include <NEON/Common/BuildInfo.h>

#include <future>
#include <thread>
#include <string>

namespace NCommon
{
    /**
     * Creates a future which already has a value immediately available
     */
    template <typename T>
    std::future<T> ImmediateFuture(const T& value)
    {
        std::promise<T> promise;
        std::future<T> future = promise.get_future();
        promise.set_value(value);
        return future;
    }

    // Specialized helper func for ImmediateFuture to work with std::future<void>
    inline std::future<void> ImmediateFuture()
    {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();
        promise.set_value();
        return future;
    }

    /**
     * Sets the name of the provided native thread handle. OS dependent.
     */
    NEON_PUBLIC void SetThreadName(std::thread& thread, const std::string& name);
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_THREADUTIL_H
