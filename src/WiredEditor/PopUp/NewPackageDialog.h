/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_POPUP_NEWPACKAGEDIALOG_H
#define WIREDEDITOR_POPUP_NEWPACKAGEDIALOG_H

#include <imgui.h>
#include <imgui_stdlib.h>

#include <SDL3/SDL_dialog.h>

#include <optional>
#include <string>

namespace Wired
{
    static constexpr auto NEW_PACKAGE_DIALOG = "New Package";

    struct NewPackageDialogResult
    {
        bool doCreateNewPackage{false};
        std::string packageName;
        std::string packageDirectory;
    };

    [[nodiscard]] static std::optional<NewPackageDialogResult> NewPackageDialog()
    {
        static std::string chosenPackageName;
        static std::string chosenDirectory{};

        std::optional<NewPackageDialogResult> result;

        if (ImGui::BeginPopupModal(NEW_PACKAGE_DIALOG, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Package Name", &chosenPackageName);

            ////

            if (ImGui::Button("Choose"))
            {
                SDL_ShowOpenFolderDialog([](void*, const char* const* filelist, int){
                    if (filelist != nullptr && *filelist != nullptr)
                    {
                        chosenDirectory = std::string(*filelist);
                    }
                }, nullptr, nullptr, nullptr, false);
            }

            ImGui::SameLine(0.0f, 20.0f);

            ImGui::Text("%s", chosenDirectory.c_str());

            ImGui::SameLine(0.0f, 20.0f);

            ImGui::Text("Location");

            ////

            if (ImGui::Button("Create"))
            {
                // Create result
                result = NewPackageDialogResult{};
                result->doCreateNewPackage = true;
                result->packageName = chosenPackageName;
                result->packageDirectory = chosenDirectory;

                // Reset static state
                chosenPackageName = {};
                chosenDirectory = {};

                // Close the dialog
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Close"))
            {
                // Create result
                result = NewPackageDialogResult{};
                result->doCreateNewPackage = false;

                // Reset static state
                chosenPackageName = {};
                chosenDirectory = {};

                // Close the dialog
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return result;
    }
}

#endif //WIREDEDITOR_POPUP_NEWPACKAGEDIALOG_H
