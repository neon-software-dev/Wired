/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_TEXTURES_H
#define WIREDENGINE_WIREDRENDERER_SRC_TEXTURES_H

#include <Wired/Render/Id.h>
#include <Wired/Render/TextureCommon.h>
#include <Wired/GPU/GPUCommon.h>

#include <NEON/Common/Space/Size2D.h>

#include <expected>
#include <optional>
#include <unordered_map>
#include <mutex>

namespace Wired::Render
{
    struct Global;

    struct LoadedTexture
    {
        TextureCreateParams createParams{};
        GPU::ImageId imageId;
    };

    struct TextureTransfer
    {
        // Source
        std::byte const* data{nullptr};
        std::size_t dataByteSize{0};

        // Destination
        TextureId textureId{};
        uint32_t level{0};
        uint32_t layer{0};
        std::optional<NCommon::Size2DUInt> destSize;
        uint32_t x{0};
        uint32_t y{0};
        uint32_t z{0};
        uint32_t d{1};
        bool cycle{true};
    };

    class Textures
    {
        public:

            explicit Textures(Global* pGlobal);
            ~Textures();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            [[nodiscard]] std::expected<TextureId, bool> CreateFromParams(GPU::CommandBufferId commandBufferId,
                                                                          const TextureCreateParams& params,
                                                                          const std::string& tag);

            [[nodiscard]] std::optional<LoadedTexture> GetTexture(TextureId textureId) const;

            [[nodiscard]] LoadedTexture GetMissingTexture2D() const { return *GetTexture(m_missingTexture2D); }
            [[nodiscard]] LoadedTexture GetMissingTextureCube() const { return *GetTexture(m_missingTextureCube); }
            [[nodiscard]] LoadedTexture GetMissingTextureArray() const { return *GetTexture(m_missingTextureArray); }

            [[nodiscard]] bool TransferData(GPU::CommandBufferId commandBufferId, const std::vector<TextureTransfer>& transfers);

            [[nodiscard]] bool GenerateMipMaps(GPU::CommandBufferId commandBufferId, TextureId textureId);

            void DestroyTexture(TextureId textureId);

        private:

            [[nodiscard]] bool CreateMissingTextures();

        private:

            Global* m_pGlobal;

            std::unordered_map<TextureId, LoadedTexture> m_textures;
            mutable std::recursive_mutex m_texturesMutex;

            TextureId m_missingTexture2D;
            TextureId m_missingTextureCube;
            TextureId m_missingTextureArray; // Note: This is a 4 layer array
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_TEXTURES_H
