/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 450

//
// INPUTS
//
layout(location = 0) in vec2 i_fragTexCoord;            // The fragment's tex coord

// Set 0 - Material Data
layout(set = 3, binding = 0) uniform sampler2D i_spriteSampler;

//
// Outputs
//
layout(location = 0) out vec4 o_fragColor;

void main() 
{
    vec4 surfaceColor = texture(i_spriteSampler, i_fragTexCoord);

    // To help with transparency, discard fragments that are sufficiently close to fully transparent,
    // so that they don't override what might have already been behind them in the depth buffer with
    // a transparent pixel
    if (surfaceColor.a < 0.001f) {  discard;  }

    o_fragColor = surfaceColor;
}

