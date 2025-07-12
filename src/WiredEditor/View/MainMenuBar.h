/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_VIEW_MAINMENUBAR_H
#define WIREDEDITOR_VIEW_MAINMENUBAR_H

#include "../PopUp/NewPackageDialog.h"

#include "../ViewModel/MainWindowVM.h"

#include <Wired/Engine/IEngineAccess.h>

#include <SDL3/SDL_dialog.h>

#include <imgui.h>

namespace Wired
{
    static constexpr auto New_PACKAGE = "New Package";
    static constexpr auto OPEN_PACKAGE = "Open Package";
    static constexpr auto SAVE_PACKAGE = "Save Package";
    static constexpr auto CLOSE_PACKAGE = "Close Package";

    static void MainMenuBar(Engine::IEngineAccess* engine, MainWindowVM* vm)
    {
        std::string menuItemClicked;

        //
        // Create menu UI
        //
        const bool packageIsOpened = vm->GetPackage().has_value();

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Package")) { menuItemClicked = New_PACKAGE; }
                if (ImGui::MenuItem("Open Package")) { menuItemClicked = OPEN_PACKAGE; }
                if (ImGui::MenuItem("Save Package", nullptr, false, packageIsOpened)) { menuItemClicked = SAVE_PACKAGE; }
                if (ImGui::MenuItem("Close Package", nullptr, false, packageIsOpened)) { menuItemClicked = CLOSE_PACKAGE; }
                if (ImGui::MenuItem("Quit")) { engine->Quit(); }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        //
        // Handle menu item clicks
        //
        if (menuItemClicked == New_PACKAGE)
        {
            ImGui::OpenPopup(NEW_PACKAGE_DIALOG);
        }
        else if (menuItemClicked == OPEN_PACKAGE)
        {
            const SDL_DialogFileFilter filters[] = { { "Wired Packages",  "wpk" } };

            SDL_ShowOpenFileDialog([](void* pUserData, const char* const* filelist, int){
                if (filelist != nullptr && *filelist != nullptr) {
                    ((MainWindowVM*)pUserData)->OnOpenPackage(*filelist);
                }
            }, (void*)vm, nullptr, filters, 1, nullptr, false);
        }
        else if (menuItemClicked == SAVE_PACKAGE)
        {
            vm->OnSavePackage();
        }
        else if (menuItemClicked == CLOSE_PACKAGE)
        {
            vm->OnClosePackage();
        }

        //
        // Create opened dialog UI
        //
        if (ImGui::IsPopupOpen(NEW_PACKAGE_DIALOG))
        {
            const auto result = NewPackageDialog();
            if (result && result->doCreateNewPackage)
            {
                vm->OnCreateNewPackage(result->packageName, result->packageDirectory);
            }
        }
    }
}

#endif //WIREDEDITOR_VIEW_MAINMENUBAR_H
