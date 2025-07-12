/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_MESSAGEDRIVENTHREADPOOL_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_MESSAGEDRIVENTHREADPOOL_H

#include <NEON/Common/SharedLib.h>
#include <NEON/Common/Thread/ResultMessage.h>
#include <NEON/Common/Container/ConcurrentQueue.h>

#include <thread>
#include <memory>
#include <functional>
#include <optional>
#include <vector>

namespace NCommon
{
    static constexpr auto DEFAULT_IDLE_INTERVAL = std::chrono::milliseconds(50);

    /**
     * Manages a thread pool which messages can be posted to for asynchronous processing on thread pool threads.
     *
     * Even though it can manage an entire thread pool, specifying a pool size of 1 is a normal use case; spawning
     * a single thread that can be communicated with via messages.
     *
     * Use the PostMessage() method to submit messages to be processed by the thread pool.
     *
     * When posting a message, a specific message handler can be provided. If so, that handler will be invoked by
     * the thread pool. If not, the global message handler provided to the constructor will be invoked.
     *
     * An optional idle handler can be provided to the constructor. If so, *every* thread in the thread pool which
     * has not received a message within the last m_idleInterval milliseconds will invoke that handler.
     */
    class NEON_PUBLIC MessageDrivenThreadPool
    {
        public:

            using MessageHandler = std::function<void(const Message::Ptr&)>;
            using IdleHandler = std::function<void()>;

        public:

            /**
             * @param tag Tag to associate with the thread pool
             * @param poolSize Number of threads to spawn for handling messages
             * @param msgHandler An optional MessageHandler that should be executed when messages arrive
             * @param idleHandler An optional handler, that if provided, will be executed every idleInterval that no message has arrived
             * @param idleInterval The interval upon which the idle handler should be called if no messages are being received
             */
            explicit MessageDrivenThreadPool(
                std::string tag,
                unsigned int poolSize = 1,
                std::optional<MessageHandler> msgHandler = std::nullopt,
                std::optional<IdleHandler> idleHandler = std::nullopt,
                const std::chrono::milliseconds& idleInterval = DEFAULT_IDLE_INTERVAL);
            ~MessageDrivenThreadPool();

            /**
             * Send a message from the current thread to the message handling thread pool.
             *
             * Full thread safety to be called from any thread.
             *
             * @param message The message to be sent.
             * @param messageHandler An optional handler to be invoked, overriding the global message handler
             */
            void PostMessage(const Message::Ptr& message, const std::optional<MessageHandler>& messageHandler = std::nullopt);

            /**
             * Send a simple, no-data, message from the current thread to the message handling thread pool.
             *
             * Full thread safety to be called from any thread.
             *
             * @param typeIdentifier String which identifies the message
             * @param messageHandler An optional handler to be invoked, overriding the global message handler
             */
            void PostMessage(const std::string& typeIdentifier, const std::optional<MessageHandler>& messageHandler = std::nullopt)
            {
                return PostMessage(std::make_shared<Message>(typeIdentifier), messageHandler);
            }

            /**
             * Executes a func on the thread pool, returning a future containing the result.
             *
             * Full thread safety to be called from any thread.
             */
            template <typename Func>
            [[nodiscard]] auto DispatchForResult(const std::string& tag, Func func)
            {
                using ResultT = std::invoke_result_t<Func>;
                auto message = std::make_shared<ResultMessage<ResultT>>(tag);
                auto future = message->CreateFuture();
                PostMessage(message, [=](const auto& execMessage){
                    std::dynamic_pointer_cast<ResultMessage<ResultT>>(execMessage)
                        ->SetResult(func());
                });
                return future;
            }

        private:

            struct EnqueuedMessage
            {
                EnqueuedMessage() = default;

                EnqueuedMessage(Message::Ptr _message, std::optional<MessageHandler> _handler)
                    : message(std::move(_message))
                    , handler(std::move(_handler))
                { }

                Message::Ptr message;
                std::optional<MessageHandler> handler;
            };

        private:

            [[nodiscard]] std::string GetThreadIdentifier(unsigned int threadIndex);

            void MessageReceiverThreadFunc(const std::string& threadIdentifier);

        private:

            std::string m_tag;
            unsigned int m_poolSize;
            std::optional<MessageHandler> m_msgHandler;
            std::optional<IdleHandler> m_idleHandler;
            std::chrono::milliseconds m_idleInterval;
            std::unique_ptr<ConcurrentQueue<EnqueuedMessage>> m_msgQueue;

            bool m_run = true;
            std::vector<std::thread> m_threads;
    };
}


#endif //NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_MESSAGEDRIVENTHREADPOOL_H
