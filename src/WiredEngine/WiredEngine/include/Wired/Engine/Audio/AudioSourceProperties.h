/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_AUDIO_AUDIOSOURCEPROPERTIES_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_AUDIO_AUDIOSOURCEPROPERTIES_H

namespace Wired::Engine
{
    /**
     * Defines properties that can be applied when playing an audio source
     */
    struct AudioSourceProperties
    {
        // Whether the audio loops (repeats)
        // Note: only applicable to static audio data
        bool looping{false};

        // The distance in world space that no attenuation occurs. At 0.0, no distance
        // attenuation ever occurs on non-linear attenuation models.
        float referenceDistance{1.0f};

        // A value of 1.0 means unattenuated. Each division by 2 equals an attenuation of
        // about -6dB. Each multiplicaton by 2 equals an amplification of about +6dB. A value
        // of 0.0 is meaningless with respect to a logarithmic scale; it is silent.
        float gain{1.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_AUDIO_AUDIOSOURCEPROPERTIES_H
