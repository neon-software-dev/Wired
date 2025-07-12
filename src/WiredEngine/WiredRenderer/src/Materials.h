/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_MATERIALS_H
#define WIREDENGINE_WIREDRENDERER_SRC_MATERIALS_H

#include "ItemBuffer.h"

#include <Wired/Render/Id.h>
#include <Wired/Render/Material/Material.h>

#include <expected>
#include <string>
#include <vector>
#include <cstddef>
#include <unordered_map>
#include <optional>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Render
{
    struct Global;

    struct LoadedMaterial
    {
        MaterialType materialType{};
        std::optional<MaterialAlphaMode> alphaMode;
        std::optional<float> alphaCutoff;
        bool twoSided{false};

        std::unordered_map<MaterialTextureType, MaterialTextureBinding> textureBindings;
    };

    class Materials
    {
        public:

            explicit Materials(Global* pGlobal);
            ~Materials();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            [[nodiscard]] std::expected<std::vector<MaterialId>, bool> CreateMaterials(const std::vector<const Material*>& materials, const std::string& userTag);
            [[nodiscard]] std::optional<LoadedMaterial> GetMaterial(const MaterialId& materialId) const;
            [[nodiscard]] bool UpdateMaterial(const MaterialId& materialId, const Material* pMaterial);
            void DestroyMaterial(const MaterialId& materialId);

            [[nodiscard]] GPU::BufferId GetMaterialPayloadsBuffer() const noexcept { return m_materialPayloadsBuffer.GetBufferId(); }

        private:

            struct PBRMaterialPayload
            {
                alignas(4) uint32_t alphaMode{(uint32_t)MaterialAlphaMode::Opaque};
                alignas(4) float alphaCutoff{1.0f};

                alignas(16) glm::vec4 albedoColor{1.0f};
                alignas(4) uint32_t hasAlbedoSampler{0};

                alignas(4) float metallicFactor{1.0f};
                alignas(4) uint32_t hasMetallicSampler{0};

                alignas(4) float roughnessFactor{1.0f};
                alignas(4) uint32_t hasRoughnessSampler{0};

                alignas(4) uint32_t hasNormalSampler{0};

                alignas(4) uint32_t hasAOTexture{0};

                alignas(16) glm::vec3 emissiveColor{0.0f};
                alignas(4) uint32_t hasEmissiveSampler{0};
            };

        private:

            [[nodiscard]] static PBRMaterialPayload GetMaterialPayload(const Material* pMaterial);

            [[nodiscard]] static std::string GetTag(const MaterialId& materialId, const std::string& userTag);

        private:

            Global* m_pGlobal;

            std::unordered_map<MaterialId, LoadedMaterial> m_materials;

            ItemBuffer<PBRMaterialPayload> m_materialPayloadsBuffer;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_MATERIALS_H
