/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_MAINWINDOW_H
#define WIREDEDITOR_WINDOW_MAINWINDOW_H

#include "../ViewModel/MainWindowVM.h"
#include "../ViewModel/AssetsWindowVM.h"

#include <imgui.h>

#include <memory>

namespace Wired
{
    class EditorResources;
    class ViewportWindow;

    namespace Engine
    {
        class IEngineAccess;
    }

    class MainWindow
    {
        public:

            MainWindow(Engine::IEngineAccess* pEngine, EditorResources* pEditorResources);
            ~MainWindow();

            void operator()();

            [[nodiscard]] MainWindowVM const* GetVM() const { return m_vm.get(); }
            [[nodiscard]] AssetsWindowVM const* GetAssetsWindowVM() const { return m_assetsWindowVM.get(); }
            [[nodiscard]] ViewportWindow const* GetViewportWindow() const { return m_viewportWindow.get(); }

        private:

            Engine::IEngineAccess* m_pEngine;
            EditorResources* m_pEditorResources;

            std::unique_ptr<AssetsWindowVM> m_assetsWindowVM;
            std::unique_ptr<MainWindowVM> m_vm;

            std::unique_ptr<ViewportWindow> m_viewportWindow;
    };
}

#endif //WIREDEDITOR_WINDOW_MAINWINDOW_H
