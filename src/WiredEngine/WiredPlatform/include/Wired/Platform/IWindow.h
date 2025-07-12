/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLATFORM_IWINDOW_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLATFORM_IWINDOW_H

#include <Wired/GPU/GPUCommon.h>

#include <NEON/Common/Space/Size2D.h>

#include <expected>

namespace Wired::Platform
{
    class IWindow
    {
        public:

            virtual ~IWindow() = default;

            [[nodiscard]] virtual std::expected<NCommon::Size2DUInt, bool> GetWindowPixelSize() const = 0;
            [[nodiscard]] virtual GPU::ShaderBinaryType GetShaderBinaryType() const = 0;
            virtual void SetMouseCapture(bool doCaptureMouse) const = 0;
            [[nodiscard]] virtual bool IsCapturingMouse() const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLATFORM_IWINDOW_H
