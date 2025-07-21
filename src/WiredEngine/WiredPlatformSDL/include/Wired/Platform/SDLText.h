/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLTEXT_H
#define WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLTEXT_H

#include <Wired/Platform/IText.h>

#include <NEON/Common/SharedLib.h>

#include <mutex>
#include <unordered_map>
#include <vector>
#include <expected>

struct TTF_Font;

namespace NCommon
{
    class ILogger;
}

namespace Wired::Platform
{
    class NEON_PUBLIC SDLText : public IText
    {
        public:

            explicit SDLText(const NCommon::ILogger* pLogger);
            ~SDLText() override;

            //
            // IText
            //
            void Destroy() override;

            bool LoadFont(const std::string& fontName, std::span<const std::byte> fontData) override;
            [[nodiscard]] bool IsFontLoaded(const std::string& fontName) override;
            void UnloadFont(const std::string& fontName) override;
            void UnloadAllFonts() override;

            [[nodiscard]] std::expected<RenderedText, bool> RenderText(const std::string& text,  const std::string& fontName, const TextProperties& properties) override;

        private:

            struct Font
            {
                std::vector<std::byte> fontData;
                std::unordered_map<FontSize, TTF_Font*> fontSizes;
            };

        private:

            [[nodiscard]] std::expected<TTF_Font*, bool> EnsureFontSize(const std::string& fontName, FontSize fontSize);

        private:

            const NCommon::ILogger* m_pLogger;

            std::unordered_map<std::string, Font> m_fonts;
            std::mutex m_fontsMutex;
    };
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLTEXT_H
