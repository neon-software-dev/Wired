/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_POPUP_NEWSCENEDIALOG_H
#define WIREDEDITOR_POPUP_NEWSCENEDIALOG_H

#include <imgui.h>
#include <imgui_stdlib.h>

#include <SDL3/SDL_dialog.h>

#include <optional>
#include <string>

namespace Wired
{
    static constexpr auto NEW_SCENE_DIALOG = "New Scene";

    struct NewSceneDialogResult
    {
        bool doCreateNewScene{false};
        std::string sceneName;
    };

    [[nodiscard]] static std::optional<NewSceneDialogResult> NewSceneDialog()
    {
        static std::string chosenSceneName;

        std::optional<NewSceneDialogResult> result;

        if (ImGui::BeginPopupModal(NEW_SCENE_DIALOG, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Scene Name", &chosenSceneName);

            if (ImGui::Button("Create"))
            {
                // Create result
                result = NewSceneDialogResult{};
                result->doCreateNewScene = true;
                result->sceneName = chosenSceneName;

                // Reset static state
                chosenSceneName = {};

                // Close the dialog
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Close"))
            {
                // Create result
                result = NewSceneDialogResult{};
                result->doCreateNewScene = false;

                // Reset static state
                chosenSceneName = {};

                // Close the dialog
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        return result;
    }
}

#endif //WIREDEDITOR_POPUP_NEWSCENEDIALOG_H
