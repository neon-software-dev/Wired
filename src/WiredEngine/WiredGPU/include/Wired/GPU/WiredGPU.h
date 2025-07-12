/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_WIREDGPU_H
#define WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_WIREDGPU_H

#include "SurfaceDetails.h"
#include "GPUSettings.h"
#include "GPUId.h"
#include "GPUCommon.h"
#include "GPUSamplerCommon.h"
#include "GraphicsPipelineParams.h"
#include "ComputePipelineParams.h"
#include "ImGuiGlobals.h"

#include <NEON/Common/Space/Size2D.h>
#include <NEON/Common/Space/Point2D.h>

#include <glm/glm.hpp>

#ifdef WIRED_IMGUI
    #include <imgui.h>
#endif

#include <optional>
#include <string>
#include <expected>
#include <vector>

namespace Wired::GPU
{
    class WiredGPU
    {
        public:

            virtual ~WiredGPU() = default;

            //
            // Initialization
            //
            virtual bool Initialize() = 0;
            virtual void Destroy() = 0;

            //
            // Valid after initialize
            //
            [[nodiscard]] virtual std::optional<std::vector<std::string>> GetSuitablePhysicalDeviceNames() const = 0;
            virtual void SetRequiredPhysicalDevice(const std::string& physicalDeviceName) = 0;

            //
            // Runtime startup/events
            //
            [[nodiscard]] virtual bool StartUp(const std::optional<const SurfaceDetails*>& pSurfaceDetails,
                                               const std::optional<ImGuiGlobals>& imGuiGlobals,
                                               const GPUSettings& gpuSettings) = 0;
            virtual void ShutDown() = 0;

            virtual void OnSurfaceDetailsChanged(const SurfaceDetails* pSurfaceDetails) = 0;
            virtual void OnGPUSettingsChanged(const GPUSettings& gpuSettings) = 0;

            virtual void RunCleanUp(bool isIdleCleanUp) = 0;

            //
            // Shaders
            //
            [[nodiscard]] virtual bool CreateShader(const ShaderSpec& shaderSpec) = 0;
            virtual void DestroyShader(const std::string& shaderName) = 0;

            //
            // Pipelines
            //
            [[nodiscard]] virtual std::expected<PipelineId, bool> CreateGraphicsPipeline(const GraphicsPipelineParams& params) = 0;
            [[nodiscard]] virtual std::expected<PipelineId, bool> CreateComputePipeline(const ComputePipelineParams& params) = 0;
            virtual void DestroyPipeline(PipelineId pipelineId) = 0;

            //
            // Images
            //
            [[nodiscard]] virtual std::expected<ImageId, bool> CreateImage(CommandBufferId commandBufferId,
                                                                           const ImageCreateParams& params,
                                                                           const std::string& tag) = 0;
            virtual void DestroyImage(ImageId imageId) = 0;

            [[nodiscard]] virtual bool GenerateMipMaps(CommandBufferId commandBufferId, ImageId imageId) = 0;

            [[nodiscard]] virtual NCommon::Size2DUInt GetSwapChainSize() const = 0;

            //
            // Buffers
            //
            [[nodiscard]] virtual std::expected<BufferId, bool> CreateTransferBuffer(const TransferBufferCreateParams& bufferCreateParams, const std::string& tag) = 0;
            [[nodiscard]] virtual std::expected<BufferId, bool> CreateBuffer(const BufferCreateParams& bufferCreateParams, const std::string& tag) = 0;
            [[nodiscard]] virtual std::expected<void*, bool> MapBuffer(BufferId bufferId, bool cycle) = 0;
            virtual bool UnmapBuffer(BufferId bufferId) = 0;
            virtual void DestroyBuffer(BufferId bufferId) = 0;

            //
            // Samplers
            //
            [[nodiscard]] virtual std::expected<SamplerId, bool> CreateSampler(const SamplerInfo& samplerInfo, const std::string& tag) = 0;
            virtual void DestroySampler(SamplerId samplerId) = 0;

            //
            // Commands
            //
            [[nodiscard]] virtual std::expected<CommandBufferId, bool> AcquireCommandBuffer(bool primary, const std::string& tag) = 0;
            virtual std::expected<bool, SurfaceError> SubmitCommandBuffer(CommandBufferId commandBufferId) = 0;
            virtual void CancelCommandBuffer(CommandBufferId commandBufferId) = 0;

            [[nodiscard]] virtual std::expected<ImageId, SurfaceError> AcquireSwapChainImage(CommandBufferId commandBufferId) = 0;

            virtual bool CmdClearColorImage(CopyPass copyPass, ImageId imageId, const ImageSubresourceRange& subresourceRange, const glm::vec4& color, bool cycle) = 0;
            virtual bool CmdBlitImage(CopyPass copyPass, ImageId sourceImage, const ImageRegion& sourceRegion, ImageId destImage, const ImageRegion& destRegion, Filter filter, bool cycle) = 0;
            virtual bool CmdUploadDataToBuffer(CopyPass copyPass, BufferId sourceTransferBufferId, const std::size_t& sourceByteOffset, BufferId destBufferId, const std::size_t& destByteOffset, const std::size_t& copyByteSize, bool cycle) = 0;
            virtual bool CmdUploadDataToImage(CopyPass copyPass, BufferId sourceTransferBufferId, const std::size_t& sourceByteOffset, ImageId destImageId, const ImageRegion& destRegion, const std::size_t& copyByteSize, bool cycle) = 0;
            virtual bool CmdCopyBufferToBuffer(CopyPass copyPass, BufferId sourceBufferId, const std::size_t& sourceByteOffset, BufferId destBufferId, const std::size_t& destByteOffset, const std::size_t& copyByteSize, bool cycle) = 0;

            virtual bool CmdExecuteCommands(CommandBufferId primaryCommandBufferId, const std::vector<CommandBufferId>& secondaryCommandBufferIds) = 0;
            virtual bool CmdBindPipeline(RenderOrComputePass pass, PipelineId pipelineId) = 0;
            virtual bool CmdBindVertexBuffers(RenderPass renderPass, uint32_t firstBinding, const std::vector<BufferBinding>& bindings) = 0;
            virtual bool CmdBindIndexBuffer(RenderPass renderPass, const BufferBinding& binding, IndexType indexType) = 0;
            virtual bool CmdDrawIndexed(RenderPass renderPass, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
            virtual bool CmdDrawIndexedIndirect(RenderPass renderPass, BufferId bufferId, const std::size_t& byteOffset, uint32_t drawCount, uint32_t stride) = 0;
            virtual bool CmdDrawIndexedIndirectCount(RenderPass renderPass, BufferId commandsBufferId, const std::size_t& commandsByteOffset, BufferId countsBufferId, const std::size_t& countByteOffset, uint32_t maxDrawCount, uint32_t stride) = 0;

            virtual bool CmdDispatch(ComputePass computePass, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

            #ifdef WIRED_IMGUI
                virtual bool CmdRenderImGuiDrawData(RenderPass renderPass, ImDrawData* pDrawData) = 0;
                [[nodiscard]] virtual std::optional<ImTextureID> CreateImGuiImageReference(ImageId imageId, SamplerId samplerId) = 0;
            #endif

            virtual bool CmdBindUniformData(RenderOrComputePass pass, const std::string& bindPoint, const void *pData, const std::size_t& byteSize) = 0;
            virtual bool CmdBindStorageReadBuffer(RenderOrComputePass pass, const std::string& bindPoint, BufferId bufferId) = 0;
            virtual bool CmdBindStorageReadWriteBuffer(RenderOrComputePass pass, const std::string& bindPoint, BufferId bufferId) = 0;
            virtual bool CmdBindImageViewSampler(RenderOrComputePass pass, const std::string& bindPoint, uint32_t arrayIndex, ImageId imageId, SamplerId samplerId) = 0;
            virtual bool CmdBindStorageReadImage(RenderOrComputePass pass, const std::string& bindPoint, ImageId imageId) = 0;
            virtual bool CmdBindStorageReadWriteImage(RenderOrComputePass pass, const std::string& bindPoint, ImageId imageId) = 0;

            virtual void CmdPushDebugSection(CommandBufferId commandBufferId, const std::string& sectionName) = 0;
            virtual void CmdPopDebugSection(CommandBufferId commandBufferId) = 0;

            //
            // Timestamps
            //
            [[nodiscard]] virtual bool HasTimestampSupport() const = 0;
            virtual void SyncDownFrameTimestamps() = 0;
            virtual void ResetFrameTimestampsForRecording(CommandBufferId commandBufferId) = 0;
            virtual void CmdWriteTimestampStart(CommandBufferId commandBufferId, const std::string& name) = 0;
            virtual void CmdWriteTimestampFinish(CommandBufferId commandBufferId, const std::string& name) = 0;
            [[nodiscard]] virtual std::optional<float> GetTimestampDiffMs(const std::string& name, uint32_t offset) const = 0;

            //
            // Rendering
            //
            virtual void StartFrame() = 0;
            virtual void EndFrame() = 0;

            [[nodiscard]] virtual std::expected<CopyPass, bool> BeginCopyPass(CommandBufferId commandBufferId, const std::string& tag) = 0;
            virtual bool EndCopyPass(CopyPass copyPass) = 0;

            [[nodiscard]] virtual std::expected<RenderPass, bool> BeginRenderPass(
                CommandBufferId commandBufferId,
                const std::vector<ColorRenderAttachment>& colorAttachments,
                const std::optional<DepthRenderAttachment>& depthAttachment,
                const NCommon::Point2DUInt& renderOffset,
                const NCommon::Size2DUInt& renderExtent,
                const std::string& tag) = 0;
            virtual bool EndRenderPass(RenderPass renderPass) = 0;

            [[nodiscard]] virtual std::expected<ComputePass, bool> BeginComputePass(CommandBufferId commandBufferId, const std::string& tag) = 0;
            virtual bool EndComputePass(ComputePass computePass) = 0;
    };
}

#endif //WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_WIREDGPU_H
