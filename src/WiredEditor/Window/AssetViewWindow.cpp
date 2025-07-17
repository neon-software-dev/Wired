/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AssetViewWindow.h"

#include "../EditorResources.h"

#include "../View/TextureView.h"

#include "../ViewModel/MainWindowVM.h"
#include "../ViewModel/AssetsWindowVM.h"

#include <Wired/Engine/IEngineAccess.h>

namespace Wired
{

void AssetViewImage(Engine::IEngineAccess* engine, MainWindowVM* mainWindowVM, const std::string& assetName)
{
    TextureView(engine, NCommon::BlitType::CenterInside, mainWindowVM->GetPackageResources()->textures.at(assetName));
}

void AssetViewModel(Engine::IEngineAccess* engine, EditorResources* pEditorResources, MainWindowVM* mainWindowVM,  AssetsWindowVM* vm, const std::string& assetName)
{
    // Look up the resources that were loaded for the active package
    const auto& packageResources = mainWindowVM->GetPackageResources();
    if (!packageResources) { return; }

    // Find the loaded model id for the chosen model asset name
    const auto modelId = packageResources->models.find(assetName);
    if (modelId == packageResources->models.cend()) { return; }

    // Retrieve the model data for the model
    const auto model = engine->GetResources()->GetModel(modelId->second);
    if (!model) { return; }

    const ImVec2 contentSize = ImGui::GetContentRegionAvail();
    const float topBarHeight = 40.0f;
    const float contentHeight = contentSize.y - topBarHeight - (1.0f * ImGui::GetStyle().ItemSpacing.y);

    //
    // Top toolbar
    //
    ImGui::BeginChild("TopToolbar", ImVec2(0, topBarHeight));
        std::string previewName = "None";
        const auto selectedAnimationName = vm->GetSelectedModelAnimationName();
        if (selectedAnimationName)
        {
            previewName = *selectedAnimationName;
            if (previewName.empty())
            {
                previewName = "[No Name Animation]";
            }
        }

        if (ImGui::BeginCombo("Animation Preview###AnimationPreviewCombo", previewName.c_str()))
        {
            if (ImGui::Selectable("None"))
            {
                vm->SetSelectedModelAnimationName(std::nullopt);
            }

            for (const auto& animationIt : (*model)->animations)
            {
                auto animationDisplayName = animationIt.first;
                if (animationDisplayName.empty())
                {
                    animationDisplayName = "[No Name Animation]";
                }

                if (ImGui::Selectable(animationDisplayName.c_str()))
                {
                    vm->SetSelectedModelAnimationName(animationIt.first);
                }
            }

            ImGui::EndCombo();
        }
    ImGui::EndChild();

    //
    // Central content
    //
    ImGui::BeginChild("CentralContent", ImVec2(0, contentHeight));
        TextureView(engine, NCommon::BlitType::CenterInside, pEditorResources->GetAssetViewColorTextureId());
    ImGui::EndChild();
}

void AssetViewWindow(Engine::IEngineAccess* engine, EditorResources* pEditorResources, MainWindowVM* mainWindowVM, AssetsWindowVM* vm)
{
    const auto selectedAsset = vm->GetSelectedAsset();

    ImGui::Begin(ASSET_VIEW_WINDOW);
        if (selectedAsset)
        {
            switch (selectedAsset->assetType)
            {
                case Engine::AssetType::Shader:
                    ImGui::Text("Shader: %s", selectedAsset->assetName.c_str());
                break;
                case Engine::AssetType::Image:
                    AssetViewImage(engine, mainWindowVM, selectedAsset->assetName);
                break;
                case Engine::AssetType::Model:
                    AssetViewModel(engine, pEditorResources, mainWindowVM, vm, selectedAsset->assetName);
                break;
                case Engine::AssetType::Audio:
                    ImGui::Text("Audio: %s", selectedAsset->assetName.c_str());
                break;
                case Engine::AssetType::Font:
                    ImGui::Text("Font: %s", selectedAsset->assetName.c_str());
                break;
            }
        }
    ImGui::End();
}

}
