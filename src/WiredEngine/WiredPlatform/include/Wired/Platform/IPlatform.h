/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLATFORM_IPLATFORM_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLATFORM_IPLATFORM_H

#include "IWindow.h"
#include "IEvents.h"
#include "IFiles.h"
#include "IImage.h"
#include "IText.h"

namespace Wired::Platform
{
    class IPlatform
    {
        public:

            virtual ~IPlatform() = default;

            [[nodiscard]] virtual IWindow* GetWindow() const = 0;
            [[nodiscard]] virtual IEvents* GetEvents() const = 0;
            [[nodiscard]] virtual IFiles* GetFiles() const = 0;
            [[nodiscard]] virtual IImage* GetImage() const = 0;
            [[nodiscard]] virtual IText* GetText() const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLATFORM_IPLATFORM_H
