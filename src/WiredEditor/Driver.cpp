/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EditorClient.h"

#include <Wired/Engine/DesktopEngine.h>

int main(int, char*[])
{
    using namespace Wired;

    Engine::DesktopEngine desktopEngine{};
    if (!desktopEngine.Initialize("WiredEditor", {0,0,1}, Engine::RunMode::Window))
    {
        return 1;
    }

    auto client = std::make_unique<EditorClient>();

    desktopEngine.ExecWindowed("Wired Editor", {3000, 2000}, std::move(client));

    return 0;
}
