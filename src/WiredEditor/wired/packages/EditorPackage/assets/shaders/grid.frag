/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 450

//
// Internal
//
struct ViewProjectionPayload
{
    mat4 viewTransform;
    mat4 projectionTransform;
};

struct GridDataPayload
{
    vec2 gridInterval;
    float gridLineWidth;
    vec3 gridLineColor;
    float cameraScale;
};

//
// INPUTS
//
layout(location = 0) in vec3 i_fragPosition;

layout(set = 3, binding = 0) uniform ViewProjectionPayloadUniform
{
    ViewProjectionPayload data;
} u_viewProjectionData;

layout(set = 3, binding = 1) uniform GridDataPayloadUniform
{
    GridDataPayload data;
} u_gridData;

//
// Outputs
//
layout(location = 0) out vec4 o_fragColor;

vec2 getScreenPosition(vec2 ndc)
{
    vec4 clip = vec4(ndc, 0.0, 1.0);
    mat4 invVP = inverse(u_viewProjectionData.data.projectionTransform * u_viewProjectionData.data.viewTransform);
    vec4 world = invVP * clip;
    return world.xy;
}

void main()
{
    const vec2 interval = u_gridData.data.gridInterval;
    const float lineWidth = u_gridData.data.gridLineWidth;
    const float cameraScale = u_gridData.data.cameraScale;

    const vec2 screenPos = getScreenPosition(i_fragPosition.xy);

    // Get distance to the closest grid line along x and y
    vec2 gridMod = abs(mod(screenPos, interval));
    gridMod = min(gridMod, interval - gridMod); // mirror mod to handle negative positions

    // Determine if within line width (line centered on interval), with a minimum line width
    // which is dependent on scale (zooming out has larger min width)
    const float effectiveLineWidth = max(lineWidth, 1.0 / cameraScale);
    const float halfLineWidth = effectiveLineWidth * 0.5f;

    const float lineAlpha = 1.0f - smoothstep(0.0f, halfLineWidth, min(gridMod.x, gridMod.y));

    const vec4 finalColor = vec4(u_gridData.data.gridLineColor, lineAlpha);

    if (finalColor.a < 0.01f)
    {
        discard;
    }

    o_fragColor = finalColor;
}
