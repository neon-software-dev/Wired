/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/Util/PerlinNoise.h>

#include <glm/gtc/constants.hpp>

namespace Wired::Engine
{

static constexpr auto NUM_GRADIENT_SAMPLES = 512U;

static inline float Fade(float t)
{
    return ((6.0f * t - 15.0f) * t + 10.0f) * t * t * t;
    //return t*t*(3-2*t); // s-curve
}

PerlinNoise PerlinNoise::Create(unsigned int seed)
{
    auto mt = std::mt19937(seed);
    auto dist = std::uniform_real_distribution<float>(0.0f, 2.0f * glm::pi<float>());

    auto gradients = std::vector<glm::vec2>();
    gradients.resize(NUM_GRADIENT_SAMPLES);

    for (unsigned int x = 0; x < NUM_GRADIENT_SAMPLES; ++x)
    {
        const float angle = dist(mt);
        gradients[x] = glm::vec2(std::cos(angle), std::sin(angle));
    }

    return PerlinNoise{seed, gradients};
}

PerlinNoise::PerlinNoise(unsigned int seed, std::vector<glm::vec2> gradients)
    : m_seed(seed)
    , m_gradients(std::move(gradients))
{

}

float PerlinNoise::operator()(const glm::vec2& p) const
{
    return Get(p);
}

glm::vec2 PerlinNoise::GetGradient(int x, int y) const
{
    //const std::size_t gradientIndex = ((x * 1836311903 ^ y * 2971215073 ^ m_seed) & 0x7fffffff) % NUM_GRADIENT_SAMPLES;

    const std::size_t gradientIndex =
        static_cast<std::size_t>(
            (static_cast<uint64_t>(x) * 1836311903ULL ^ static_cast<uint64_t>(y) * 2971215073ULL ^ m_seed))
            % NUM_GRADIENT_SAMPLES;

    return m_gradients.at(gradientIndex);
}

float PerlinNoise::Get(const glm::vec2& p, unsigned int numOctaves) const
{
    float result = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;

    float totalAmplitude = 0.0f;

    for (unsigned int x = 0; x < numOctaves; ++x)
    {
        result += amplitude * Get(glm::vec2(p.x * frequency, p.y * frequency));

        totalAmplitude += amplitude;

        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return result / totalAmplitude;
}

float PerlinNoise::Get(const glm::vec2& p) const
{
    // Top-Left X/Y coordinates of the cell that p is contained within
    auto pCellX = static_cast<int>(std::floor(p.x));
    auto pCellY = static_cast<int>(std::floor(p.y));

    // Fetch the grid gradients of the cell's four bounding points.
    // Note the clockwise ordering.
    const auto& gv1 = GetGradient(pCellX, pCellY);
    const auto& gv2 = GetGradient(pCellX + 1, pCellY);
    const auto& gv3 = GetGradient(pCellX + 1, pCellY + 1);
    const auto& gv4 = GetGradient(pCellX, pCellY + 1);

    // Calculate the offset vectors pointing from each bounding point to the query point
    const auto ov1 = p - glm::vec2(pCellX, pCellY);
    const auto ov2 = p - glm::vec2(pCellX + 1, pCellY);
    const auto ov3 = p - glm::vec2(pCellX + 1, pCellY + 1);
    const auto ov4 = p - glm::vec2(pCellX, pCellY + 1);

    // Calculate the dot products of each bounding point's random gradient with the offset
    // vector from that bounding point to the query point
    const auto d1 = glm::dot(gv1, ov1);
    const auto d2 = glm::dot(gv2, ov2);
    const auto d3 = glm::dot(gv3, ov3);
    const auto d4 = glm::dot(gv4, ov4);

    // X/Y percentages (0.0..1.0) of the query point's position within its cell
    const float xPercent = p.x - (float)pCellX;
    const float yPercent = p.y - (float)pCellY;

    // Fade/smoothed X/Y percentages
    const float xS = Fade(xPercent);
    const float yS = Fade(yPercent);

    // Lerp the calculated dot products in X directions then in Y direction
    const auto topXLerp = glm::mix(d1, d2, xS);
    const auto bottomXLerp = glm::mix(d4, d3, xS); // Note the correction for clockwise ordering
    const auto yLerp = glm::mix(topXLerp, bottomXLerp, yS);

    return yLerp;
}

std::vector<float> PerlinNoise::Get(const std::pair<int, int>& queryOffset,
                                    unsigned int querySize,
                                    unsigned int dataSize,
                                    unsigned int numOctaves) const
{
    // The interval of query points within the query section needed to match the data size
    const float interval = (float)querySize / (float)(dataSize - 1);

    std::vector<float> data;
    data.resize(dataSize * dataSize);

    for (unsigned int y = 0; y < dataSize; ++y)
    {
        for (unsigned int x = 0; x < dataSize; ++x)
        {
            const glm::vec2 queryPoint = {
                (float)queryOffset.first + ((float)x * interval),
                (float)queryOffset.second + ((float)y * interval)
            };

            data[x + (y * dataSize)] = Get(queryPoint, numOctaves);
        }
    }

    return data;
}

std::unique_ptr<NCommon::ImageData> PerlinNoise::ToImage(const std::vector<float>& data)
{
    const auto dataSize = (unsigned int)std::sqrt(data.size());

    std::vector<std::byte> dataBytes;
    dataBytes.reserve(data.size() * 4);

    for (const auto& val : data)
    {
        // Convert from [-1,1] -> [0,1]
        const float rangedVal = (val + 1.0f) / 2.0f;

        // Convert from [0,1] -> [0,255]
        const auto imageByte = std::byte(rangedVal * 255.0f);

        dataBytes.push_back(imageByte);        // B
        dataBytes.push_back(imageByte);        // G
        dataBytes.push_back(imageByte);        // R
        dataBytes.push_back(std::byte(255));   // A
    }

    return std::make_unique<NCommon::ImageData>(
        dataBytes,
        1,
        dataSize,
        dataSize,
        NCommon::ImageData::PixelFormat::B8G8R8A8_LINEAR
    );
}

}
