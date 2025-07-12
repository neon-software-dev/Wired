/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_ASSETVIEWWINDOW_H
#define WIREDEDITOR_WINDOW_ASSETVIEWWINDOW_H

namespace Wired::Engine
{
    class IEngineAccess;
}

namespace Wired
{
    class MainWindowVM;
    class AssetsWindowVM;
    class EditorResources;

    static constexpr auto ASSET_VIEW_WINDOW = "Asset View###AssetViewWindow";

    void AssetViewWindow(Engine::IEngineAccess* engine, EditorResources* pEditorResources, MainWindowVM* mainWindowVM, AssetsWindowVM* vm);
}

#endif //WIREDEDITOR_WINDOW_ASSETVIEWWINDOW_H
