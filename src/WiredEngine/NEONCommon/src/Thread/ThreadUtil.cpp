/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <NEON/Common/Thread/ThreadUtil.h>

#if !__linux__ && !__unix__
    #include <windows.h>
#endif

namespace NCommon
{

void SetThreadName(std::thread& thread, const std::string& name)
{
    std::string threadName = name;

    if (threadName.length() > 15)
    {
        threadName = threadName.substr(0, 15);
    }

    #if __linux__ || __unix__
        pthread_setname_np(thread.native_handle(), threadName.c_str());
    #else
        const std::wstring wideThreadName(threadName.cbegin(), threadName.cend());
        SetThreadDescription(static_cast<HANDLE>(thread.native_handle()), wideThreadName.c_str());
    #endif
}

}
