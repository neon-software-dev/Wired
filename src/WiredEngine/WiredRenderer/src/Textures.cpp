/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Textures.h"

#include "Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <NEON/Common/ImageData.h>
#include <NEON/Common/Log/ILogger.h>

#include <cstring>

namespace Wired::Render
{

Textures::Textures(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Textures::~Textures()
{
    m_pGlobal = nullptr;
}

bool Textures::StartUp()
{
    m_pGlobal->pLogger->Info("Textures: Starting Up");

    if (!CreateMissingTextures())
    {
        m_pGlobal->pLogger->Fatal("Textures::StartUp: Failed to create missing textures");
        return false;
    }

    return true;
}

void Textures::ShutDown()
{
    m_pGlobal->pLogger->Info("Textures: Shutting down");

    std::lock_guard<std::recursive_mutex> lock(m_texturesMutex);

    while (!m_textures.empty())
    {
        DestroyTexture(m_textures.cbegin()->first);
    }
}

std::expected<TextureId, bool> Textures::CreateFromParams(GPU::CommandBufferId commandBufferId, const TextureCreateParams& params, const std::string& tag)
{
    GPU::ImageCreateParams imageCreateParams{};

    switch (params.textureType)
    {
        case TextureType::Texture2D: imageCreateParams.imageType = GPU::ImageType::Image2D; break;
        case TextureType::Texture2DArray: imageCreateParams.imageType = GPU::ImageType::Image2DArray; break;
        case TextureType::Texture3D: imageCreateParams.imageType = GPU::ImageType::Image3D; break;
        case TextureType::TextureCube: imageCreateParams.imageType = GPU::ImageType::ImageCube; break;
    }

    if (params.usageFlags.contains(TextureUsageFlag::GraphicsSampled)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::GraphicsSampled); }
    if (params.usageFlags.contains(TextureUsageFlag::ComputeSampled)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::ComputeSampled); }
    if (params.usageFlags.contains(TextureUsageFlag::ColorTarget)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::ColorTarget); }
    if (params.usageFlags.contains(TextureUsageFlag::DepthStencilTarget)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::DepthStencilTarget); }
    if (params.usageFlags.contains(TextureUsageFlag::PostProcess)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::PostProcess); }
    if (params.usageFlags.contains(TextureUsageFlag::TransferSrc)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::TransferSrc); }
    if (params.usageFlags.contains(TextureUsageFlag::TransferDst)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::TransferDst); }
    if (params.usageFlags.contains(TextureUsageFlag::GraphicsStorageRead)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::GraphicsStorageRead); }
    if (params.usageFlags.contains(TextureUsageFlag::ComputeStorageRead)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::ComputeStorageRead); }
    if (params.usageFlags.contains(TextureUsageFlag::ComputeStorageReadWrite)) { imageCreateParams.usageFlags.insert(GPU::ImageUsageFlag::ComputeStorageReadWrite); }

    imageCreateParams.size = params.size;
    imageCreateParams.colorSpace = params.colorSpace;
    imageCreateParams.numLayers = params.numLayers;
    imageCreateParams.numMipLevels = params.numMipLevels;

    const auto imageId = m_pGlobal->pGPU->CreateImage(commandBufferId, imageCreateParams, tag);
    if (!imageId)
    {
        m_pGlobal->pLogger->Error("Textures::CreateFromParams: Failed to create image for the texture");
        return std::unexpected(false);
    }

    const LoadedTexture loadedTexture{
        .createParams = params,
        .imageId = *imageId
    };

    const auto textureId = m_pGlobal->ids.textureIds.GetId();

    std::lock_guard<std::recursive_mutex> lock(m_texturesMutex);
    m_textures.insert({textureId, loadedTexture});

    return textureId;
}

std::optional<LoadedTexture> Textures::GetTexture(TextureId textureId) const
{
    std::lock_guard<std::recursive_mutex> lock(m_texturesMutex);

    const auto it = m_textures.find(textureId);
    if (it == m_textures.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

bool Textures::TransferData(GPU::CommandBufferId commandBufferId, const std::vector<TextureTransfer>& transfers)
{
    //
    // Determine the total byte size of all data that's being transferred
    //
    std::size_t totalTransferByteSize{0};
    std::vector<LoadedTexture> transferTextures;

    for (const auto& transfer : transfers)
    {
        const auto loadedTexture = GetTexture(transfer.textureId);
        if (!loadedTexture)
        {
            m_pGlobal->pLogger->Error("Textures::TransferData: No such texture exists: {}", transfer.textureId.id);
            return false;
        }

        totalTransferByteSize += transfer.dataByteSize;
        transferTextures.push_back(*loadedTexture);
    }

    //
    // Create a transfer buffer large enough to hold all the transfer data
    //
    assert(totalTransferByteSize <= std::numeric_limits<uint32_t>::max());

    const GPU::TransferBufferCreateParams transferBufferCreateParams{
        .usageFlags = {GPU::TransferBufferUsageFlag::Upload},
        .byteSize = static_cast<uint32_t>(totalTransferByteSize),
        .sequentiallyWritten = true
    };

    const auto transferBufferId = m_pGlobal->pGPU->CreateTransferBuffer(transferBufferCreateParams, "TransferTexture");
    if (!transferBufferId)
    {
        m_pGlobal->pLogger->Error("Textures::TransferData: Failed to create transfer buffer");
        return false;
    }

    //
    // Map the transfer buffer into memory and fill it with data
    //
    auto pTransferData = m_pGlobal->pGPU->MapBuffer(*transferBufferId, false);
    if (!pTransferData)
    {
        m_pGlobal->pLogger->Error("Textures::TransferData: Failed to map the transfer buffer");
        m_pGlobal->pGPU->DestroyBuffer(*transferBufferId);
        return false;
    }

    std::vector<std::size_t> transferStartOffsets;
    std::size_t bytePosition = 0;

    for (const auto& transfer : transfers)
    {
        transferStartOffsets.push_back(bytePosition);
        memcpy(static_cast<std::byte*>(*pTransferData) + bytePosition, transfer.data, transfer.dataByteSize);
        bytePosition += transfer.dataByteSize;
    }

    (void)m_pGlobal->pGPU->UnmapBuffer(*transferBufferId);

    //
    // Start a copy pass containing a copy command for each transfer
    //
    const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(commandBufferId, "TextureDataTransfer");

    for (unsigned int x = 0; x < transfers.size(); ++x)
    {
        const auto& transfer = transfers.at(x);
        const auto& loadedTexture = transferTextures.at(x);

        const uint32_t destWidth = transfer.destSize.has_value() ? transfer.destSize->w : loadedTexture.createParams.size.w;
        const uint32_t destHeight = transfer.destSize.has_value() ? transfer.destSize->h : loadedTexture.createParams.size.h;

        m_pGlobal->pGPU->CmdUploadDataToImage(
            *copyPass,
            *transferBufferId,
            transferStartOffsets.at(x),
            loadedTexture.imageId,
            GPU::ImageRegion{
                .layerIndex = transfer.layer,
                .mipLevel = transfer.level,
                .offsets = {
                    NCommon::Point3DUInt{(uint32_t)transfer.x, (uint32_t)transfer.y, 0},
                    NCommon::Point3DUInt{(uint32_t)(transfer.x + destWidth), (uint32_t)(transfer.y + destHeight), transfer.d}
                }
            },
            transfer.dataByteSize,
            transfer.cycle
        );
    }

    m_pGlobal->pGPU->EndCopyPass(*copyPass);

    //
    // Cleanup
    //
    m_pGlobal->pGPU->DestroyBuffer(*transferBufferId);

    return true;
}

bool Textures::GenerateMipMaps(GPU::CommandBufferId commandBufferId, TextureId textureId)
{
    const auto loadedTexture = GetTexture(textureId);
    if (!loadedTexture)
    {
        m_pGlobal->pLogger->Error("Textures::GenerateMipMaps: No such texture exists: {}", textureId.id);
        return false;
    }

    if (!m_pGlobal->pGPU->GenerateMipMaps(commandBufferId, loadedTexture->imageId))
    {
        m_pGlobal->pLogger->Error("Textures::GenerateMipMaps: Call to generate mipmaps failed: {}", textureId.id);
        return false;
    }

    return true;
}

void Textures::DestroyTexture(TextureId textureId)
{
    std::lock_guard<std::recursive_mutex> lock(m_texturesMutex);

    const auto loadedTexture = GetTexture(textureId);
    if (!loadedTexture)
    {
        return;
    }

    m_pGlobal->pLogger->Debug("Textures: Destroying texture: {} (image: {})", textureId.id, loadedTexture->imageId.id);

    m_pGlobal->pGPU->DestroyImage(loadedTexture->imageId);

    m_textures.erase(textureId);
}

bool Textures::CreateMissingTextures()
{
    const unsigned int sizePx = 256;
    const unsigned int squareSizePx = 32;
    const std::array<std::byte, 4> squareOnColor{std::byte{255}, std::byte{0}, std::byte{255}, std::byte{255}};
    const std::array<std::byte, 4> squareOffColor{std::byte{0}, std::byte{0}, std::byte{0}, std::byte{255}};

    std::vector<std::byte> missingTextureData(sizePx * sizePx * 4, std::byte{0});

    for (unsigned int y = 0; y < sizePx; ++y)
    {
        for (unsigned int x = 0; x < sizePx; ++x)
        {
            const unsigned int row = y / squareSizePx;
            const unsigned int col = x / squareSizePx;
            const bool on = (((row % 2) + (col % 2)) % 2) == 0;
            const auto color = on ? squareOnColor : squareOffColor;

            missingTextureData[(y * sizePx * 4) + (x * 4)] = color[0];
            missingTextureData[(y * sizePx * 4) + (x * 4) + 1] = color[1];
            missingTextureData[(y * sizePx * 4) + (x * 4) + 2] = color[2];
            missingTextureData[(y * sizePx * 4) + (x * 4) + 3] = color[3];
        }
    }

    const auto missingTextureImage = std::make_unique<NCommon::ImageData>(
        missingTextureData,
        1,
        sizePx,
        sizePx,
        NCommon::ImageData::PixelFormat::B8G8R8A8_SRGB
    );

    const auto commandBufferId = m_pGlobal->pGPU->AcquireCommandBuffer(true, "TransferMissingTextures");
    if (!commandBufferId)
    {
        m_pGlobal->pLogger->Error("Textures::CreateMissingTexture: Failed to allocate a command buffer");
        return false;
    }

    //
    // Missing 2D Texture
    //
    {
        const auto textureCreateParams = TextureCreateParams {
            .textureType = TextureType::Texture2D,
            .usageFlags = {TextureUsageFlag::GraphicsSampled, TextureUsageFlag::TransferDst},
            .size = {(unsigned int)missingTextureImage->GetPixelWidth(), (unsigned int)missingTextureImage->GetPixelHeight(), 1},
            .numLayers = 1,
            .numMipLevels = 1
        };

        auto result = CreateFromParams(*commandBufferId, textureCreateParams, "Missing2D");
        if (!result)
        {
            m_pGlobal->pLogger->Error("Textures::CreateMissingTexture: Failed to create 2D missing texture");
            (void)m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
            return false;
        }

        TextureTransfer textureTransfer{};
        textureTransfer.data = missingTextureImage->GetPixelData();
        textureTransfer.dataByteSize = missingTextureImage->GetTotalByteSize();
        textureTransfer.textureId = *result;
        textureTransfer.layer = 0;
        textureTransfer.cycle = false;

        if (!TransferData(*commandBufferId, {textureTransfer}))
        {
            m_pGlobal->pLogger->Error("Textures::CreateMissingTexture: Failed to transfer 2D missing texture data");
            (void)m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
            DestroyTexture(*result);
            return false;
        }

        m_missingTexture2D = *result;
    }

    //
    // Missing Cube Texture
    //
    {
        const auto textureCreateParams = TextureCreateParams {
            .textureType = TextureType::TextureCube,
            .usageFlags = {TextureUsageFlag::GraphicsSampled, TextureUsageFlag::TransferDst},
            .size = {(unsigned int)missingTextureImage->GetPixelWidth(), (unsigned int)missingTextureImage->GetPixelHeight(), 1},
            .numLayers = 6,
            .numMipLevels = 1
        };

        auto result = CreateFromParams(*commandBufferId, textureCreateParams, "MissingCube");
        if (!result)
        {
            m_pGlobal->pLogger->Error("Textures::CreateMissingTexture: Failed to create cube missing texture");
            (void)m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
            return false;
        }

        for (unsigned int layerIndex = 0; layerIndex < 6; ++layerIndex)
        {
            TextureTransfer textureTransfer{};
            textureTransfer.data = missingTextureImage->GetPixelData();
            textureTransfer.dataByteSize = missingTextureImage->GetTotalByteSize();
            textureTransfer.textureId = *result;
            textureTransfer.layer = layerIndex;
            textureTransfer.cycle = false;

            if (!TransferData(*commandBufferId, {textureTransfer}))
            {
                m_pGlobal->pLogger->Error("Textures::CreateMissingTexture: Failed to transfer cube missing texture data");
                (void) m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
                DestroyTexture(*result);
                return false;
            }
        }

        m_missingTextureCube = *result;
    }

    //
    // Missing Array Texture
    //
    {
        const auto textureCreateParams = TextureCreateParams {
            .textureType = TextureType::Texture2DArray,
            .usageFlags = {TextureUsageFlag::GraphicsSampled, TextureUsageFlag::TransferDst},
            .size = {(unsigned int)missingTextureImage->GetPixelWidth(), (unsigned int)missingTextureImage->GetPixelHeight(), 1},
            .numLayers = 4,
            .numMipLevels = 1
        };

        auto result = CreateFromParams(*commandBufferId, textureCreateParams, "MissingArray");
        if (!result)
        {
            m_pGlobal->pLogger->Error("Textures::CreateMissingTexture: Failed to create array missing texture");
            (void)m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
            return false;
        }

        for (unsigned int x = 0; x < 4; ++x)
        {
            TextureTransfer textureTransfer{};
            textureTransfer.data = missingTextureImage->GetPixelData();
            textureTransfer.dataByteSize = missingTextureImage->GetTotalByteSize();
            textureTransfer.textureId = *result;
            textureTransfer.layer = x;
            textureTransfer.cycle = false;

            if (!TransferData(*commandBufferId, {textureTransfer}))
            {
                m_pGlobal->pLogger->Error("Textures::CreateMissingTexture: Failed to transfer array missing texture data");
                (void) m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
                DestroyTexture(*result);
                return false;
            }
        }

        m_missingTextureArray = *result;
    }

    //
    // Finish
    //
    (void)m_pGlobal->pGPU->SubmitCommandBuffer(*commandBufferId);

    return true;
}

}
