/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SDLUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <cmath>

namespace Wired::Platform
{

std::unique_ptr<NCommon::ImageData> SDLSurfaceToImageData(const NCommon::ILogger* pLogger, SDL_Surface *pSurface, bool holdsLinearData)
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

SDL_Color ToSDLColor(Color color)
{
    return SDL_Color {.r = color.r, .g = color.g, .b = color.b, .a = color.a};
}

SDL_Surface* ResizeToPow2Dimensions(const NCommon::ILogger* pLogger, SDL_Surface *pSurface, SDL_Color fillColor)
{
    SDL_LockSurface(pSurface);

    const auto nextPow2Width = static_cast<unsigned int>(pow(2, ceil(log(pSurface->w)/log(2))));
    const auto nextPow2Height = static_cast<unsigned int>(pow(2, ceil(log(pSurface->h)/log(2))));

    // Create a new surface to hold the resized image
    const auto pResultSurface = SDL_CreateSurface((int)nextPow2Width, (int)nextPow2Height, pSurface->format);

    if (pResultSurface == nullptr)
    {
        pLogger->Error("ResizeToPow2Dimensions: Failed to create a new surface, error: {}", SDL_GetError());
        SDL_UnlockSurface(pSurface);
        return nullptr;
    }

    // Fill the newly created surface fully with a solid color
    const auto resultFillRect = SDL_Rect{
        .x = 0,
        .y = 0,
        .w = pResultSurface->w,
        .h = pResultSurface->h
    };

    const auto surfaceFormatDetails = SDL_GetPixelFormatDetails(pResultSurface->format);
    const auto surfaceFillColor = SDL_MapRGBA(surfaceFormatDetails, nullptr, fillColor.r, fillColor.g, fillColor.b, fillColor.a);
    SDL_FillSurfaceRect(pResultSurface, &resultFillRect, surfaceFillColor);

    // Copy the (smaller or equal) source surface to the top left corner of the new result surface
    const auto destRect = SDL_Rect{
        .x = 0,
        .y = 0,
        .w = pSurface->w,
        .h = pSurface->h
    };

    SDL_UnlockSurface(pSurface);

    if (!SDL_BlitSurface(pSurface, nullptr, pResultSurface, &destRect))
    {
        pLogger->Error("ResizeToPow2Dimensions: Failed to blit surface, error: {}", SDL_GetError());
        return nullptr;
    }

    return pResultSurface;
}

}
