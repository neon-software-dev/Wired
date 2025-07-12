/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SceneWindow.h"

#include "../PopUp/NewSceneDialog.h"

#include "../EditorResources.h"

namespace Wired
{

void SceneToolbar(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM)
{
    const auto selectedScene = pMainWindowVM->GetSelectedScene();
    const auto toolbarButtonSize = pEditorResources->GetToolbarActionButtonSize();

    ImGui::BeginChild("SceneToolBar", {0.0, 0.0f}, ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

        //
        // Scene Select ComboBox
        //
        std::string comboPreview = "Scene Select";
        if (selectedScene)
        {
            comboPreview = (*selectedScene)->name;
        }

        if (ImGui::BeginCombo("###SceneCombo", comboPreview.c_str(), ImGuiComboFlags_HeightLarge))
        {
            for (const auto& scene : pMainWindowVM->GetPackage()->scenes)
            {
                const bool sceneIsSelected = selectedScene && ((*selectedScene)->name == scene->name);

                if (ImGui::Selectable(scene->name.c_str(), sceneIsSelected))
                {
                    pMainWindowVM->OnSceneSelected(scene->name);
                }

                if (sceneIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        //
        // Add Scene Button
        //
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - toolbarButtonSize.x);

        if (ImGui::ImageButton(
            "SceneToolBarAddButton",
            pEditorResources->CreateTextureReference("add.png", Render::DefaultSampler::LinearClamp),
            toolbarButtonSize
        ))
        {
            ImGui::OpenPopup(NEW_SCENE_DIALOG);
        }

        if (ImGui::IsPopupOpen(NEW_SCENE_DIALOG))
        {
            const auto result = NewSceneDialog();
            if (result && result->doCreateNewScene)
            {
                pMainWindowVM->OnCreateNewScene(result->sceneName);
            }
        }
    ImGui::EndChild();
}

void SceneNodesToolbar(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM)
{
    const auto hasSelectedScene = pMainWindowVM->GetSelectedScene().has_value();
    const auto toolbarButtonSize = pEditorResources->GetToolbarActionButtonSize();

    ImGui::BeginDisabled(!hasSelectedScene);

        ImGui::SameLine(
            ImGui::GetContentRegionAvail().x
            - (toolbarButtonSize.x * 2.0f)
            - (ImGui::GetStyle().ItemSpacing.x)
            - (ImGui::GetStyle().FramePadding.x * 4.0f)
        );

        if (ImGui::ImageButton(
            "SceneNodesToolBarDeleteButton",
            pEditorResources->CreateTextureReference("delete.png", Render::DefaultSampler::LinearClamp),
            toolbarButtonSize
        ))
        {
            const auto selectedSceneNode = pMainWindowVM->GetSelectedSceneNode();
            if (selectedSceneNode)
            {
                pMainWindowVM->OnDeleteSceneNode((*selectedSceneNode)->name);
            }
        }

        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x + (ImGui::GetStyle().FramePadding.x * 2.0f));

        if (ImGui::ImageButton(
            "SceneNodesToolBarAddButton",
            pEditorResources->CreateTextureReference("add.png", Render::DefaultSampler::LinearClamp),
            toolbarButtonSize
        ))
        {
            ImGui::OpenPopup("NodeTypePopUp");
        }

        if (ImGui::BeginPopupContextItem("NodeTypePopUp"))
        {
            if (ImGui::Selectable("Entity"))
            {
                pMainWindowVM->OnCreateNewEntityNode();
            }
            else if (ImGui::Selectable("Player"))
            {
                pMainWindowVM->OnCreateNewPlayerNode();
            }

            ImGui::EndPopup();
        }

    ImGui::EndDisabled();
}

std::string GetNodeListName(const Engine::SceneNode* pNode)
{
    std::string descriptor;

    switch (pNode->GetType())
    {
        case Engine::SceneNode::Type::Entity: descriptor = "Entity"; break;
        case Engine::SceneNode::Type::Player: descriptor = "Player"; break;
    }

    return std::format("{} ({})", pNode->name, descriptor);
}

void SceneNodesList(MainWindowVM* pMainWindowVM)
{
    const auto selectedScene = pMainWindowVM->GetSelectedScene();
    if (!selectedScene) { return; }

    const auto selectedSceneNode = pMainWindowVM->GetSelectedSceneNode();

    ImGui::BeginChild("SceneNodesList", ImVec2(0,0), ImGuiChildFlags_FrameStyle);
        for (const auto& node : (*selectedScene)->nodes)
        {
            const bool isSelected = selectedSceneNode && ((*selectedSceneNode)->name == node->name);
            const auto listName = GetNodeListName(node.get());

            if (ImGui::Selectable(listName.c_str(), isSelected))
            {
                pMainWindowVM->OnSceneNodeSelected(node->name);
            }
        }
    ImGui::EndChild();
}

void SceneNodes(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM)
{
    ImGui::BeginChild("SceneNodes", {0.0, 0.0f}, ImGuiChildFlags_Borders);
        SceneNodesToolbar(pEditorResources, pMainWindowVM);
        SceneNodesList(pMainWindowVM);
    ImGui::EndChild();
}

void SceneWindow(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM)
{
    ImGui::Begin(SCENE_WINDOW);

    // If no package is open, don't display anything
    if (!pMainWindowVM->GetPackage().has_value())
    {
        ImGui::End();
        return;
    }

    SceneToolbar(pEditorResources, pMainWindowVM);
    SceneNodes(pEditorResources, pMainWindowVM);

    ImGui::End();
}

}
