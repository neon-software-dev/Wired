/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_ITEXT_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_ITEXT_H

#include "Text.h"

#include <string>
#include <span>
#include <expected>

namespace Wired::Platform
{
    class IText
    {
        public:

            virtual ~IText() = default;

            virtual void Destroy() = 0;

            virtual bool LoadFont(const std::string& fontName, std::span<const std::byte> fontData) = 0;
            [[nodiscard]] virtual bool IsFontLoaded(const std::string& fontName) = 0;
            virtual void UnloadFont(const std::string& fontName) = 0;
            virtual void UnloadAllFonts() = 0;

            [[nodiscard]] virtual std::expected<RenderedText, bool> RenderText(const std::string& text,
                                                                               const std::string& fontName,
                                                                               const TextProperties& properties) = 0;
    };
}


#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_ITEXT_H
