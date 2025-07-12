/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <NEON/Common/ImageData.h>

#include <cassert>

namespace NCommon
{

ImageData::ImageData(std::vector<std::byte> pixelBytes,
                     uint32_t numLayers,
                     std::size_t pixelWidth,
                     std::size_t pixelHeight,
                     ImageData::PixelFormat pixelFormat)
    : m_pixelBytes(std::move(pixelBytes))
    , m_numLayers(numLayers)
    , m_pixelWidth(pixelWidth)
    , m_pixelHeight(pixelHeight)
    , m_pixelFormat(pixelFormat)
{
    assert(SanityCheckValues());
}

std::unique_ptr<NCommon::ImageData> ImageData::Clone() const
{
    return std::make_unique<ImageData>(
        m_pixelBytes,
        m_numLayers,
        m_pixelWidth,
        m_pixelHeight,
        m_pixelFormat
    );
}

uint8_t ImageData::GetBytesPerPixel() const
{
    switch (m_pixelFormat)
    {
        case PixelFormat::B8G8R8A8_SRGB:
        case PixelFormat::B8G8R8A8_LINEAR:
            return 4;
    }

    assert(false);
    return 0;
}

std::byte const* ImageData::GetPixelData(const uint32_t& layerIndex, const uintmax_t& pixelIndex) const
{
    assert(layerIndex < GetNumLayers());
    assert(pixelIndex < GetLayerNumPixels());

    const auto dataByteOffset = (layerIndex * GetLayerByteSize()) + pixelIndex * GetBytesPerPixel();

    return m_pixelBytes.data() + static_cast<long>(dataByteOffset);
}

bool ImageData::SanityCheckValues() const
{
    return m_pixelBytes.size() == m_pixelWidth * m_pixelHeight * m_numLayers * GetBytesPerPixel();
}

}
