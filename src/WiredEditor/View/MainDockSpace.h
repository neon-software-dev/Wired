/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_VIEW_MAINDOCKSPACE_H
#define WIREDEDITOR_VIEW_MAINDOCKSPACE_H

#include "../Window/AssetsWindow.h"
#include "../Window/AssetViewWindow.h"
#include "../Window/SceneWindow.h"
#include "../Window/NodeEditorWindow.h"
#include "../Window/ViewportWindow.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace Wired
{
    static void MainDockSpace()
    {
        const auto viewport = ImGui::GetMainViewport();

        ImGuiID dockId = ImGui::GetID("MainDockSpace");
        const bool init = ImGui::DockBuilderGetNode(dockId) == nullptr;
        if (init)
        {
            ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockId, viewport->WorkSize);

            ImGuiID dockIdLeft = 0, dockIdRight = 0, dockIdCenter = 0, dockIdBottom = 0,
                dockIdBottomLeft = 0, dockIdBottomRight = 0;

            ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Left, 0.15f, &dockIdLeft, &dockIdCenter);
            ImGui::DockBuilderSplitNode(dockIdCenter, ImGuiDir_Right, 0.17647f, &dockIdRight, &dockIdCenter);
            ImGui::DockBuilderSplitNode(dockIdCenter, ImGuiDir_Up, 0.75f, &dockIdCenter, &dockIdBottom);
            ImGui::DockBuilderSplitNode(dockIdBottom, ImGuiDir_Left, 0.50f, &dockIdBottomLeft, &dockIdBottomRight);

            ImGui::DockBuilderDockWindow(SCENE_WINDOW, dockIdLeft);
            ImGui::DockBuilderDockWindow(VIEWPORT_WINDOW, dockIdCenter);
            ImGui::DockBuilderDockWindow(ASSETS_WINDOW, dockIdBottomLeft);
            ImGui::DockBuilderDockWindow(ASSET_VIEW_WINDOW, dockIdBottomRight);
            ImGui::DockBuilderDockWindow(NODE_EDITOR_WINDOW, dockIdRight);

            ImGui::DockBuilderFinish(dockId);
        }

        ImGui::DockSpaceOverViewport(dockId, viewport);
    }
}

#endif //WIREDEDITOR_VIEW_MAINDOCKSPACE_H
