/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 450

//
// INPUTS
//
layout(location = 0) in vec3 i_fragTexCoord;            // The fragment's tex coord

// Set 2 - Texture bindings
layout(set = 1, binding = 0) uniform samplerCube i_skyboxSampler;

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor;

void main()
{
    o_fragColor = texture(i_skyboxSampler, i_fragTexCoord);
}
