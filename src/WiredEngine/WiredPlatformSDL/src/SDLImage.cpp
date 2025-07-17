/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Platform/SDLImage.h>

#include "SDLUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace Wired::Platform
{

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
