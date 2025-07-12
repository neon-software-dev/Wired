/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <NEON/Common/Thread/MessageDrivenThreadPool.h>
#include <NEON/Common/Thread/ThreadUtil.h>

#include <sstream>
#include <cassert>

namespace NCommon
{

MessageDrivenThreadPool::MessageDrivenThreadPool(std::string tag,
                                                 unsigned int poolSize,
                                                 std::optional<MessageHandler> msgHandler,
                                                 std::optional<IdleHandler> idleHandler,
                                                 const std::chrono::milliseconds& idleInterval)
    : m_tag(std::move(tag))
    , m_poolSize(poolSize)
    , m_msgHandler(std::move(msgHandler))
    , m_idleHandler(std::move(idleHandler))
    , m_idleInterval(idleInterval)
    , m_msgQueue(std::make_unique<ConcurrentQueue<EnqueuedMessage>>())
{
    for (unsigned int x = 0; x < m_poolSize; ++x)
    {
        const auto threadIdentifier = GetThreadIdentifier(x);

        auto thread = std::thread(&MessageDrivenThreadPool::MessageReceiverThreadFunc, this, threadIdentifier);

        NCommon::SetThreadName(thread, threadIdentifier);

        m_threads.emplace_back(std::move(thread));
    }
}

MessageDrivenThreadPool::~MessageDrivenThreadPool()
{
    // Negate the run flag, prompting all threads to finish
    m_run = false;

    // Unblock all threads from waiting for new messages, allowing them to see
    // the new run state
    for (unsigned int x = 0; x < m_poolSize; ++x)
    {
        m_msgQueue->UnblockPopper(GetThreadIdentifier(x));
    }

    // Now wait for all threads to finish
    for (auto& thread : m_threads)
    {
        thread.join();
    }
}

void MessageDrivenThreadPool::PostMessage(const Message::Ptr& message, const std::optional<MessageHandler>& messageHandler)
{
    m_msgQueue->Push(EnqueuedMessage(message, messageHandler));
}

void MessageDrivenThreadPool::MessageReceiverThreadFunc(const std::string& threadIdentifier)
{
    std::optional<std::chrono::milliseconds> maxWait;

    if (m_idleHandler)
    {
        maxWait = m_idleInterval;
    }

    while (m_run)
    {
        // Wait until either we receive a message to be handled, or the queue has stopped us
        // from listening due to a UnblockPopper call when this class is destructed, or
        // max wait time has expired
        const auto msg = m_msgQueue->BlockingPop(threadIdentifier, maxWait);

        // Check whether we were told to stop, and stop the thread if so.
        if (!m_run) { return; }

        // Otherwise, if there was a message popped, process it
        if (msg)
        {
            // If the message has a handler set, invoke it
            if (msg->handler)
            {
                (*msg->handler)(msg->message);
            }
            // Otherwise, if a global message handler was provided, invoke it
            else if (m_msgHandler)
            {
                (*m_msgHandler)(msg->message);
            }
            else
            {
                // Need at least some sort of handler for the message
                assert(false);
            }
        }
        // Otherwise, if the pop timed out, give the handler an idle callback
        else if (m_idleHandler)
        {
            (*m_idleHandler)();
        }
    }
}

std::string MessageDrivenThreadPool::GetThreadIdentifier(unsigned int threadIndex)
{
    std::stringstream ss;
    ss << "MTP" << threadIndex << "-" << m_tag;
    return ss.str();
}

}
