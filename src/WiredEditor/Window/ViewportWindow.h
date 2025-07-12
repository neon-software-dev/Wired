/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_VIEWPORTWINDOW_H
#define WIREDEDITOR_WINDOW_VIEWPORTWINDOW_H

#include <Wired/Engine/EngineCommon.h>
#include <Wired/Engine/World/Camera2D.h>
#include <Wired/Engine/World/Camera3D.h>

#include <Wired/Render/Id.h>

#include <imgui.h>

#include <optional>
#include <memory>

namespace Wired
{
    class EditorResources;
    class MainWindowVM;
    class RenderOutputView;

    namespace Engine
    {
        class IEngineAccess;
    }

    static constexpr auto VIEWPORT_WINDOW = "Viewport###ViewPortWindow";

    static constexpr auto VIEWPORT_MAX_2D_SCALE = 10.0f;
    static constexpr auto VIEWPORT_MIN_2D_SCALE = 0.2f;

    class ViewportWindow
    {
        public:

            ViewportWindow(Engine::IEngineAccess* pEngine,
                           EditorResources* pEditorResources,
                           MainWindowVM* pMainWindowVM);
            ~ViewportWindow();

            void operator()(Render::TextureId textureId);

        private:

            void ViewportTopToolbar();
            void ViewportBottomToolbar();

            void HandleViewportScrollWheel();
            void HandleViewportMouseMovement();
            void HandleViewportKeyEvents();

            void ZoomCamera2D(Engine::Camera2D* pCamera, float delta);

        private:

            Engine::IEngineAccess* m_pEngine;
            EditorResources* m_pEditorResources;
            MainWindowVM* m_pMainWindowVM;
            std::unique_ptr<RenderOutputView> m_renderOutputView;
    };
}

#endif //WIREDEDITOR_WINDOW_VIEWPORTWINDOW_H
