/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_PLATFORM_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_PLATFORM_H

#include "IPlatform.h"

#include <memory>

namespace Wired::Platform
{
    class Platform : public IPlatform
    {
        public:

            Platform(std::shared_ptr<IWindow> window,
                     std::unique_ptr<IEvents> events,
                     std::unique_ptr<IFiles> files,
                     std::unique_ptr<IImage> image)
                : m_window(std::move(window))
                , m_events(std::move(events))
                , m_files(std::move(files))
                , m_image(std::move(image))
            { }

            ~Platform() override = default;

            [[nodiscard]] IWindow* GetWindow() const override { return m_window.get(); };
            [[nodiscard]] IEvents* GetEvents() const override { return m_events.get(); }
            [[nodiscard]] IFiles* GetFiles() const override { return m_files.get(); }
            [[nodiscard]] IImage* GetImage() const override { return m_image.get(); }

        private:

            std::shared_ptr<IWindow> m_window;
            std::unique_ptr<IEvents> m_events;
            std::unique_ptr<IFiles> m_files;
            std::unique_ptr<IImage> m_image;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_PLATFORM_H
