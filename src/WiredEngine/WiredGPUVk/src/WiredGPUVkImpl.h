/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_WIREDGPUVKIMPL_H
#define WIREDENGINE_WIREDGPUVK_SRC_WIREDGPUVKIMPL_H

#include "Vulkan/VulkanCommandPool.h"

#include "State/CommandBuffer.h"

#include <Wired/GPU/WiredGPUVk.h>

#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>

namespace NCommon
{
    class ILogger;
}

namespace Wired::GPU
{
    struct Global;
    class Frames;
    class CommandBuffers;
    class Images;
    class Buffers;
    class Shaders;
    class VkSamplers;
    class Layouts;
    class VkPipelines;
    class DescriptorSets;
    class UniformBuffers;
    struct Usages;

    class WiredGPUVkImpl : public WiredGPUVk
    {
        public:

            explicit WiredGPUVkImpl(const NCommon::ILogger* pLogger, WiredGPUVkInput input);
            ~WiredGPUVkImpl() override;

            //
            // GPUVk
            //
            [[nodiscard]] VkInstance GetVkInstance() const override;

            //
            // GPU
            //
            [[nodiscard]] bool Initialize() override;
            void Destroy() override;

            [[nodiscard]] std::optional<std::vector<std::string>> GetSuitablePhysicalDeviceNames() const override;
            void SetRequiredPhysicalDevice(const std::string& physicalDeviceName) override;

            // Runtime startup/events
            [[nodiscard]] bool StartUp(const std::optional<const SurfaceDetails*>& surfaceDetails,
                                       const std::optional<ImGuiGlobals>& imGuiGlobals,
                                       const GPUSettings& gpuSettings) override;
            void ShutDown() override;
            void OnSurfaceDetailsChanged(const SurfaceDetails* pSurfaceDetails) override;
            void OnGPUSettingsChanged(const GPUSettings& gpuSettings) override;

            void RunCleanUp(bool isIdleCleanUp) override;

            // Shaders
            [[nodiscard]] bool CreateShader(const ShaderSpec& shaderSpec) override;
            void DestroyShader(const std::string& shaderName) override;

            // Pipelines
            [[nodiscard]] std::expected<PipelineId, bool> CreateGraphicsPipeline(const GraphicsPipelineParams& params) override;
            [[nodiscard]] std::expected<PipelineId, bool> CreateComputePipeline(const ComputePipelineParams& params) override;
            void DestroyPipeline(PipelineId pipelineId) override;

            // Images
            [[nodiscard]] std::expected<ImageId, bool> CreateImage(CommandBufferId commandBufferId,
                                                                   const ImageCreateParams& params,
                                                                   const std::string& tag) override;
            void DestroyImage(ImageId imageId) override;

            [[nodiscard]] bool GenerateMipMaps(CommandBufferId commandBufferId, ImageId imageId) override;

            [[nodiscard]] NCommon::Size2DUInt GetSwapChainSize() const override;

            // Buffers
            [[nodiscard]] std::expected<BufferId, bool> CreateTransferBuffer(const TransferBufferCreateParams& bufferCreateParams, const std::string& tag) override;
            [[nodiscard]] std::expected<BufferId, bool> CreateBuffer(const BufferCreateParams& bufferCreateParams, const std::string& tag) override;
            [[nodiscard]] std::expected<void*, bool> MapBuffer(BufferId bufferId, bool cycle) override;
            bool UnmapBuffer(BufferId bufferId) override;
            void DestroyBuffer(BufferId bufferId) override;

            // Samplers
            [[nodiscard]] std::expected<SamplerId, bool> CreateSampler(const SamplerInfo& samplerInfo, const std::string& tag) override;
            void DestroySampler(SamplerId samplerId) override;

            // Commands
            [[nodiscard]] std::expected<CommandBufferId, bool> AcquireCommandBuffer(bool primary, const std::string& tag) override;
            std::expected<bool, SurfaceError> SubmitCommandBuffer(CommandBufferId commandBufferId) override;
            void CancelCommandBuffer(CommandBufferId commandBufferId) override;

            [[nodiscard]] std::expected<ImageId, SurfaceError> AcquireSwapChainImage(CommandBufferId commandBufferId) override;

            bool CmdClearColorImage(CopyPass copyPass, ImageId imageId, const ImageSubresourceRange& subresourceRange, const glm::vec4& color, bool cycle) override;
            bool CmdBlitImage(CopyPass copyPass, ImageId sourceImageId, const ImageRegion& sourceRegion, ImageId destImageId, const ImageRegion& destRegion, Filter filter, bool cycle) override;
            bool CmdUploadDataToBuffer(CopyPass copyPass, BufferId sourceTransferBufferId, const std::size_t& sourceByteOffset, BufferId destBufferId, const std::size_t& destByteOffset, const std::size_t& copyByteSize, bool cycle) override;
            bool CmdUploadDataToImage(CopyPass copyPass, BufferId sourceTransferBufferId, const std::size_t& sourceByteOffset, ImageId destImageId, const ImageRegion& destRegion, const std::size_t& copyByteSize, bool cycle) override;
            bool CmdCopyBufferToBuffer(CopyPass copyPass, BufferId sourceBufferId, const std::size_t& sourceByteOffset, BufferId destBufferId, const std::size_t& destByteOffset, const std::size_t& copyByteSize, bool cycle) override;

            bool CmdExecuteCommands(CommandBufferId primaryCommandBufferId, const std::vector<CommandBufferId>& secondaryCommandBufferIds) override;
            bool CmdBindPipeline(RenderOrComputePass pass, PipelineId pipelineId) override;
            bool CmdBindVertexBuffers(RenderPass renderPass, uint32_t firstBinding, const std::vector<BufferBinding>& bindings) override;
            bool CmdBindIndexBuffer(RenderPass renderPass, const BufferBinding& binding, IndexType indexType) override;
            bool CmdDrawIndexed(RenderPass renderPass, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
            bool CmdDrawIndexedIndirect(RenderPass renderPass, BufferId bufferId, const std::size_t& byteOffset, uint32_t drawCount, uint32_t stride) override;
            bool CmdDrawIndexedIndirectCount(RenderPass renderPass, BufferId commandsBufferId, const std::size_t& commandsByteOffset, BufferId countsBufferId, const std::size_t& countByteOffset, uint32_t maxDrawCount, uint32_t stride) override;

            bool CmdDispatch(ComputePass computePass, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;

            #ifdef WIRED_IMGUI
                bool CmdRenderImGuiDrawData(RenderPass renderPass, ImDrawData* pDrawData) override;
                [[nodiscard]] std::optional<ImTextureID> CreateImGuiImageReference(ImageId imageId, SamplerId samplerId) override;
            #endif

            bool CmdBindUniformData(RenderOrComputePass pass, const std::string& bindPoint, const void *pData, const std::size_t& byteSize) override;
            bool CmdBindStorageReadBuffer(RenderOrComputePass pass, const std::string& bindPoint, BufferId bufferId) override;
            bool CmdBindStorageReadWriteBuffer(RenderOrComputePass pass, const std::string& bindPoint, BufferId bufferId) override;
            bool CmdBindImageViewSampler(RenderOrComputePass pass, const std::string& bindPoint, uint32_t arrayIndex, ImageId imageId, SamplerId samplerId) override;
            bool CmdBindStorageReadImage(RenderOrComputePass pass, const std::string& bindPoint, ImageId imageId) override;
            bool CmdBindStorageReadWriteImage(RenderOrComputePass pass, const std::string& bindPoint, ImageId imageId) override;

            void CmdPushDebugSection(CommandBufferId commandBufferId, const std::string& sectionName) override;
            void CmdPopDebugSection(CommandBufferId commandBufferId) override;

            // Timestamps
            [[nodiscard]] bool HasTimestampSupport() const override;
            void SyncDownFrameTimestamps() override;
            void ResetFrameTimestampsForRecording(CommandBufferId commandBufferId) override;
            void CmdWriteTimestampStart(CommandBufferId commandBufferId, const std::string& name) override;
            void CmdWriteTimestampFinish(CommandBufferId commandBufferId, const std::string& name) override;
            [[nodiscard]] std::optional<float> GetTimestampDiffMs(const std::string& name, uint32_t offset) const override;

            // Rendering
            void StartFrame() override;
            void EndFrame() override;

            [[nodiscard]] std::expected<CopyPass, bool> BeginCopyPass(CommandBufferId commandBufferId, const std::string& tag) override;
            bool EndCopyPass(CopyPass copyPass) override;

            [[nodiscard]] std::expected<RenderPass, bool> BeginRenderPass(
                CommandBufferId commandBufferId,
                const std::vector<ColorRenderAttachment>& colorAttachments,
                const std::optional<DepthRenderAttachment>& depthAttachment,
                const NCommon::Point2DUInt& renderOffset,
                const NCommon::Size2DUInt& renderExtent,
                const std::string& tag) override;
            bool EndRenderPass(RenderPass renderPass) override;

            [[nodiscard]] std::expected<ComputePass, bool> BeginComputePass(CommandBufferId commandBufferId, const std::string& tag) override;
            bool EndComputePass(ComputePass computePass) override;

        private:

            [[nodiscard]] bool CreateVkInstance();
            void DestroyVkInstance();

            [[nodiscard]] bool InitImGui();
            void DestroyImGui();

            void RecreateSwapChain();

            [[nodiscard]] std::expected<VulkanCommandPool*, bool> EnsureThreadCommandPool();
            [[nodiscard]] std::expected<DescriptorSets*, bool> EnsureThreadDescriptorSets();

            void BarrierGraphicsSetResourcesForUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings);
            void BarrierGraphicsSetResourcesToDefaultUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings);

            void BarrierComputeSetResourcesForUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings);
            void BarrierComputeSetResourcesToDefaultUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings);

            void BindDescriptorSetsNeedingRefresh(CommandBuffer* pCommandBuffer, PassState& passState);

            [[nodiscard]] std::expected<bool, SurfaceError> PresentSwapChainImage(uint32_t swapChainImageIndex, VkSemaphore waitSemaphore);

        private:

            std::unique_ptr<Global> m_global;
            WiredGPUVkInput m_input;

            std::unique_ptr<Frames> m_frames;
            std::unique_ptr<CommandBuffers> m_commandBuffers;
            std::unique_ptr<Images> m_images;
            std::unique_ptr<Buffers> m_buffers;
            std::unique_ptr<Shaders> m_shaders;
            std::unique_ptr<VkSamplers> m_samplers;
            std::unique_ptr<Layouts> m_layouts;
            std::unique_ptr<VkPipelines> m_pipelines;
            std::unique_ptr<UniformBuffers> m_uniformBuffers;
            std::unique_ptr<Usages> m_usages;

            // Thread id -> commandsQueue command pool
            std::unordered_map<std::thread::id, std::unique_ptr<VulkanCommandPool>> m_commandPools;
            std::mutex m_commandPoolsMutex;

            // Thread id -> DescriptorSets
            std::unordered_map<std::thread::id, std::unique_ptr<DescriptorSets>> m_descriptorSets;
            std::mutex m_descriptorSetsMutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_WIREDGPUVKIMPL_H
