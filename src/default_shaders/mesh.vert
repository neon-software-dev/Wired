/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// Internal
//
struct ObjectGlobalUniformPayload
{
    // General
    mat4 surfaceTransform;
    uint lightId;

    // Lighting
    vec3 ambientLight;
    uint highestLightId;
    bool hdrEnabled;
    float shadowCascadeOverlap;                 // Ratio of overlap between cascade cuts
};

struct ViewProjectionUniformPayload
{
    mat4 viewTransform;
    mat4 projectionTransform;
};

struct ObjectInstanceDataPayload
{
    bool isValid;
    uint objectId;
    uint meshId;
    uint materialId;
    mat4 modelTransform;
};

struct DrawDataPayload
{
    uint objectId;
};

mat3 GenerateTBNNormalTransform(vec3 vertexNormal_modelSpace, vec3 vertexTangent_modelSpace, mat4 modelTransform);

//
// INPUTS
//
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;

layout(std140, set = 1, binding = 0) uniform ObjectGlobalUniformPayloadBuffer
{
    ObjectGlobalUniformPayload data;
} u_globalData;

layout(std140, set = 1, binding = 1) uniform ViewProjectionUniformPayloadBuffer
{
    ViewProjectionUniformPayload data;
} u_viewProjectionData;

layout(std430, set = 1, binding = 2) readonly buffer ObjectInstanceDataPayloadBuffer
{
    ObjectInstanceDataPayload data[];
} i_objectInstanceData;

layout(std430, set = 2, binding = 0) readonly buffer DrawDataPayloadBuffer
{
    DrawDataPayload data[];
} i_drawData;

//
// OUTPUTS
//
layout(location = 0) out uint o_instanceIndex;
layout(location = 1) out vec2 o_fragTexCoord;
layout(location = 2) out vec3 o_fragPos_worldSpace;
layout(location = 3) out vec3 o_fragNormal_modelSpace;
layout(location = 4) out mat3 o_tbnNormalTransform;

void main()
{
    const DrawDataPayload drawDataPayload = i_drawData.data[gl_InstanceIndex];
    const ObjectInstanceDataPayload instanceDataPayload = i_objectInstanceData.data[drawDataPayload.objectId];

    const vec3 fragPos_worldSpace =
        (instanceDataPayload.modelTransform * vec4(i_vertexPosition_modelSpace, 1.0f)).xyz;

    gl_Position =
        u_globalData.data.surfaceTransform *
        u_viewProjectionData.data.projectionTransform *
        u_viewProjectionData.data.viewTransform *
        vec4(fragPos_worldSpace, 1.0f);

    o_instanceIndex = gl_InstanceIndex;
    o_fragTexCoord = i_vertexUv;
    o_fragPos_worldSpace = fragPos_worldSpace;
    o_fragNormal_modelSpace = i_vertexNormal_modelSpace;
    o_tbnNormalTransform = GenerateTBNNormalTransform(i_vertexNormal_modelSpace, i_vertexTangent_modelSpace, instanceDataPayload.modelTransform);
}

mat3 GenerateTBNNormalTransform(vec3 vertexNormal_modelSpace, vec3 vertexTangent_modelSpace, mat4 modelTransform)
{
    const vec3 T = normalize(vec3(modelTransform * vec4(vertexTangent_modelSpace, 0.0)));
    const vec3 N = normalize(vec3(modelTransform * vec4(vertexNormal_modelSpace, 0.0)));
    const vec3 B = cross(N, T);
    return mat3(T, B, N);
}
