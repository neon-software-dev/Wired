/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_FONT_FONTMANAGER_H
#define WIREDENGINE_WIREDENGINE_SRC_FONT_FONTMANAGER_H

#include <Wired/Engine/ResourceIdentifier.h>

#include <Wired/Platform/Text.h>

#include <span>
#include <vector>
#include <unordered_map>
#include <expected>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Platform
{
    class IText;
}

namespace Wired::Engine
{
    class FontManager
    {
        public:

            FontManager(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, Platform::IText* pText);
            ~FontManager();

            bool Startup();
            void Shutdown();
            void DestroyAll();


            [[nodiscard]] bool LoadResourceFont(const ResourceIdentifier& resourceIdentifier, std::span<const std::byte> fontData);
            [[nodiscard]] bool IsResourceFontLoaded(const ResourceIdentifier& resourceIdentifier);
            void DestroyResourceFont(const ResourceIdentifier& resourceIdentifier);

            [[nodiscard]] std::expected<Platform::RenderedText, bool> RenderText(const std::string& text,
                                                                                 const ResourceIdentifier& font,
                                                                                 const Platform::TextProperties& properties);

        private:

            const NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;
            Platform::IText* m_pText;
    };
}


#endif //WIREDENGINE_WIREDENGINE_SRC_FONT_FONTMANAGER_H
