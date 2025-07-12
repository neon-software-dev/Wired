/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 450

//
// Internal
//
const float PI = 3.14159265359;

const uint MAX_SHADOW_MAP_LIGHT_COUNT = 16;
const uint SHADOW_CASCADE_COUNT = 4;            // Cascade count for cascaded shadow maps
const uint MAX_SHADOW_RENDER_COUNT = 6;         // Maximum shadow renders per light

const uint LIGHT_TYPE_POINT = 0;
const uint LIGHT_TYPE_SPOTLIGHT = 1;
const uint LIGHT_TYPE_DIRECTIONAL = 2;

const uint ATTENUATION_MODE_NONE = 0;           // Attenuation - none
const uint ATTENUATION_MODE_LINEAR = 1;         // Attenuation - linear decrease
const uint ATTENUATION_MODE_EXPONENTIAL = 2;    // Attenuation - exponential decrease

const uint ALPHA_MODE_OPAQUE = 0;               // Opaque final alpha
const uint ALPHA_MODE_MASK = 1;                 // Fully transparent or opaque final alpha depending on mask
const uint ALPHA_MODE_BLEND = 2;                // Translucent-capable

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

struct PBRMaterialPayload
{
    uint alphaMode;
    float alphaCutoff;

    vec4 albedoColor;
    bool hasAlbedoSampler;

    float metallicFactor;
    bool hasMetallicSampler;

    float roughnessFactor;
    bool hasRoughnessSampler;

    bool hasNormalSampler;

    bool hasAOSampler;

    vec3 emissiveColor;
    bool hasEmissiveSampler;
};

struct ShadowSamplerUniformPayload
{
    uint lightId;
    uint samplerIndex;
};

struct ShadowMapPayload
{
    vec3 worldPos;                  // World position the shadow map was rendered from
    mat4 viewProjection;            // View+Projection transform for the shadow map render

    // Directional shadow map specific
    vec2 cut;                       // Cascade [start, end] distances, in camera view space
    uint cascadeIndex;              // Cascade index [0..Shadow_Cascade_Count)
};

struct LightPayload
{
    bool isValid;
    uint id;
    bool castsShadows;
    vec3 worldPos;

    // Base light properties
    uint lightType;
    uint attenuationMode;           // (ATTENUATION_MODE_{X})
    float maxAffectRange;
    vec3 color;
    vec3 directionUnit;
    float areaOfEffect;
};

struct FragLightingParameters
{
    vec4 albedo;
    float metallic;
    float roughness;
    float ao;
};

FragLightingParameters GetFragLightingParameters(PBRMaterialPayload materialPayload);

//
// INPUTS
//
layout(location = 0) flat in uint i_instanceIndex;
layout(location = 1) in vec2 i_fragTexCoord;
layout(location = 2) in vec3 i_fragPos_worldSpace;
layout(location = 3) in vec3 i_fragNormal_modelSpace;
layout(location = 4) in mat3 i_tbnNormalTransform;

layout(std430, set = 0, binding = 1) readonly buffer PBRMaterialPayloadBuffer
{
    PBRMaterialPayload data[];
} i_materialPayloads;

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

layout(std430, set = 1, binding = 3) readonly buffer LightPayloadBuffer
{
    LightPayload data[];
} i_lightData;

layout(std430, set = 1, binding = 4) readonly buffer ShadowMapPayloadBuffer
{
    ShadowMapPayload data[];
} i_shadowMapData;

layout(std430, set = 2, binding = 0) readonly buffer DrawDataPayloadBuffer
{
    DrawDataPayload data[];
} i_drawData;

layout(set = 3, binding = 0) uniform sampler2D i_albedoSampler;
layout(set = 3, binding = 1) uniform sampler2D i_metallicSampler;
layout(set = 3, binding = 2) uniform sampler2D i_roughnessSampler;
layout(set = 3, binding = 3) uniform sampler2D i_normalSampler;
layout(set = 3, binding = 4) uniform sampler2D i_aoSampler;
layout(set = 3, binding = 5) uniform sampler2D i_emissionSampler;

void main()
{
    const DrawDataPayload drawDataPayload = i_drawData.data[i_instanceIndex];
    const ObjectInstanceDataPayload instanceDataPayload = i_objectInstanceData.data[drawDataPayload.objectId];
    const PBRMaterialPayload materialPayload = i_materialPayloads.data[instanceDataPayload.materialId];
    const LightPayload lightPayload = i_lightData.data[u_globalData.data.lightId];

    const vec3 fragPos_worldSpace = i_fragPos_worldSpace;
    const vec3 fragPos_viewSpace = (u_viewProjectionData.data.viewTransform * vec4(i_fragPos_worldSpace, 1.0f)).xyz;

    const FragLightingParameters lightingParams = GetFragLightingParameters(materialPayload);

    // Discard fully transparent fragments
    if (lightingParams.albedo.a <= 0.01f)
    {
        discard;
    }

    if (lightPayload.lightType == LIGHT_TYPE_POINT ||
        lightPayload.lightType == LIGHT_TYPE_SPOTLIGHT)
    {
        // Reversed z, linear, range from [0..1] from far to near
        gl_FragDepth = (lightPayload.maxAffectRange - length(fragPos_viewSpace)) / lightPayload.maxAffectRange;
    }
    else
    {
        const vec4 fragPosition_shadowClipSpace =
            u_globalData.data.surfaceTransform *
            u_viewProjectionData.data.projectionTransform *
            vec4(fragPos_viewSpace, 1.0f);

        const vec3 fragPosition_shadowNDCSpace = fragPosition_shadowClipSpace.xyz / fragPosition_shadowClipSpace.w;

        // Reversed z, non-linear, [0..1] from far to near, within the scope of the shadow render orthograph projection depth
        gl_FragDepth = fragPosition_shadowNDCSpace.z;
    }
}

FragLightingParameters GetFragLightingParameters(PBRMaterialPayload materialPayload)
{
    FragLightingParameters params;

    //
    // Read parameter values from the fragment's material
    //
    params.albedo = materialPayload.hasAlbedoSampler ?
        texture(i_albedoSampler, i_fragTexCoord) :
        materialPayload.albedoColor;

    params.metallic = materialPayload.hasMetallicSampler ?
        texture(i_metallicSampler, i_fragTexCoord).r : // Requires texture to store metallic in r channel
        materialPayload.metallicFactor;

    params.roughness = materialPayload.hasRoughnessSampler ?
        texture(i_roughnessSampler, i_fragTexCoord).g : // Requires texture to store roughness in g channel
        materialPayload.roughnessFactor;

    params.ao = materialPayload.hasAOSampler ?
        texture(i_aoSampler, i_fragTexCoord).b : // Requires texture to store AO in b channel
        1.0f;

    //
    // Apply alpha mode
    //
    if (materialPayload.alphaMode == ALPHA_MODE_OPAQUE)
    {
        // "The rendered output is fully opaque and any alpha value is ignored."
        params.albedo.a = 1.0f;
    }
    else if (materialPayload.alphaMode == ALPHA_MODE_MASK)
    {
        // "The rendered output is either fully opaque or fully transparent depending on the alpha value and
        // the specified alpha cutoff value."
        params.albedo.a = params.albedo.a >= materialPayload.alphaCutoff ? 1.0f : 0.0f;
    }
    else if (materialPayload.alphaMode == ALPHA_MODE_BLEND)
    {
        // no-op - use the alphas as specified by the material
    }

    return params;
}
