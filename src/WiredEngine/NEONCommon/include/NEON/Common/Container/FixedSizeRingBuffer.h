/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_CONTAINER_FIXEDSIZERINGBUFFER_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_CONTAINER_FIXEDSIZERINGBUFFER_H

#include <array>
#include <ranges>

namespace NCommon
{
    /**
     * Provides a simple ring buffer container. Can push elements into it and when it reaches
     * its capacity limit, starts using an internal offset to drop elements from the front.
     */
    template <typename T, unsigned int CAPACITY>
    class FixedSizeRingBuffer
    {
        public:

            template <typename U>
            void PushBack(U&& val)
            {
                if (m_size < CAPACITY)
                {
                    m_data.at(m_size++) = std::forward<U>(val);
                    return;
                }

                if (m_offset == CAPACITY)
                {
                    m_offset = 0;
                    std::ranges::copy(m_data | std::views::drop(CAPACITY), m_data.begin());
                }

                m_data.at(m_offset++ + CAPACITY) = std::forward<U>(val);
            }

            [[nodiscard]] std::size_t GetSize() const noexcept { return m_size; }
            [[nodiscard]] bool IsEmpty() const noexcept { return m_size == 0; }

            [[nodiscard]] T& At(const std::size_t& index) { return m_data.at(m_offset + index); }
            T& operator[](const std::size_t& index) { return m_data.at(m_offset + index); }

            const T* Data() const { return m_data.data() + m_offset; }

        private:

            std::array<T, CAPACITY * 2> m_data;
            std::size_t m_offset{0};
            std::size_t m_size{0};
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_CONTAINER_FIXEDSIZERINGBUFFER_H
