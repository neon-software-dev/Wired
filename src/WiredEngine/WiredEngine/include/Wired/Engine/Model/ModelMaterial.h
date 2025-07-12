/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELMATERIAL_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELMATERIAL_H

#include <Wired/Render/MaterialCommon.h>
#include <Wired/GPU/GPUSamplerCommon.h>

#include <NEON/Common/ImageData.h>

#include <glm/glm.hpp>

#include <vector>
#include <string>
#include <optional>
#include <cstddef>
#include <unordered_map>

namespace Wired::Engine
{
    struct ModelEmbeddedData
    {
        std::vector<std::byte> data;
        std::size_t dataWidth{0};
        std::size_t dataHeight{0};
        std::optional<std::string> dataFormat;
    };

    struct ModelTexture
    {
        std::string fileName;
        GPU::SamplerAddressMode uSamplerAddressMode{GPU::SamplerAddressMode::Clamp};
        GPU::SamplerAddressMode vSamplerAddressMode{GPU::SamplerAddressMode::Clamp};
        GPU::SamplerAddressMode wSamplerAddressMode{GPU::SamplerAddressMode::Clamp};

        std::optional<ModelEmbeddedData> embeddedData;
    };

    enum class ModelTextureType
    {
        // Blinn material
        Diffuse,
        Opacity,

        // PBR material
        Albedo,
        Metallic,
        Roughness,
        Normal,
        AO,
        Emission
    };

    /**
     * Properties of a specific material that a model uses
     */
    struct ModelMaterial
    {
        enum class Type
        {
            Blinn,
            PBR
        };

        virtual ~ModelMaterial() = default;

        [[nodiscard]] virtual Type GetType() const = 0;

        std::string name;
        unsigned int materialIndex{0};
        std::optional<Render::MaterialAlphaMode> alphaMode;
        std::optional<float> alphaCutoff;
        bool twoSided{false};
        std::unordered_map<ModelTextureType, ModelTexture> textures;
    };

    struct ModelBlinnMaterial : public ModelMaterial
    {
        [[nodiscard]] Type GetType() const override { return Type::Blinn; };

        glm::vec4 diffuseColor{1.0f};
        glm::vec3 specularColor{1.0f};
        glm::vec3 emissiveColor{0.0f};
        float shininess{0.0f};
        float opacity{1.0f};
    };

    struct ModelPBRMaterial : public ModelMaterial
    {
        [[nodiscard]] Type GetType() const override { return Type::PBR; };

        glm::vec4 albedoColor{1.0f};
        glm::vec3 emissiveColor{0.0f};
        float metallicFactor{1.0f};
        float roughnessFactor{1.0f};
    };

    inline bool IsLinearModelTextureType(ModelTextureType type)
    {
        return
            type == ModelTextureType::Normal ||
            type == ModelTextureType::Metallic ||
            type == ModelTextureType::Roughness ||
            type == ModelTextureType::AO;
    }

    inline std::optional<Render::MaterialTextureType> ToRenderMaterialTextureType(ModelTextureType modelTextureType)
    {
        switch (modelTextureType)
        {
            case ModelTextureType::Diffuse: return std::nullopt;
            case ModelTextureType::Opacity: return std::nullopt;
            case ModelTextureType::Albedo: return Render::MaterialTextureType::Albedo;
            case ModelTextureType::Metallic: return Render::MaterialTextureType::Metallic;
            case ModelTextureType::Roughness: return Render::MaterialTextureType::Roughness;
            case ModelTextureType::Normal: return Render::MaterialTextureType::Normal;
            case ModelTextureType::AO: return Render::MaterialTextureType::AO;
            case ModelTextureType::Emission: return Render::MaterialTextureType::Emission;
        }

        assert(false);
        return std::nullopt;
    }
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELMATERIAL_H
