/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MATERIAL_MATERIAL_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MATERIAL_MATERIAL_H

#include "../MaterialCommon.h"
#include "../Id.h"

#include <Wired/GPU/GPUSamplerCommon.h>

#include <glm/glm.hpp>

#include <unordered_map>
#include <optional>
#include <memory>

namespace Wired::Render
{
    struct MaterialTextureBinding
    {
        TextureId textureId{};
        GPU::SamplerAddressMode uSamplerAddressMode{GPU::SamplerAddressMode::Clamp};
        GPU::SamplerAddressMode vSamplerAddressMode{GPU::SamplerAddressMode::Clamp};
        GPU::SamplerAddressMode wSamplerAddressMode{GPU::SamplerAddressMode::Clamp};
    };

    struct Material
    {
        virtual ~Material() = default;

        [[nodiscard]] virtual MaterialType GetType() const = 0;

        std::optional<MaterialAlphaMode> alphaMode;
        std::optional<float> alphaCutoff;
        bool twoSided{false};

        std::unordered_map<MaterialTextureType, MaterialTextureBinding> textureBindings;
    };

    struct PBRMaterial : public Material
    {
        [[nodiscard]] MaterialType GetType() const override { return MaterialType::PBR; }

        glm::vec4 albedoColor{1.0f};
        glm::vec3 emissiveColor{0.0f};
        float metallicFactor{1.0f};
        float roughnessFactor{1.0f};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MATERIAL_MATERIAL_H
