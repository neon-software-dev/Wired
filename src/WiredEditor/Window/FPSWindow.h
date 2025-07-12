/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_FPSWINDOW_H
#define WIREDEDITOR_WINDOW_FPSWINDOW_H

#include <imgui.h>

namespace Wired
{
    static void FPSWindow()
    {
        ImGui::SetNextWindowBgAlpha(0.3f);
        ImGui::SetNextWindowPos(ImVec2(0.0f, ImGui::GetMainViewport()->Size.y), 0, ImVec2(0.0f, 1.0f));
        ImGui::Begin("FPSWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::Text("%.3f (%.0f FPS) ", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }
}

#endif //WIREDEDITOR_WINDOW_FPSWINDOW_H
