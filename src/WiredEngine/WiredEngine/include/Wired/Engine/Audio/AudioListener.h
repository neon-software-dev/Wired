/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_AUDIO_AUDIOLISTENER_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_AUDIO_AUDIOLISTENER_H

#include <Wired/Engine/World/WorldCommon.h>

#include <glm/glm.hpp>

namespace Wired::Engine
{
    struct CameraAudioListener
    {
        float gain{1.0f};

        std::string worldName;
        CameraId cameraId;
    };

    /**
     * Defines the properties relevant to an audio listener: the listener's position and orientation
     */
    struct AudioListener
    {
        // Output master gain to apply to audio (1.0f = unattenuated)
        float gain{1.0f};

        // The listener's position, in world space
        glm::vec3 worldPosition{0.0f, 0.0f, 0.0f};

        // The listener's orientation unit vector
        glm::vec3 lookUnit{0.0f, 0.0f, -1.0f};

        // Up-vector for the listener's orientation
        glm::vec3 upUnit{0.0f, 1.0f, 0.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_AUDIO_AUDIOLISTENER_H
