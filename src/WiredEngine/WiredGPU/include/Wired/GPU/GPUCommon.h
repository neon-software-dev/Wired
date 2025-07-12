/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUCOMMON_H
#define WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUCOMMON_H

#include "GPUId.h"

#include <NEON/Common/Space/Point3D.h>
#include <NEON/Common/Space/Size3D.h>

#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <variant>

namespace Wired::GPU
{
    enum class SurfaceError
    {
        SurfaceInvalidated,
        SurfaceLost,
        Other
    };

    enum class Filter
    {
        Linear,
        Nearest
    };

    enum class IndexType
    {
        Uint16,
        Uint32
    };

    enum class ShaderType
    {
        Vertex,
        Fragment,
        Compute
    };

    enum class ShaderBinaryType
    {
        SPIRV
    };

    struct ShaderSpec
    {
        std::string shaderName;
        ShaderType shaderType{};
        ShaderBinaryType binaryType{};
        std::vector<std::byte> shaderBinary;
    };

    struct CopyPass{ CommandBufferId commandBufferId{}; };
    struct RenderPass{ CommandBufferId commandBufferId{}; };
    struct ComputePass{ CommandBufferId commandBufferId{}; };

    using RenderOrComputePass = std::variant<RenderPass, ComputePass>;

    struct IndirectDrawCommand
    {
        uint32_t indexCount{0};
        uint32_t instanceCount{0};
        uint32_t firstIndex{0};
        int32_t vertexOffset{0};
        uint32_t firstInstance{0};
    };

    //
    // Images
    //

    enum class ImageType
    {
        Image2D,
        Image2DArray,
        Image3D,
        ImageCube
    };

    enum class ImageUsageFlag
    {
        GraphicsSampled,
        ComputeSampled,
        ColorTarget,
        DepthStencilTarget,
        PostProcess,
        TransferSrc,
        TransferDst,
        GraphicsStorageRead,
        ComputeStorageRead,
        ComputeStorageReadWrite
    };

    using ImageUsageFlags = std::unordered_set<ImageUsageFlag>;

    enum class ImageAspect
    {
        Color,
        Depth
    };

    enum class ColorSpace
    {
        SRGB,
        Linear
    };

    struct ImageCreateParams
    {
        ImageType imageType{ImageType::Image2D};
        ImageUsageFlags usageFlags{};
        NCommon::Size3DUInt size{0, 0, 0};
        ColorSpace colorSpace{ColorSpace::SRGB};
        uint32_t numLayers{1};
        uint32_t numMipLevels{1};
    };

    struct ImageSubresourceRange {
        ImageAspect imageAspect{ImageAspect::Color};
        uint32_t baseMipLevel{0};
        uint32_t levelCount{1};
        uint32_t baseArrayLayer{0};
        uint32_t layerCount{1};
    };

    static constexpr auto OneLevelOneLayerColorImageRange = ImageSubresourceRange{
        .imageAspect = ImageAspect::Color,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };

    struct ImageRegion
    {
        uint32_t layerIndex{0};
        uint32_t mipLevel{0};
        std::array<NCommon::Point3DUInt, 2> offsets{NCommon::Point3DUInt{0,0,0}, NCommon::Point3DUInt{0,0,0}};
    };

    //
    // Buffers
    //

    enum class BufferUsageFlag
    {
        Vertex,
        Index,
        Indirect,
        TransferSrc,
        TransferDst,
        GraphicsUniformRead,
        GraphicsStorageRead,
        ComputeUniformRead,
        ComputeStorageRead,
        ComputeStorageReadWrite
    };

    using BufferUsageFlags = std::unordered_set<BufferUsageFlag>;

    struct BufferCreateParams
    {
        BufferUsageFlags usageFlags{};
        std::size_t byteSize{0};
        bool dedicatedMemory{false};
    };

    enum class TransferBufferUsageFlag
    {
        Upload,
        Download
    };

    using TransferBufferUsageFlags = std::unordered_set<TransferBufferUsageFlag>;

    struct TransferBufferCreateParams
    {
        TransferBufferUsageFlags usageFlags{};
        std::size_t byteSize{0};
        bool sequentiallyWritten{false};
    };

    struct BufferBinding
    {
        BufferId bufferId{};
        std::size_t byteOffset{0};

        auto operator<=>(const BufferBinding&) const = default;
    };

    //
    // Rendering
    //

    enum class LoadOp
    {
        Load,
        Clear,
        DontCare
    };

    enum class StoreOp
    {
        Store,
        DontCare
    };

    struct ColorRenderAttachment
    {
        ImageId imageId;
        uint32_t mipLevel{0};
        uint32_t layer{0};
        LoadOp loadOp{LoadOp::Clear};
        StoreOp storeOp{StoreOp::Store};
        glm::vec4 clearColor{0,0,0,1};
        bool cycle{true};
    };

    struct DepthRenderAttachment
    {
        ImageId imageId;
        uint32_t mipLevel{0};
        uint32_t layer{0};
        LoadOp loadOp{LoadOp::Clear};
        StoreOp storeOp{StoreOp::Store};
        float clearDepth{0.0f};
        bool cycle{true};
    };

    struct ComputeBufferAttachment
    {
        std::string bindPoint;
        BufferId bufferId;
        bool cycle{true};
    };

    enum class CullFace
    {
        None,
        Front,
        Back
    };
}

#endif //WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUCOMMON_H
