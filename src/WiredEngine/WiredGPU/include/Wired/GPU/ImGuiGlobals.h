/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_IMGUIGLOBALS_H
#define WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_IMGUIGLOBALS_H

#ifdef WIRED_IMGUI
    #include <imgui.h>
    #include <implot.h>
#endif

namespace Wired::GPU
{
    struct ImGuiGlobals
    {
        #ifdef WIRED_IMGUI
            ImGuiContext* pImGuiContext;
            ImGuiMemAllocFunc pImGuiMemAllocFunc;
            ImGuiMemFreeFunc pImGuiMemFreeFunc;

            ImPlotContext* pImPlotContext;
        #endif
    };
}

#endif //WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_IMGUIGLOBALS_H
