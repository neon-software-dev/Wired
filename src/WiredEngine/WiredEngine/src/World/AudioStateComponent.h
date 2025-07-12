/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_AUDIOSTATECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_AUDIOSTATECOMPONENT_H

#include <Wired/Engine/Audio/AudioCommon.h>

#include <unordered_set>

namespace Wired::Engine
{
    struct AudioStateComponent
    {
        std::unordered_set<AudioSourceId> activeSources;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_AUDIOSTATECOMPONENT_H
