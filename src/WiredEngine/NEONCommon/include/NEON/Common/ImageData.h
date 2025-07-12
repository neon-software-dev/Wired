/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_IMAGEDATA_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_IMAGEDATA_H

#include <NEON/Common/SharedLib.h>

#include <vector>
#include <memory>
#include <cstdint>
#include <cstddef>

namespace NCommon
{
    /**
     * Contains the data associated with a 2D image: pixels, a pixel format, and a width/height.
     *
     * Note that the pixel data is required to be in linear color space.
     */
    class NEON_PUBLIC ImageData
    {
        public:

            enum class PixelFormat
            {
                /**
                 *  A four-component, 32-bit unsigned normalized format that has an 8-bit R component stored with sRGB
                 *  nonlinear encoding in byte 0, an 8-bit G component stored with sRGB nonlinear encoding in byte 1,
                 *  an 8-bit B component stored with sRGB nonlinear encoding in byte 2, and an 8-bit A component in byte 3.
                 */
                B8G8R8A8_SRGB,

                B8G8R8A8_LINEAR
            };

        public:

            /**
             * @param pixelBytes        The image's raw byte data
             * @param numLayers         Number of width x height layers in the data
             * @param pixelWidth        The pixel width of the image
             * @param pixelHeight       The pixel height of the image
             * @param pixelFormat       The pixel format the image data uses
             */
            ImageData(
                std::vector<std::byte> pixelBytes,
                uint32_t numLayers,
                std::size_t pixelWidth,
                std::size_t pixelHeight,
                PixelFormat pixelFormat);

            [[nodiscard]] std::unique_ptr<NCommon::ImageData> Clone() const;

            /**
             * @return A pointer to the raw bytes that make up the image
             */
            [[nodiscard]] std::byte const* GetPixelData() const noexcept { return m_pixelBytes.data(); }

            /**
            * @return A pointer to the raw bytes that make up the image, starting at the specified layer+pixel
            */
            [[nodiscard]] std::byte const* GetPixelData(const uint32_t& layerIndex, const uintmax_t& pixelIndex) const;

            /**
            * @return The number of layers in the image
            */
            [[nodiscard]] uint32_t GetNumLayers() const noexcept { return m_numLayers; }

            /**
             * @return The width, in pixels, of the image
             */
            [[nodiscard]] std::size_t GetPixelWidth() const noexcept { return m_pixelWidth; }

            /**
             * @return The height, in pixels, of the image
             */
            [[nodiscard]] std::size_t GetPixelHeight() const noexcept { return m_pixelHeight; }

            /**
             * @return The format defining the elements of each pixel
             */
            [[nodiscard]] PixelFormat GetPixelFormat() const noexcept { return m_pixelFormat; }

            /**
             * @return The total number of pixels in one layer of the image
             */
            [[nodiscard]] std::size_t GetLayerNumPixels() const noexcept { return m_pixelWidth * m_pixelHeight; }

            /**
             * @return The total byte size of one layer of the image
             */
            [[nodiscard]] uint64_t GetLayerByteSize() const noexcept { return GetLayerNumPixels() * GetBytesPerPixel(); }

            /**
             * @return The total byte size of the image
             */
            [[nodiscard]] uint64_t GetTotalByteSize() const noexcept { return m_pixelBytes.size(); }

            /**
             * @return The number of bytes which make up one pixel
             */
            [[nodiscard]] uint8_t GetBytesPerPixel() const;

        private:

            [[nodiscard]] bool SanityCheckValues() const;

        private:

            std::vector<std::byte> m_pixelBytes;        // Sequence of bytes representing individual RGB/RGBA/etc components
            uint32_t m_numLayers;                       // Number of width x height layers in the data
            std::size_t m_pixelWidth;                   // Width of the image, in pixels
            std::size_t m_pixelHeight;                  // Height of the image, in pixels
            PixelFormat m_pixelFormat;                  // Pixel format of the image.
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_IMAGEDATA_H
