/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_UTIL_PERLINNOISE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_UTIL_PERLINNOISE_H

#include <NEON/Common/ImageData.h>
#include <NEON/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <random>

namespace Wired::Engine
{
    class NEON_PUBLIC PerlinNoise
    {
        public:

            [[nodiscard]] static PerlinNoise Create(unsigned int seed);

        public:

            /** See: Get(..) */
            [[nodiscard]] float operator()(const glm::vec2& p) const;

            /**
             * Query for the noise value at a specific query coordinate.
             *
             * @param p The coordinate to query
             * @param numOctaves Number of octaves to query for
             *
             * @return The perlin noise value, in the range of [-1,1]
             */
            [[nodiscard]] float Get(const glm::vec2& p, unsigned int numOctaves) const;

            /**
             * Query for noise values within a particular subsection. Will look at the noise
             * that's querySize x querySize points large, at offset queryOffset, and will fetch
             * dataSize x dataSize values from that query subset.
             *
             * @param queryOffset The offset into the noise to query
             * @param querySize The noise size to query for data from
             * @param dataSize The size of data points to create from the query subset
             * @param numOctaves Number of octaves to query for
             *
             * @return The data points
             */
            [[nodiscard]] std::vector<float> Get(const std::pair<int, int>& queryOffset,
                                                 unsigned int querySize,
                                                 unsigned int dataSize,
                                                 unsigned int numOctaves = 1) const;

            /**
             * Simple helper function that converts perlin noise data to a B8G8R8A8_LINEAR formatted ImageData.
             * Sets the R,G, and B values of each pixel to the [0..255] mapped data value, and the A value to 255.
             *
             * @param data Perlin noise data fetched via a call to Get(..)
             *
             * @return an ImageData representing the data
             */
             [[nodiscard]] static std::unique_ptr<NCommon::ImageData> ToImage(const std::vector<float>& data);

        private:

            PerlinNoise(unsigned int seed, std::vector<glm::vec2> gradients);

            [[nodiscard]] glm::vec2 GetGradient(int x, int y) const;

            [[nodiscard]] float Get(const glm::vec2& p) const;

        private:

            unsigned int m_seed;
            std::vector<glm::vec2> m_gradients;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_UTIL_PERLINNOISE_H
