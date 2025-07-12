/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_RESULTMESSAGE_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_RESULTMESSAGE_H

#include <NEON/Common/Thread/Message.h>

#include <future>

namespace NCommon
{
    /**
     * Message which allows for a result to be asynchronously returned via a promise/future pair
     *
     * @tparam T The class type of the result to be returned
     */
    template <class T>
    class ResultMessage : public Message
    {
        public:

            using Ptr = std::shared_ptr<ResultMessage>;

        public:

            explicit ResultMessage(std::string typeIdentifier)
                : Message(std::move(typeIdentifier))
            { }

            /**
             * Creates a ResultMessage which fulfills a provided promise rather than
             * creating a new promise unique to this message.
             */
            ResultMessage(std::string typeIdentifier, std::promise<T> promise)
                : Message(std::move(typeIdentifier))
                , m_promise(std::move(promise))
            { }

            /**
             * Call this on caller thread before sending the message to
             * get the future which holds the result of the message.
             *
             * Never call this more than once for a particular message.
             *
             * @return The future that will receive the result
             */
            std::future<T> CreateFuture()
            {
                return m_promise.get_future();
            }

            /**
             * Call to notify the caller thread of the result of the operation
             *
             * @param result The result to report
             */
            void SetResult(const T& result)
            {
                m_promise.set_value(result);
            }

            /**
             * Steals (moves out) the message's promise. If called, then other
             * methods in this class that deal with the promise (e.g. SetResult)
             * can never be called again.
             *
             * @return This message's promise
             */
            std::promise<T> StealPromise()
            {
                return std::move(m_promise);
            }

        private:

            std::promise<T> m_promise;
    };

    //
    // Specific Common ResultMessages
    //

    /**
     * A ResultMessage which returns a boolean result
     */
    struct BoolResultMessage : public NCommon::ResultMessage<bool>
    {
        BoolResultMessage()
            : NCommon::ResultMessage<bool>("BoolResultMessage")
        { }

        explicit BoolResultMessage(std::string typeIdentifier)
            : NCommon::ResultMessage<bool>(std::move(typeIdentifier))
        { }
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_RESULTMESSAGE_H
