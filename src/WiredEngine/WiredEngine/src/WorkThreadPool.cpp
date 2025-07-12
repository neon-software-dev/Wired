/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorkThreadPool.h"

namespace Wired::Engine
{
WorkThreadPool::WorkThreadPool(unsigned int numThreads)
    : m_threadPool(std::make_unique<NCommon::MessageDrivenThreadPool>(
        "EngineWork",
        numThreads,
        [](const auto& message) { HandleMessage(message); }
    ))
{

}

WorkThreadPool::~WorkThreadPool()
{
    // Signals any active threads to shut down
    m_canceled = true;

    // Destroying the thread pool blocks until the threads are stopped
    m_threadPool = nullptr;
}

void WorkThreadPool::PumpFinished()
{
    std::erase_if(m_finishedOnMainEntries, [](const std::shared_ptr<WorkEntry>& workEntry) {
        return workEntry->TryFulfill();
    });
}

void WorkThreadPool::HandleMessage(const NCommon::Message::Ptr& message)
{
    std::dynamic_pointer_cast<WorkMessage>(message)->DoWork();
}

}
