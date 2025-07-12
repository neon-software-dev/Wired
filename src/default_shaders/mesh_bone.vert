/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// Internal
//
const uint MESH_MAX_LOD = 3;

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

struct MeshLODPayload
{
    bool isValid;

    float renderDistance;

    uint vertexOffset;
    uint numIndices;
    uint firstIndex;
};

struct MeshPayload
{
    bool hasCullAABB;
    vec3 cullAABBMin;
    vec3 cullAABBMax;
    uint numBones;

    MeshLODPayload lodData[MESH_MAX_LOD];
};

struct BoneVertex
{
    vec4 pos_modelSpace;
    vec3 normal_modelSpace;
    vec3 tangent_modelSpace;
};

BoneVertex TransformVertexByBones(uint boneStartIndex, MeshPayload meshPayload);
mat3 GenerateTBNNormalTransform(vec3 vertexNormal_modelSpace, vec3 vertexTangent_modelSpace, mat4 modelTransform);

//
// INPUTS
//
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;
layout(location = 4) in ivec4 i_bones;
layout(location = 5) in vec4 i_boneWeights;

layout(std430, set = 0, binding = 0) readonly buffer MeshPayloadBuffer
{
    MeshPayload data[];
} i_meshPayloads;

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

layout(std430, set = 1, binding = 6) readonly buffer BoneTransformsPayloadBuffer
{
    mat4 data[];
} i_boneTransformsData;

layout(std430, set = 1, binding = 7) readonly buffer BoneMappingPayloadBuffer
{
    uint data[];
} i_boneMappingData;

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
    const MeshPayload meshPayload = i_meshPayloads.data[instanceDataPayload.meshId];
    const uint boneStartIndex = i_boneMappingData.data[drawDataPayload.objectId];

    // Transform the vertex's values by its per-instance bone transforms
    const BoneVertex boneVertex = TransformVertexByBones(boneStartIndex, meshPayload);

    const vec4 vertexPos_worldSpace = instanceDataPayload.modelTransform * boneVertex.pos_modelSpace;
    
    gl_Position =
        u_globalData.data.surfaceTransform *
        u_viewProjectionData.data.projectionTransform *
        u_viewProjectionData.data.viewTransform *
        vertexPos_worldSpace;

    o_instanceIndex = gl_InstanceIndex;
    o_fragTexCoord = i_vertexUv;
    o_fragPos_worldSpace = vec3(vertexPos_worldSpace);
    o_fragNormal_modelSpace = boneVertex.normal_modelSpace;
    o_tbnNormalTransform = GenerateTBNNormalTransform(boneVertex.normal_modelSpace, boneVertex.tangent_modelSpace, instanceDataPayload.modelTransform);
}

BoneVertex TransformVertexByBones(uint boneStartIndex, MeshPayload meshPayload)
{
    BoneVertex boneVertex;
    boneVertex.pos_modelSpace = vec4(0);
    boneVertex.normal_modelSpace = vec3(0);
    boneVertex.tangent_modelSpace = vec3(0);

    // Get the index into the bone transforms vector which this instance's bone transforms are stored
    const uint numMeshBones = meshPayload.numBones;
    const uint boneTransformsBaseOffset = boneStartIndex;

    // Each vertex can have up to 4 bones which modify it. Loop through all four and apply
    // their transformations together.
    for (int x = 0; x < 4; ++x)
    {
        // If a bone doesn't affect this vertex, do nothing
        if (i_bones[x] == -1) { continue; }

        const mat4 boneTransform = i_boneTransformsData.data[boneTransformsBaseOffset + i_bones[x]];

        // Modify the vertex's postion and normal by the amount the bone transform specifies
        boneVertex.pos_modelSpace += (boneTransform * vec4(i_vertexPosition_modelSpace, 1)) * i_boneWeights[x];
        boneVertex.normal_modelSpace += (mat3(boneTransform) * i_vertexNormal_modelSpace) * i_boneWeights[x];
        boneVertex.tangent_modelSpace += (mat3(boneTransform) * i_vertexTangent_modelSpace) * i_boneWeights[x];
    }

    boneVertex.normal_modelSpace = normalize(boneVertex.normal_modelSpace);
    boneVertex.tangent_modelSpace = normalize(boneVertex.tangent_modelSpace);

    return boneVertex;
}

mat3 GenerateTBNNormalTransform(vec3 vertexNormal_modelSpace, vec3 vertexTangent_modelSpace, mat4 modelTransform)
{
    const vec3 T = normalize(vec3(modelTransform * vec4(vertexTangent_modelSpace, 0.0)));
    const vec3 N = normalize(vec3(modelTransform * vec4(vertexNormal_modelSpace, 0.0)));
    const vec3 B = cross(N, T);
    return mat3(T, B, N);
}
