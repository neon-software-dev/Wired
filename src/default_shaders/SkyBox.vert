/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 450

//
// Definitions
//
struct SkyBoxGlobalUniformPayload
{
    // General
    mat4 surfaceTransform;          // Projection Space -> Rotated projection space
};

struct ViewProjectionUniformPayload
{
    mat4 viewTransform;
    mat4 projectionTransform;
};

//
// INPUTS
//

// Vertex Data
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;

layout(std140, set = 0, binding = 0) uniform SkyBoxGlobalUniformPayloadBuffer
{
    SkyBoxGlobalUniformPayload data;
} u_globalData;

layout(std140, set = 0, binding = 1) uniform ViewProjectionUniformPayloadBuffer
{
    ViewProjectionUniformPayload data;
} u_viewProjectionData;

//
// OUTPUTS
//
layout(location = 0) out vec3 o_fragTexCoord;           // The vertex's tex coord

void main()
{
    const vec4 vertexPosition =
        u_globalData.data.surfaceTransform *
        u_viewProjectionData.data.projectionTransform *
        u_viewProjectionData.data.viewTransform *
    vec4(i_vertexPosition_modelSpace, 1.0);

    gl_Position = vertexPosition;

    // For reversed depth buffer, force z position to 0 so that after perspective divide
    // by w it stays at the farthest depth value, 0. (Note that we have depth write
    // disabled for sky box, so it's only used for depth testing).
    gl_Position.z = 0.0f;

    // Outputs being passed to the fragment shader.
    // Note that the vertex position is used rather than the non-existent vertex uv
    o_fragTexCoord = i_vertexPosition_modelSpace;
    // Adjustment otherwise the sides are all flipped horizontally
    o_fragTexCoord.x *= -1.0;
}
