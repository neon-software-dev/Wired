/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLUTIL_H
#define WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLUTIL_H

#include <Wired/Platform/Color.h>

#include <NEON/Common/ImageData.h>
#include <NEON/Common/SharedLib.h>

#include <SDL3/SDL.h>

#include <memory>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Platform
{
    /**
     * Converts an SDL_Surface to a BGRA32-formatted ImageData
     */
    [[nodiscard]] NEON_LOCAL std::unique_ptr<NCommon::ImageData> SDLSurfaceToImageData(
        const NCommon::ILogger* pLogger,
        SDL_Surface *pSurface,
        bool holdsLinearData);

    /**
     * Converts a Platform Color to an SDL_Color
     */
    [[nodiscard]] NEON_LOCAL SDL_Color ToSDLColor(Color color);

    /**
     * Returns a new surface which contains the supplied surface's pixels but with the surface's
     * dimensions either left the same or adjusted upwards to be a power of two. Does not necessarily
     * return a square surface, only a surface with power of two dimensions.
     *
     * For example, a 110x512 image would be converted to 128x512.
     *
     * The source surface is left unmodified.
     *
     * The result surface is in an RGBA32 format where the extra space that doesn't contain the old
     * surface's pixels is filled with the specified fill pixel color.
     *
     * @param logger Logger to receive any error messages
     * @param pSurface The surface to be resized.
     * @param fillColor The color to fill the background of the new surface with
     *
     * @return A new surface, with power of two dimensions, containing the supplied
     * surface's pixel data, or null on error.
     */
    [[nodiscard]] NEON_LOCAL SDL_Surface* ResizeToPow2Dimensions(const NCommon::ILogger* pLogger, SDL_Surface *pSurface, SDL_Color fillColor);
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_SRC_SDLUTIL_H
