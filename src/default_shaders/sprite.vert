/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// Internal
//
struct ViewProjectionUniformPayload
{
    mat4 viewTransform;
    mat4 projectionTransform;
};

struct SpriteInstanceDataPayload
{
    uint isValid;
    uint id;
    uint meshId;
    mat4 modelTransform;
    vec2 uvTranslation;
    vec2 uvSize;
};

struct DrawDataPayload
{
    uint spriteId;
};

vec2 GetUVCoords(const SpriteInstanceDataPayload spritePayload, const uint vertexIndex);

//
// INPUTS
//
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;

layout(std140, set = 1, binding = 0) uniform ViewProjectionUniformPayloadBuffer
{
    ViewProjectionUniformPayload data;
} u_viewProjectionData;

layout(std430, set = 1, binding = 1) readonly buffer SpriteInstanceDataPayloadBuffer
{
    SpriteInstanceDataPayload data[];
} i_spriteInstanceData;

layout(std430, set = 2, binding = 0) readonly buffer DrawDataPayloadBuffer
{
    DrawDataPayload data[];
} i_drawData;

//
// OUTPUTS
//
layout(location = 0) out vec2 o_fragTexCoord;           // The vertex's tex coord

void main() 
{
    const DrawDataPayload drawDataPayload = i_drawData.data[gl_InstanceIndex];
    const SpriteInstanceDataPayload instanceDataPayload = i_spriteInstanceData.data[drawDataPayload.spriteId];

    o_fragTexCoord = GetUVCoords(instanceDataPayload, gl_VertexIndex);
    
    gl_Position =
        //u_globalData.data.surfaceTransform *
        u_viewProjectionData.data.projectionTransform *
        u_viewProjectionData.data.viewTransform *
        instanceDataPayload.modelTransform *
        vec4(i_vertexPosition_modelSpace, 1.0f);
}

vec2 GetUVCoords(const SpriteInstanceDataPayload instanceDataPayload, const uint vertexIndex)
{
    vec2 uvCoord = instanceDataPayload.uvTranslation;

    if (vertexIndex == 0)
    {
        uvCoord.y += instanceDataPayload.uvSize.y;
    }
    if (vertexIndex == 1)
    {
        uvCoord.x += instanceDataPayload.uvSize.x;
        uvCoord.y += instanceDataPayload.uvSize.y;
    }
    else if (vertexIndex == 2)
    {
        uvCoord.x += instanceDataPayload.uvSize.x;
    }
    else if (vertexIndex == 3)
    {
        uvCoord = uvCoord;
    }

    return uvCoord;
}
