/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_SCENEWINDOW_H
#define WIREDEDITOR_WINDOW_SCENEWINDOW_H

#include "../ViewModel/MainWindowVM.h"

#include <imgui.h>

namespace Wired
{
    class EditorResources;

    static constexpr auto SCENE_WINDOW = "Scene###SceneWindow";

    void SceneWindow(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM);
}

#endif //WIREDEDITOR_WINDOW_SCENEWINDOW_H
