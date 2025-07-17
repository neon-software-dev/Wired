/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_TEXT_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_TEXT_H

#include "Color.h"

#include <NEON/Common/ImageData.h>

#include <memory>

namespace Wired::Platform
{
    struct RenderedText
    {
        std::unique_ptr<NCommon::ImageData> imageData; // The rendered text's image data
        uint32_t textPixelWidth{0}; // Pixel width of the text, within the rendered image
        uint32_t textPixelHeight{0}; // Pixel height of the text, within the rendered image
    };

    using FontSize = uint16_t;

    struct TextProperties
    {
        FontSize fontSize{0}; // The font size to use
        uint32_t wrapLength{0}; // At what pixel width the text should be wrapped, or 0 for no wrapping
        Color fgColor{Color::Black()}; // The text's foreground color
        Color bgColor{Color::White()}; // The text's background color
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_TEXT_H
