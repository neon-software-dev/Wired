/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_WINDOW_ASSETSWINDOW_H
#define WIREDEDITOR_WINDOW_ASSETSWINDOW_H

namespace Wired
{
    class MainWindowVM;
    class AssetsWindowVM;

    static constexpr auto ASSETS_WINDOW = "Asset Management###AssetsWindow";

    void AssetsWindow(MainWindowVM* mainWindowVM, AssetsWindowVM* vm);
}

#endif //WIREDEDITOR_WINDOW_ASSETSWINDOW_H
