/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINEIMGUI_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINEIMGUI_H

#include "IEngineAccess.h"

#ifdef WIRED_IMGUI
    #include <imgui.h>
#endif

namespace Wired::Engine
{
    inline void EnsureImGui(IEngineAccess* engine)
    {
        (void)engine;

        #ifdef WIRED_IMGUI
            if (engine->IsImGuiAvailable())
            {
                const auto imGuiGlobals = engine->GetImGuiGlobals();

                ImGui::SetCurrentContext(imGuiGlobals.pImGuiContext);
                ImGui::SetAllocatorFunctions(imGuiGlobals.pImGuiMemAllocFunc, imGuiGlobals.pImGuiMemFreeFunc, nullptr);
            }
        #endif
    }
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_ENGINEIMGUI_H
