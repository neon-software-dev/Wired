/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_PACKAGEWINDOW_H
#define WIREDEDITOR_WINDOW_PACKAGEWINDOW_H

#include "../ViewModel/MainWindowVM.h"

#include <imgui.h>

namespace Wired
{
    static constexpr auto PACKAGE_WINDOW = "Package###PackageWindow";

    void PackageWindow(MainWindowVM* vm)
    {
        ImGui::Begin(PACKAGE_WINDOW);

        const auto& package = vm->GetPackage();
        if (!package)
        {
            ImGui::Text("No package loaded");
            ImGui::End();
            return;
        }

        if (ImGui::BeginTabBar("TabBar"))
        {
            if (ImGui::BeginTabItem("Details"))
            {
                ImGui::Text("Package: %s", package->manifest.packageName.c_str());
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }
}

#endif //WIREDEDITOR_WINDOW_PACKAGEWINDOW_H
