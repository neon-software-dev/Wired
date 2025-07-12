/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_NODEEDITORWINDOW_H
#define WIREDEDITOR_WINDOW_NODEEDITORWINDOW_H

namespace Wired
{
    class EditorResources;
    class MainWindowVM;

    static constexpr auto NODE_EDITOR_WINDOW = "Node Editor###NodeEditorWindow";

    void NodeEditorWindow(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM);
}

#endif //WIREDEDITOR_WINDOW_NODEEDITORWINDOW_H
