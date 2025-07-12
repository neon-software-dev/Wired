/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 450
#extension GL_EXT_nonuniform_qualifier : require

//
// Internal
//
const float PI = 3.14159265359;
const float EPSILON = 1e-6;

const uint SHADER_MAX_SHADOW_MAP_LIGHT_COUNT = 5;       // Maximum number of lights which can have shadow maps provided
const uint SHADER_MAX_SHADOW_MAP_SPOTLIGHT_COUNT = 2;   // Maximum number of spotlights which can have shadow maps provided
const uint SHADER_MAX_SHADOW_MAP_POINT_COUNT = 2;       // Maximum number of point lights which can have shadow maps provided
const uint SHADER_MAX_SHADOW_MAP_DIRECTIONAL_COUNT = 1; // Maximum number of directional lights which can have shadow maps provided

const uint MAX_PER_LIGHT_SHADOW_RENDER_COUNT = 6;   // Maximum shadow renders per light (6 for point lights, 4 for directional, 1 for spotlight)
const uint SHADOW_CASCADE_COUNT = 4;                // Cascade count for cascaded shadow maps

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
    uint arrayIndex;
};

struct ShadowMapPayload
{
    vec3 worldPos;                  // World position the shadow map was rendered from
    mat4 viewProjection;            // ViewProjection for the shadow map render

    // Directional shadow map specific
    vec2 cut;                       // Cascade [start, end] world distances, in camera view space
    uint cascadeIndex;              // Cascade index [0..Shadow_Cascade_Count)
};

struct LightPayload
{
    bool isValid;
    uint id;
    bool castsShadows;
    vec3 worldPos;

    // Base light properties
    uint lightType;                 // (LIGHT_TYPE_{X})
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

//
// Helper funcs
//
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    const float a      = roughness*roughness;
    const float a2     = a*a;
    const float NdotH  = max(dot(N, H), 0.0f);
    const float NdotH2 = NdotH*NdotH;

    const float num    = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.f);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    const float r = (roughness + 1.0f);
    const float k = (r*r) / 8.0f;

    const float num   = NdotV;
    const float denom = NdotV * (1.0f - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    const float NdotV = max(dot(N, V), 0.0f);
    const float NdotL = max(dot(N, L), 0.0f);
    const float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    const float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

FragLightingParameters GetFragLightingParameters(PBRMaterialPayload materialPayload);
vec3 CalculateLightRadiance(ObjectInstanceDataPayload instanceData, PBRMaterialPayload materialPayload, LightPayload lightData, FragLightingParameters lightingParams, mat3 normalTransform);
float CalculateLightAttenuation(LightPayload lightData, float fragToLight_distance);
vec3 GetFragmentNormalWorldSpace(PBRMaterialPayload materialPayload, mat4 modelTransform);
bool CanLightAffectFragment(LightPayload light, vec3 fragPosition_worldSpace);
float GetFragShadowLevel(LightPayload lightData, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace);

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

layout(std140, set = 1, binding = 5) uniform ShadowSamplerUniformPayloadBuffer
{
    ShadowSamplerUniformPayload data[SHADER_MAX_SHADOW_MAP_LIGHT_COUNT];
} u_shadowSamplerData;

layout(set = 1, binding = 8) uniform sampler2D i_shadowSampler_single[SHADER_MAX_SHADOW_MAP_SPOTLIGHT_COUNT];
layout(set = 1, binding = 9) uniform samplerCube i_shadowSampler_cube[SHADER_MAX_SHADOW_MAP_POINT_COUNT];
layout(set = 1, binding = 10) uniform sampler2DArray i_shadowSampler_array[SHADER_MAX_SHADOW_MAP_DIRECTIONAL_COUNT];

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

//
// Outputs
//
layout(location = 0) out vec4 o_fragColor;

void main()
{
    const DrawDataPayload drawDataPayload = i_drawData.data[i_instanceIndex];
    const ObjectInstanceDataPayload instanceDataPayload = i_objectInstanceData.data[drawDataPayload.objectId];
    const PBRMaterialPayload materialPayload = i_materialPayloads.data[instanceDataPayload.materialId];

    const FragLightingParameters lightingParams = GetFragLightingParameters(materialPayload);

    // Discard (sufficiently close to) transparent fragments
    if (lightingParams.albedo.a <= 0.01f)
    {
        discard;
    }

    const mat3 normalTransform = mat3(transpose(inverse(u_viewProjectionData.data.viewTransform)));

    vec3 totalLo = vec3(0.0);

    for (uint i = 1; i <= u_globalData.data.highestLightId; ++i)
    {
        const LightPayload lightPayload = i_lightData.data[i];
        if (!lightPayload.isValid)
        {
            continue;
        }

        if (!CanLightAffectFragment(lightPayload, i_fragPos_worldSpace))
        {
            continue;
        }

        totalLo += CalculateLightRadiance(instanceDataPayload, materialPayload, lightPayload, lightingParams, normalTransform);
    }

    const vec3 ambient = u_globalData.data.ambientLight * lightingParams.albedo.xyz * lightingParams.ao;

    const vec3 emissive = materialPayload.hasEmissiveSampler ?
        texture(i_emissionSampler, i_fragTexCoord).rgb :
        materialPayload.emissiveColor;

    vec3 color = totalLo + ambient + emissive;

    // If HDR isn't enabled, clamp the output color to [0..1] range
    if (!u_globalData.data.hdrEnabled)
    {
        color = clamp(color, vec3(0, 0, 0), vec3(1, 1, 1));
    }

    o_fragColor = vec4(color, lightingParams.albedo.a);
}

vec3 CalculateLightRadiance(ObjectInstanceDataPayload instanceData, PBRMaterialPayload materialPayload, LightPayload lightData, FragLightingParameters lightingParams, mat3 normalTransform)
{
    const vec3 fragPos_worldSpace = i_fragPos_worldSpace;
    const vec3 fragPos_viewSpace = (u_viewProjectionData.data.viewTransform * vec4(i_fragPos_worldSpace, 1.0f)).xyz;

    // Calculate whether the fragment is in shadow from the light
    //const float fragShadowLevel = 0.0f;
    const float fragShadowLevel = GetFragShadowLevel(lightData, fragPos_viewSpace, fragPos_worldSpace);

    // If the fragment is in full shadow from the light, the light doesn't affect it, bail out early
    if (abs(fragShadowLevel - 1.0) < EPSILON)
    {
        return vec3(0,0,0);
    }

    const vec3 fragNormal_worldSpace = GetFragmentNormalWorldSpace(materialPayload, instanceData.modelTransform);
    const vec3 fragNormal_viewSpaceUnit = normalize(normalTransform * fragNormal_worldSpace).xyz;

    const vec3 cameraPos_viewSpace = vec3(0,0,0);

    const vec3 fragToCamera_viewSpaceUnit = normalize(cameraPos_viewSpace - fragPos_viewSpace);

    const vec3 F0 = mix(vec3(0.04), lightingParams.albedo.xyz, lightingParams.metallic);

    const vec3 lightPos_worldSpace = lightData.worldPos;
    const vec3 lightPos_viewSpace = (u_viewProjectionData.data.viewTransform * vec4(lightPos_worldSpace, 1.0f)).xyz;

    vec3 fragToLight_viewSpaceUnit = vec3(0,0,0);
    float fragToLightDistance = 0.0f;

    if (lightData.lightType == LIGHT_TYPE_DIRECTIONAL)
    {
        // Directional lights are considered to be infinitely far away with parallel rays, and thus
        // we ignore the position the light is at and just use the opposite of the light's direction
        // as the direction to the light. Also applying the normal matrix to convert the direction
        // vector from world space to view space, so that translation/scale in the view transform
        // doesn't impact the direction vector, only camera rotation.
        fragToLight_viewSpaceUnit = normalize(normalTransform * -lightData.directionUnit);

        // However, we DO use the light's position for attenuation purposes. Note that the typical
        // denominator in the ray/plane intersection is 1 in this case, and so is ignored
        fragToLightDistance = dot((lightData.worldPos - fragPos_worldSpace), -lightData.directionUnit);
    }
    else
    {
        // Point and spot lights eminate from a specific world position, so we can draw a vector from the
        // fragment to the light's position
        fragToLight_viewSpaceUnit = normalize(lightPos_viewSpace - fragPos_viewSpace);

        // Physical world distance between the fragment and the light
        fragToLightDistance = distance(lightPos_viewSpace, fragPos_viewSpace);
    }

    const vec3 H            = normalize(fragToCamera_viewSpaceUnit + fragToLight_viewSpaceUnit);
    const float attenuation = CalculateLightAttenuation(lightData, fragToLightDistance);
    const vec3 radiance     = lightData.color * attenuation;

    // cook-torrance brdf
    const float NDF = DistributionGGX(fragNormal_viewSpaceUnit, H, lightingParams.roughness);
    const float G   = GeometrySmith(fragNormal_viewSpaceUnit, fragToCamera_viewSpaceUnit, fragToLight_viewSpaceUnit, lightingParams.roughness);
    const vec3 F    = FresnelSchlick(max(dot(H, fragToCamera_viewSpaceUnit), 0.0f), F0);

    const vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - lightingParams.metallic;

    const vec3 numerator    = NDF * G * F;
    const float denominator =   4.0 *
                                max(dot(fragNormal_viewSpaceUnit, fragToCamera_viewSpaceUnit), 0.0f) *
                                max(dot(fragNormal_viewSpaceUnit, fragToLight_viewSpaceUnit), 0.0f) + 0.0001f;
    const vec3 specular     = numerator / denominator;

    const float NdotL = max(dot(fragNormal_viewSpaceUnit, fragToLight_viewSpaceUnit), 0.0f);

    const vec3 lighting = (kD * lightingParams.albedo.xyz / PI + specular) * radiance * NdotL;

    return lighting * (1.0f - fragShadowLevel);
}

float CalculateLightAttenuation(LightPayload lightData, float fragToLight_distance)
{
    // Calculate light attenuation
    float lightAttenuation = 1.0f;

    if (lightData.attenuationMode == ATTENUATION_MODE_NONE)
    {
        lightAttenuation = 1.0f;
    }
    else if (lightData.attenuationMode == ATTENUATION_MODE_LINEAR)
    {
        // c1 / d with c1 = 5.0
        // Note: Don't change the constant(s) without updating relevant lighting code
        lightAttenuation = clamp(5.0f / fragToLight_distance, 0.0f, 1.0f);
    }
    else if (lightData.attenuationMode == ATTENUATION_MODE_EXPONENTIAL)
    {
        // 1.0 / (c1 + c2*d^2) with c1 = 1.0, c2 = 0.1
        // Note: Don't change the constant(s) without updating relevant lighting code
        lightAttenuation = 1.0/(1.0f + 0.1f * pow(fragToLight_distance, 2));
    }

    return lightAttenuation;
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

vec3 GetFragmentNormalWorldSpace(PBRMaterialPayload materialPayload, mat4 modelTransform)
{
    if (!materialPayload.hasNormalSampler)
    {
        // If the material doesn't have a normal texture, then we just use the model-space
        // normal that was provided by the vertex shader
        return normalize((modelTransform * vec4(i_fragNormal_modelSpace, 0.0f)).xyz);
    }
    else
    {
        // Otherwise, read the normal from the normal texture
        vec3 normalMapValue = texture(i_normalSampler, i_fragTexCoord).rgb;

        // Convert from RGB space [0..1] to 3D space [-1..1]
        normalMapValue = (normalMapValue * 2.0) - 1.0;

        return normalize(i_tbnNormalTransform * normalMapValue);
    }
}

bool CanLightAffectFragment_Directional(LightPayload light, vec3 fragPosition_worldSpace)
{
    //
    // Verify the fragment is within range of the light
    //
    const float fragToLightPlaneDistance = dot((light.worldPos - fragPosition_worldSpace), -light.directionUnit);

    if (light.attenuationMode != ATTENUATION_MODE_NONE)
    {
        if (abs(fragToLightPlaneDistance) > light.maxAffectRange)
        {
            return false;
        }
    }

    //
    // Verify the fragment falls within the light's emit disk
    //

    // If the fragment is behind the light, it's not affected by it
    if (fragToLightPlaneDistance < 0.0f)
    {
        return false;
    }

    // If the light doesn't use a specific area of effect, then it affects everything in front of it
    if (light.areaOfEffect <= EPSILON)
    {
        return true;
    }

    // Otherwise, do a ray/disk intersection test to see whether the fragment is within the disk on
    // the light plane that light is being emitted from
    const vec3 intersectionPoint = fragPosition_worldSpace + (-light.directionUnit * fragToLightPlaneDistance);
    const float radius = distance(intersectionPoint, light.worldPos);

    // Outside the area of effect disk
    if (radius > light.areaOfEffect)
    {
        return false;
    }

    return true;
}

bool CanLightAffectFragment_Point(LightPayload light, vec3 fragPosition_worldSpace)
{
    //
    // Verify the fragment is within range of the light. As point lights are
    // omnidirectional, location of the frag doesn't matter, just distance.
    //
    const vec3 lightToFrag_worldSpace = fragPosition_worldSpace - light.worldPos;

    if (light.attenuationMode != ATTENUATION_MODE_NONE)
    {
        if (length(lightToFrag_worldSpace) > light.maxAffectRange)
        {
            return false;
        }
    }

    return true;
}

bool CanLightAffectFragment_Spotlight(LightPayload light, vec3 fragPosition_worldSpace)
{
    //
    // Verify the fragment is within range of the light
    //
    const vec3 lightToFrag_worldSpace = fragPosition_worldSpace - light.worldPos;
    const vec3 lightToFrag_worldSpaceUnit = normalize(lightToFrag_worldSpace);

    if (light.attenuationMode != ATTENUATION_MODE_NONE)
    {
        if (length(lightToFrag_worldSpace) > light.maxAffectRange)
        {
            return false;
        }
    }

    //
    // Verify the fragment falls within the light's cone fov
    //
    const float vectorAlignment = dot(light.directionUnit, lightToFrag_worldSpaceUnit);
    const float alignmentAngleDegrees = degrees(acos(vectorAlignment));
    const bool alignedWithLightCone = alignmentAngleDegrees <= light.areaOfEffect / 2.0f;

    return alignedWithLightCone;
}

bool CanLightAffectFragment(LightPayload light, vec3 fragPosition_worldSpace)
{
    if (light.lightType == LIGHT_TYPE_POINT)
    {
        return CanLightAffectFragment_Point(light, fragPosition_worldSpace);
    }
    else if (light.lightType == LIGHT_TYPE_SPOTLIGHT)
    {
        return CanLightAffectFragment_Spotlight(light, fragPosition_worldSpace);
    }
    else if (light.lightType == LIGHT_TYPE_DIRECTIONAL)
    {
        return CanLightAffectFragment_Directional(light, fragPosition_worldSpace);
    }

    return false;
}

float GetFragShadowLevel_Spotlight(LightPayload lightData, ShadowSamplerUniformPayload samplerPayload, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace)
{
    const uint shadowMapPayloadIndex = lightData.id * MAX_PER_LIGHT_SHADOW_RENDER_COUNT;
    const ShadowMapPayload shadowMapPayload = i_shadowMapData.data[shadowMapPayloadIndex];

    // Fragment world position -> light (shadow render) clip space
    const vec4 fragPosition_lightClipSpace = shadowMapPayload.viewProjection * vec4(fragPosition_worldSpace, 1.0);

    // If the fragment isn't within the bounds of the light render, it's not in shadow from it
    if (abs(fragPosition_lightClipSpace.x) > fragPosition_lightClipSpace.w ||
        abs(fragPosition_lightClipSpace.y) > fragPosition_lightClipSpace.w ||
        fragPosition_lightClipSpace.z > fragPosition_lightClipSpace.w || // reversed-Z: far is at z = 0
        fragPosition_lightClipSpace.z < 0.0)
    {
        o_fragColor = vec4(0,0,0,1);
        return 0.0f;
    }

    // Light clip space - > Light NDC space
    const vec3 fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;

    // Distance from light to frag, [0..1] from near to far, across the light's max affect range
    const float fragmentDepth = length(fragPosition_worldSpace - lightData.worldPos) / lightData.maxAffectRange;

    // Convert NDC xy to shadow map texture coordinates
    vec2 shadowUV = fragPosition_lightNDCSpace.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;

    // PCF filter
    const float bias = 0.0005f;
    const vec2 texelSize = 1.0 / textureSize(i_shadowSampler_single[nonuniformEXT(samplerPayload.arrayIndex)], 0).xy;
    const int sampleSize = 2;

    float shadowLevel = 0.0f;

    for (int x = -sampleSize; x <= sampleSize; ++x)
    {
        for (int y = -sampleSize; y <= sampleSize; ++y)
        {
            const vec2 samplePoint = shadowUV + (vec2(x, y) * texelSize);

            // Subtracting from 1 to convert from [1..0] z-axis in shadow map to [0..1]
            const float sampledDepth = 1.0f - texture(i_shadowSampler_single[nonuniformEXT(samplerPayload.arrayIndex)], samplePoint).r;

            const bool inShadow = sampledDepth + bias < fragmentDepth;
            shadowLevel += inShadow ? 1.0 : 0.0;
        }
    }

    return shadowLevel / pow((sampleSize * 2.0f + 1.0f), 2.0f);
}

float GetFragShadowLevel_Point(LightPayload lightData, ShadowSamplerUniformPayload samplerPayload, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace)
{
    const vec3 lightToFrag_worldSpace = fragPosition_worldSpace - lightData.worldPos;

    // Distance from light to frag, [0..1] from near to far, across the light's max affect range
    const float fragmentDepth = length(lightToFrag_worldSpace) / lightData.maxAffectRange;

    // Convert to cube-map left-handed coordinate system with swapped z-axis, and normalized
    const vec3 sampleVector = normalize(lightToFrag_worldSpace * vec3(1,1,-1));

    // PCF filter
    int sampleSize = 1;
    const float diskScale = 0.1f;
    const float diskRadius = diskScale * fragmentDepth; // Use distance to frag to determine radius to kernel over

    const float bias = 0.0005f;
    float shadowLevel = 0.0f;

    for (int x = -sampleSize; x <= sampleSize; ++x)
    {
        for (int y = -sampleSize; y <= sampleSize; ++y)
        {
            for (int z = -sampleSize; z <= sampleSize; ++z)
            {
                const vec3 offsetSampleVector = normalize(sampleVector + (vec3(x, y, z) * diskRadius));

                const float sampledDepth = 1.0f - texture(i_shadowSampler_cube[nonuniformEXT(samplerPayload.arrayIndex)], offsetSampleVector).r;

                const bool inShadow = sampledDepth + bias < fragmentDepth;
                shadowLevel += inShadow ? 1.0 : 0.0;
            }
        }
    }

    return shadowLevel / pow((sampleSize * 2.0f + 1.0f), 3.0f);
}

bool GetFragCascadeShadowMapPayloadIndex(LightPayload lightData, vec3 fragPosition_viewSpace, out uint payloadIndex)
{
    // Z-distance along the camera view projection
    const float fragDistance_viewSpace = abs(fragPosition_viewSpace.z);

    const uint lightShadowMapPayloadsBeginIndex = lightData.id * MAX_PER_LIGHT_SHADOW_RENDER_COUNT;

    // Look through the shadow map payload at each of the cascade shadow renders, looking for the nearest
    // cascade render which overlaps with the frag
    for (uint cascadeIndex = 0; cascadeIndex < SHADOW_CASCADE_COUNT; ++cascadeIndex)
    {
        const uint shadowMapPayloadIndex = lightShadowMapPayloadsBeginIndex + cascadeIndex;

        const ShadowMapPayload shadowMapPayload = i_shadowMapData.data[shadowMapPayloadIndex];

        if ((fragDistance_viewSpace >= shadowMapPayload.cut.x) && (fragDistance_viewSpace <= shadowMapPayload.cut.y))
        {
            payloadIndex = shadowMapPayloadIndex;
            return true;
        }
    }

    return false;
}

float GetFragShadowLevel_Cascaded(LightPayload lightData, ShadowSamplerUniformPayload samplerPayload, ShadowMapPayload shadowMap, vec3 fragPosition_worldSpace, float lightToFragDepth)
{
    const vec4 fragPosition_lightClipSpace = shadowMap.viewProjection * vec4(fragPosition_worldSpace, 1);

    // Sanity check the fragment is within the shadow map
    if ((abs(fragPosition_lightClipSpace.x) > fragPosition_lightClipSpace.w) ||
        (abs(fragPosition_lightClipSpace.y) > fragPosition_lightClipSpace.w) ||
        (fragPosition_lightClipSpace.z > fragPosition_lightClipSpace.w) ||
        (fragPosition_lightClipSpace.z < 0.0f))
    {
        return 0.0f;
    }

    const vec3 fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;

    // Convert NDC xy to shadow map texture coordinates
    vec2 shadowUV = fragPosition_lightNDCSpace.xy * 0.5 + 0.5;
    shadowUV.y = 1.0 - shadowUV.y;

    const vec2 texelSize = 1.0 / textureSize(i_shadowSampler_array[nonuniformEXT(samplerPayload.arrayIndex)], 0).xy;

    float shadowLevel = 0.0;
    int sampleSize = 1;

    // PCF filter
    const float bias = 0.0005f;

    for (int x = -sampleSize; x <= sampleSize; ++x)
    {
        for (int y = -sampleSize; y <= sampleSize; ++y)
        {
            // [0..1] from close to far
            const float sampledDepth = 1.0f - texture(
                i_shadowSampler_array[nonuniformEXT(samplerPayload.arrayIndex)],
                vec3(shadowUV + (vec2(x, y) * texelSize), shadowMap.cascadeIndex)
            ).r;

            const bool inShadow = sampledDepth + bias < lightToFragDepth;
            shadowLevel += inShadow ? 1.0 : 0.0;
        }
    }

    return shadowLevel / pow((sampleSize * 2.0f + 1.0f), 2.0f);
}

float GetFragShadowLevel_Cascaded(LightPayload lightData, ShadowSamplerUniformPayload samplerPayload, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace)
{
    const float fragDistance_viewSpace = abs(fragPosition_viewSpace.z);

    //
    // Get the ShadowMapPayload for the cascade the fragment falls within
    //
    uint shadowMapPayloadIndex;

    if (!GetFragCascadeShadowMapPayloadIndex(lightData, fragPosition_viewSpace, shadowMapPayloadIndex))
    {
        return 0.0f;
    }

    ShadowMapPayload shadowMap = i_shadowMapData.data[shadowMapPayloadIndex];

    const float cutRange = shadowMap.cut.y - shadowMap.cut.x;
    const float cutBlendRange = cutRange * u_globalData.data.shadowCascadeOverlap;
    const float cutBlendStart = shadowMap.cut.y - cutBlendRange;

    const bool fragWithinCutBlendBand = (fragDistance_viewSpace >= cutBlendStart) &&
                                        (shadowMap.cascadeIndex < (SHADOW_CASCADE_COUNT - 1));

    vec4 fragPosition_lightClipSpace = shadowMap.viewProjection * vec4(fragPosition_worldSpace, 1);
    vec3 fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;

    // [0..1] from close to far
    float lightToFragDepth = 1.0f - abs(fragPosition_lightNDCSpace.z);

    const float fragShadowLevelMain = GetFragShadowLevel_Cascaded(lightData, samplerPayload, shadowMap, fragPosition_worldSpace, lightToFragDepth);

    //
    // If the fragment doesn't fall within the cut blend band, return the shadow level as-is
    //
    if (!fragWithinCutBlendBand)
    {
        return fragShadowLevelMain;
    }

    //
    // Otherwise, get the shadow level from the next cascade as well, and blend the two shadow levels together
    //
    shadowMap = i_shadowMapData.data[shadowMapPayloadIndex + 1];
    fragPosition_lightClipSpace = shadowMap.viewProjection * vec4(fragPosition_worldSpace, 1);
    fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;

    // [0..1] from close to far
    lightToFragDepth = 1.0f - fragPosition_lightNDCSpace.z;

    const float fragShadowLevelNext = GetFragShadowLevel_Cascaded(lightData, samplerPayload, shadowMap, fragPosition_worldSpace, lightToFragDepth);

    //
    // Blend the two levels together depending on progress through the blend band
    //
    const float percentWithinBlendBand = (fragDistance_viewSpace - cutBlendStart) / cutBlendRange;

    return (fragShadowLevelMain * (1.0f - percentWithinBlendBand)) + (fragShadowLevelNext * percentWithinBlendBand);
}

bool GetShadowSamplerUniformPayload(uint lightId, out ShadowSamplerUniformPayload result)
{
    for (uint x = 0; x < SHADER_MAX_SHADOW_MAP_LIGHT_COUNT; ++x)
    {
        const ShadowSamplerUniformPayload samplerPayload = u_shadowSamplerData.data[x];

        if (samplerPayload.lightId == lightId)
        {
            result = samplerPayload;
            return true;
        }
    }

    return false;
}

float GetFragShadowLevel(LightPayload lightData, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace)
{
    if (!lightData.castsShadows)
    {
        return 0.0f;
    }

    ShadowSamplerUniformPayload samplerPayload;

    if (!GetShadowSamplerUniformPayload(lightData.id, samplerPayload))
    {
        return 0.0f;
    }

    switch (lightData.lightType)
    {
        case LIGHT_TYPE_SPOTLIGHT:  { return GetFragShadowLevel_Spotlight(lightData, samplerPayload, fragPosition_viewSpace, fragPosition_worldSpace); }
        case LIGHT_TYPE_POINT:      { return GetFragShadowLevel_Point(lightData, samplerPayload, fragPosition_viewSpace, fragPosition_worldSpace); }
        case LIGHT_TYPE_DIRECTIONAL:{ return GetFragShadowLevel_Cascaded(lightData, samplerPayload, fragPosition_viewSpace, fragPosition_worldSpace); }

        // Unsupported light type, return no shadow since we don't know how to access its shadow map
        default: {  return 0.0f; }
    }
}
