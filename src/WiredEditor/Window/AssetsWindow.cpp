/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AssetsWindow.h"

#include "../ViewModel/MainWindowVM.h"
#include "../ViewModel/AssetsWindowVM.h"

#include <imgui.h>

namespace Wired
{

void ShadersView(const Engine::Package& package, AssetsWindowVM* vm)
{
    const auto selectedAsset = vm->GetSelectedAsset();
    const bool isAssetTypeSelected = selectedAsset && selectedAsset->assetType == Engine::AssetType::Shader;

    ImGui::BeginChild("ShaderAssetList", ImVec2(0,0), ImGuiChildFlags_FrameStyle);
        for (const auto& assetName : package.assetNames.shaderAssetNames)
        {
            const bool assetIsSelected = isAssetTypeSelected && (selectedAsset->assetName == assetName);

            if (ImGui::Selectable(assetName.c_str(), assetIsSelected))
            {
                vm->SetSelectedAsset(SelectedAsset{.assetType = Engine::AssetType::Shader, .assetName = assetName});
            }
        }
    ImGui::EndChild();
}

void ImagesView(const Engine::Package& package, AssetsWindowVM* vm)
{
    const auto selectedAsset = vm->GetSelectedAsset();
    const bool isAssetTypeSelected = selectedAsset && selectedAsset->assetType == Engine::AssetType::Image;

    ImGui::BeginChild("ImageAssetList", ImVec2(0,0), ImGuiChildFlags_FrameStyle);
    for (const auto& assetName : package.assetNames.imageAssetNames)
    {
        const bool assetIsSelected = isAssetTypeSelected && (selectedAsset->assetName == assetName);

        if (ImGui::Selectable(assetName.c_str(), assetIsSelected))
        {
            vm->SetSelectedAsset(SelectedAsset{.assetType = Engine::AssetType::Image, .assetName = assetName});
        }
    }
    ImGui::EndChild();
}

void ModelsView(const Engine::Package& package, AssetsWindowVM* vm)
{
    const auto selectedAsset = vm->GetSelectedAsset();
    const bool isAssetTypeSelected = selectedAsset && selectedAsset->assetType == Engine::AssetType::Model;

    ImGui::BeginChild("ModelAssetList", ImVec2(0,0), ImGuiChildFlags_FrameStyle);
    for (const auto& assetName : package.assetNames.modelAssetNames)
    {
        const bool assetIsSelected = isAssetTypeSelected && (selectedAsset->assetName == assetName);

        if (ImGui::Selectable(assetName.c_str(), assetIsSelected))
        {
            vm->SetSelectedAsset(SelectedAsset{.assetType = Engine::AssetType::Model, .assetName = assetName});
        }
    }
    ImGui::EndChild();
}

void AudioView(const Engine::Package& package, AssetsWindowVM* vm)
{
    const auto selectedAsset = vm->GetSelectedAsset();
    const bool isAssetTypeSelected = selectedAsset && selectedAsset->assetType == Engine::AssetType::Audio;

    ImGui::BeginChild("AudioAssetList", ImVec2(0,0), ImGuiChildFlags_FrameStyle);
    for (const auto& assetName : package.assetNames.audioAssetNames)
    {
        const bool assetIsSelected = isAssetTypeSelected && (selectedAsset->assetName == assetName);

        if (ImGui::Selectable(assetName.c_str(), assetIsSelected))
        {
            vm->SetSelectedAsset(SelectedAsset{.assetType = Engine::AssetType::Audio, .assetName = assetName});
        }
    }
    ImGui::EndChild();
}

void FontsView(const Engine::Package& package, AssetsWindowVM* vm)
{
    const auto selectedAsset = vm->GetSelectedAsset();
    const bool isAssetTypeSelected = selectedAsset && selectedAsset->assetType == Engine::AssetType::Font;

    ImGui::BeginChild("FontsAssetList", ImVec2(0,0), ImGuiChildFlags_FrameStyle);
    for (const auto& assetName : package.assetNames.fontAssetNames)
    {
        const bool assetIsSelected = isAssetTypeSelected && (selectedAsset->assetName == assetName);

        if (ImGui::Selectable(assetName.c_str(), assetIsSelected))
        {
            vm->SetSelectedAsset(SelectedAsset{.assetType = Engine::AssetType::Font, .assetName = assetName});
        }
    }
    ImGui::EndChild();
}

void AssetTypeTabs(const Engine::Package& package, AssetsWindowVM* vm)
{
    if (ImGui::BeginTabBar("AssetTypes"))
    {
        if (ImGui::BeginTabItem("Shaders"))
        {
            ShadersView(package, vm);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Images"))
        {
            ImagesView(package, vm);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Models"))
        {
            ModelsView(package, vm);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Audio"))
        {
            AudioView(package, vm);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Fonts"))
        {
            FontsView(package, vm);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void AssetsWindow(MainWindowVM* mainWindowVM, AssetsWindowVM* vm)
{
    ImGui::Begin(ASSETS_WINDOW);

    const auto& package = mainWindowVM->GetPackage();
    if (!package)
    {
        ImGui::End();
        return;
    }

    AssetTypeTabs(*package, vm);

    ImGui::End();
}

}
