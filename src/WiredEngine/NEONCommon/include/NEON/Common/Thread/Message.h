/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_MESSAGE_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_MESSAGE_H

#include <NEON/Common/SharedLib.h>

#include <memory>
#include <string>

namespace NCommon
{
    /**
     * Generic message object which can be used as a cross-thread communication primitive.
     */
    class NEON_PUBLIC Message
    {
        public:

            using Ptr = std::shared_ptr<Message>;

        public:

            /**
             * @param typeIdentifier A string which uniquely identifies the type/subclass of the message
             */
            explicit Message(std::string typeIdentifier)
                : m_typeIdentifier(std::move(typeIdentifier))
            {}

            virtual ~Message() = default;

            /**
             * @return A string which uniquely identifies the type/subclass of the message
             */
            [[nodiscard]] virtual std::string GetTypeIdentifier() const noexcept
            {
                return m_typeIdentifier;
            }

        private:

            std::string m_typeIdentifier;
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_THREAD_MESSAGE_H
