/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Platform/SDLText.h>

#include "SDLUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <SDL3/SDL_iostream.h>
#include <SDL3_ttf/SDL_ttf.h>

namespace Wired::Platform
{

SDLText::SDLText(const NCommon::ILogger* pLogger)
    : m_pLogger(pLogger)
{

}

SDLText::~SDLText()
{
    m_pLogger = nullptr;
}

void SDLText::Destroy()
{
    UnloadAllFonts();
}

bool SDLText::LoadFont(const std::string& fontName, std::span<const std::byte> fontData)
{
    if (m_fonts.contains(fontName))
    {
        return true;
    }

    std::lock_guard<std::mutex> lock(m_fontsMutex);

    m_fonts.insert({fontName, Font{.fontData = {fontData.begin(), fontData.end()}, .fontSizes = {}}});

    return true;
}

bool SDLText::IsFontLoaded(const std::string& fontName)
{
    return m_fonts.contains(fontName);
}

void SDLText::UnloadFont(const std::string& fontName)
{
    std::lock_guard<std::mutex> lock(m_fontsMutex);

    const auto it = m_fonts.find(fontName);
    if (it == m_fonts.cend())
    {
        return;
    }

    for (const auto& fontSize : it->second.fontSizes)
    {
        TTF_CloseFont(fontSize.second);
    }

    m_fonts.erase(it);
}

void SDLText::UnloadAllFonts()
{
    while (!m_fonts.empty())
    {
        UnloadFont(m_fonts.cbegin()->first);
    }
}

std::expected<RenderedText, bool> SDLText::RenderText(const std::string& text,  const std::string& fontName, const TextProperties& properties)
{
    const auto pFont = EnsureFontSize(fontName, properties.fontSize);
    if (!pFont)
    {
        LogError("SDLText::RenderText: Failed to ensure font size: {}", fontName);
        return std::unexpected(false);
    }

    const auto fgColor = ToSDLColor(properties.fgColor);
    const auto bgColor = ToSDLColor(properties.bgColor);

    SDL_Surface *pRenderedTextSurface;

    if (properties.wrapLength == 0)
    {
        pRenderedTextSurface = TTF_RenderText_Blended(*pFont, text.c_str(), 0, fgColor);
    }
    else
    {
        pRenderedTextSurface = TTF_RenderText_Blended_Wrapped(*pFont, text.c_str(), 0, fgColor, (int)properties.wrapLength);
    }

    if (pRenderedTextSurface == nullptr)
    {
        LogError("SDLText::RenderText: Failed to render text, error: {}", SDL_GetError());
        return std::unexpected(false);
    }

    RenderedText renderedText{};
    renderedText.textPixelWidth = (uint32_t)pRenderedTextSurface->w;
    renderedText.textPixelHeight = (uint32_t)pRenderedTextSurface->h;

    // Resize the surface so that it can be used as a texture
    SDL_Surface *pResizedSurface = ResizeToPow2Dimensions(m_pLogger, pRenderedTextSurface, bgColor);
    SDL_DestroySurface(pRenderedTextSurface);

    if (pResizedSurface == nullptr)
    {
        m_pLogger->Error("SDLText::RenderText: Failed to resize surface to power of two");
        return std::unexpected(false);
    }

    renderedText.imageData = SDLSurfaceToImageData(m_pLogger, pResizedSurface, false);
    SDL_DestroySurface(pResizedSurface);

    return renderedText;
}

std::expected<TTF_Font*, bool> SDLText::EnsureFontSize(const std::string& fontName, FontSize fontSize)
{
    std::lock_guard<std::mutex> lock(m_fontsMutex);

    const auto fontIt = m_fonts.find(fontName);
    if (fontIt == m_fonts.cend())
    {
        LogError("SDLText::EnsureFontSize: Font is not loaded: {}", fontName);
        return std::unexpected(false);
    }

    const auto sizeIt = fontIt->second.fontSizes.find(fontSize);
    if (sizeIt != fontIt->second.fontSizes.cend())
    {
        return sizeIt->second;
    }

    const auto pFont = TTF_OpenFontIO(SDL_IOFromConstMem(fontIt->second.fontData.data(), fontIt->second.fontData.size()), true, (float)fontSize);
    if (pFont == nullptr)
    {
        LogError("SDLText::EnsureFontSize: Call to TTF_OpenFontIO failed for font: {}", fontName);
        return std::unexpected(false);
    }

    fontIt->second.fontSizes.insert({fontSize, pFont});

    return pFont;
}

}
