/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLIMAGE_H
#define WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLIMAGE_H

#include <Wired/Platform/IImage.h>

#include <NEON/Common/SharedLib.h>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Platform
{
    class NEON_PUBLIC SDLImage : public IImage
    {
        public:

            explicit SDLImage(NCommon::ILogger* pLogger);
            ~SDLImage() override;

            [[nodiscard]] std::expected<std::unique_ptr<NCommon::ImageData>, bool>
                DecodeBytesAsImage(const std::vector<std::byte>& imageBytes,
                                   const std::optional<std::string>& imageTypeHint,
                                   bool holdsLinearData) const override;

        private:

            NCommon::ILogger* m_pLogger;
    };
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLIMAGE_H
