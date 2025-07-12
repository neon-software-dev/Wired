/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_METRICS_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_METRICS_H

namespace Wired::Render
{
    // CPU metrics
    static constexpr auto METRIC_RENDERER_CPU_ALL_FRAME_WORK = "renderer_cpu_all_frame_work";

    // GPU metrics
    static constexpr auto METRIC_RENDERER_GPU_ALL_FRAME_WORK = "renderer_gpu_all_frame_work";
    static constexpr auto METRIC_RENDERER_GPU_ALL_SHADOW_MAP_RENDER_WORK = "renderer_gpu_all_shadow_map_render_work";
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_METRICS_H
