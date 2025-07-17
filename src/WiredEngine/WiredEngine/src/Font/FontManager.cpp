/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FontManager.h"

#include <Wired/Platform/IText.h>

#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Metrics/IMetrics.h>

namespace Wired::Engine
{

FontManager::FontManager(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, Platform::IText* pText)
    : m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_pText(pText)
{

}

FontManager::~FontManager()
{
    m_pLogger = nullptr;
    m_pMetrics = nullptr;
    m_pText = nullptr;
}

bool FontManager::Startup()
{
    LogInfo("FontManager: Starting Up");

    return true;
}

void FontManager::Shutdown()
{
    LogInfo("FontManager shutting down");

    m_pText->Destroy();
}

void FontManager::DestroyAll()
{
    m_pText->UnloadAllFonts();
}

bool FontManager::LoadResourceFont(const ResourceIdentifier& resourceIdentifier, std::span<const std::byte> fontData)
{
    LogInfo("FontManager: Loading resource font: {}", resourceIdentifier.GetUniqueName());

    return m_pText->LoadFont(resourceIdentifier.GetUniqueName(), fontData);
}

bool FontManager::IsResourceFontLoaded(const ResourceIdentifier& resourceIdentifier)
{
    return m_pText->IsFontLoaded(resourceIdentifier.GetUniqueName());
}

void FontManager::DestroyResourceFont(const ResourceIdentifier& resourceIdentifier)
{
    LogInfo("FontManager: Destroying resource font: {}", resourceIdentifier.GetUniqueName());

    m_pText->UnloadFont(resourceIdentifier.GetUniqueName());
}

std::expected<Platform::RenderedText, bool> FontManager::RenderText(const std::string& text,
                                                                    const ResourceIdentifier& font,
                                                                    const Platform::TextProperties& properties)
{
    return m_pText->RenderText(text, font.GetUniqueName(), properties);
}

}
