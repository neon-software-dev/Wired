/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Platform/SDLImage.h>

#include <NEON/Common/Log/ILogger.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace Wired::Platform
{

std::unique_ptr<NCommon::ImageData> SDLSurfaceToImageData(NCommon::ILogger* pLogger,
                                                          SDL_Surface *pSurface,
                                                          bool holdsLinearData)
{
    SDL_Surface* pFormattedSurface = nullptr;
    bool surfaceConverted = false;

    // Lock the provided surface to read its data
    SDL_LockSurface(pSurface);

    // The renderer requires sampled textures to be in B8G8R8A8 format
    const SDL_PixelFormat desiredSDLPixelFormat = SDL_PIXELFORMAT_BGRA32;

    // Check if we need to convert the surface to a different format or if it can be used as-is
    if (pSurface->format == desiredSDLPixelFormat)
    {
        // Surface is already in a good format
        pFormattedSurface = pSurface;
    }
    else
    {
        // Convert the surface to RGBA32 as that's what the Renderer wants for textures
        pFormattedSurface = SDL_ConvertSurface(pSurface, desiredSDLPixelFormat);
        surfaceConverted = true;

        // Unlock the old surface as we're not using it any longer
        SDL_UnlockSurface(pSurface);

        if (pFormattedSurface == nullptr)
        {
            pLogger->Log(NCommon::LogLevel::Error,
             "SDLSurfaceToImageData: Surface could not be converted to a supported pixel format");
            return nullptr;
        }

        // Lock the new surface for reading its pixels
        SDL_LockSurface(pFormattedSurface);
    }

    // Byte size of the pixel data
    const unsigned int bytesPerPixel = 4;
    const uint32_t pixelDataByteSize = (uint32_t)pFormattedSurface->w * (uint32_t)pFormattedSurface->h * bytesPerPixel;

    // Copy the surface's pixel data into a vector
    std::vector<std::byte> imageBytes;
    imageBytes.reserve(pixelDataByteSize);
    imageBytes.insert(imageBytes.end(), (std::byte*)pFormattedSurface->pixels, (std::byte*)pFormattedSurface->pixels + pixelDataByteSize);

    NCommon::ImageData::PixelFormat pixelFormat = holdsLinearData ?
      NCommon::ImageData::PixelFormat::B8G8R8A8_LINEAR :
      NCommon::ImageData::PixelFormat::B8G8R8A8_SRGB;

    auto loadResult = std::make_unique<NCommon::ImageData>(
        imageBytes,
        1,
        static_cast<unsigned int>(pFormattedSurface->w),
        static_cast<unsigned int>(pFormattedSurface->h),
        pixelFormat
    );

    // Unlock the surface
    SDL_UnlockSurface(pFormattedSurface);

    // If we had to convert the surface format, free the converted surface we made
    if (surfaceConverted)
    {
        SDL_DestroySurface(pFormattedSurface);
    }

    return loadResult;
}

SDLImage::SDLImage(NCommon::ILogger* pLogger)
    : m_pLogger(pLogger)
{

}

SDLImage::~SDLImage()
{
    m_pLogger = nullptr;
}

std::expected<std::unique_ptr<NCommon::ImageData>, bool>
SDLImage::DecodeBytesAsImage(const std::vector<std::byte>& imageBytes,
                             const std::optional<std::string>& imageTypeHint,
                             bool holdsLinearData) const
{
    auto pRwOps = SDL_IOFromConstMem((void*)imageBytes.data(), imageBytes.size());

    const char* pImageTypeHint = imageTypeHint.has_value() ? imageTypeHint->c_str() : nullptr;

    SDL_Surface* pSurface = IMG_LoadTyped_IO(pRwOps, true, pImageTypeHint);
    if (pSurface == nullptr)
    {
        LogError("SDLImage::DecodeBytesAsImage: IMG_LoadTyped_IO failed, error: {}", SDL_GetError());
        return std::unexpected(false);
    }

    auto imageData = SDLSurfaceToImageData(m_pLogger, pSurface, holdsLinearData);
    if (imageData == nullptr)
    {
        LogError("SDLImage::DecodeBytesAsImage: SDLSurfaceToImageData failed");
        return std::unexpected(false);
    }

    SDL_DestroySurface(pSurface);

    return imageData;
}

}
