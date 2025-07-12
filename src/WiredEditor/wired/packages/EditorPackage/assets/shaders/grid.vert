/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// Internal
//

//
// INPUTS
//
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;

//
// OUTPUTS
//
layout(location = 0) out vec3 o_fragPosition;

void main() 
{
    // Convert the standard sprite mesh to span the full render area (e.g. [0.5, 0.5] -> [1, 1])
    vec3 vertexPosition = vec3(i_vertexPosition_modelSpace.xy * 2.0f, 1.0f);

    o_fragPosition = vertexPosition;
    gl_Position = vec4(vertexPosition, 1.0f);
}
