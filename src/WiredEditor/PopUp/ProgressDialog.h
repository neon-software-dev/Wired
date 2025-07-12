/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_POPUP_PROGRESSDIALOG_H
#define WIREDEDITOR_POPUP_PROGRESSDIALOG_H

#include "../ViewModel/MainWindowVM.h"

#include <NEON/Common/Build.h>

#include <imgui.h>

#include <string>
#include <optional>
#include <utility>
#include <functional>
#include <format>
#include <cassert>

namespace Wired
{
    struct ProgressDialogContents
    {
        // Main message displayed above the progress indicator
        std::string message;

        // For indeterminate progress: If supplied, will display this text on top
        // of the progress indicator. If not supplied, will say "Working..." instead
        std::optional<std::string> indeterminateProgressText;

        // If supplied, turns the progress bar into a determinate progress bar
        std::optional<std::pair<unsigned int, unsigned int>> progress;

        // If supplied, will display a cancel button which will invoke this callback
        // when pressed
        std::optional<std::function<void()>> cancelFunc;
    };

    static constexpr auto PROGRESS_DIALOG = "ProgressDialog";

    /**
     * Displays a modal progress dialog for determinate or indeterminate progress.
     *
     * @param contents The details to be shown, or std::nullopt to close the dialog
     */
    inline void ProgressDialog(const std::optional<ProgressDialogContents>& contents)
    {
        static bool cancelPressed = false;

        if (ImGui::BeginPopupModal(PROGRESS_DIALOG, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
        {
            //
            // If we were given empty contents then this dialog is no longer wanted, destroy it
            //
            if (!contents.has_value())
            {
                // Reset static state
                cancelPressed = false;

                // Close and end the dialog
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                return;
            }

            //
            // Main progress dialog text/message
            //
            ImGui::Text("%s", contents->message.c_str());

            ImGui::Separator();

            //
            // Progress Indicator
            //
            float progressFraction{0.0f};
            std::string progressText;

            // Determinate
            if (contents->progress)
            {
                assert(contents->progress->second != 0);
                progressFraction = (float)contents->progress->first / (float)contents->progress->second;
                progressText = std::format("{}/{}", contents->progress->first, contents->progress->second);
            }
            // Indeterminate
            else
            {
                progressFraction = -1.0f * (float)ImGui::GetTime();

                progressText = "Working...";
                if (contents->indeterminateProgressText)
                {
                    progressText = *contents->indeterminateProgressText;
                }
            }

            ImGui::ProgressBar(progressFraction, ImVec2(0.0f, 0.0f), progressText.c_str());

            //
            // Create a cancel button if a cancel function was supplied
            //
            if (contents->cancelFunc.has_value())
            {
                // Disable the cancel button if it has already been clicked
                ImGui::BeginDisabled(cancelPressed);

                    const std::string cancelText = cancelPressed ? "Cancelling" : "Cancel";

                    if (ImGui::Button(cancelText.c_str()))
                    {
                        (*contents->cancelFunc)();
                        cancelPressed = true;
                    }

                ImGui::EndDisabled();
            }

            ImGui::EndPopup();
        }
    }
}

#endif //WIREDEDITOR_POPUP_PROGRESSDIALOG_H
