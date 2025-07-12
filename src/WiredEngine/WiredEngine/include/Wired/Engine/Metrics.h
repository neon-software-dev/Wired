/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_METRICS_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_METRICS_H

namespace Wired::Engine
{
    static constexpr auto METRIC_SIM_STEP_TIME = "engine_simulation_step_time";

    static constexpr auto METRIC_RENDER_FRAME_TIME = "engine_render_frame_time";
    static constexpr auto METRIC_RENDER_STATE_UPDATE_COUNT = "engine_render_state_updates";

    static constexpr auto METRIC_PHYSICS_SIM_TIME = "engine_physics_sim_time";
    static constexpr auto METRIC_PHYSICS_NUM_ACTIVE_BODIES = "engine_physics_num_active_bodies";

    static constexpr auto METRIC_AUDIO_NUM_BUFFERS = "engine_audio_num_buffers";
    static constexpr auto METRIC_AUDIO_NUM_SOURCES = "engine_audio_num_sources";
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_METRICS_H
