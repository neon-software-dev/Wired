/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WiredGPUVkImpl.h"
#include "Global.h"
#include "VulkanCallsUtil.h"
#include "Common.h"
#include "Usages.h"

#include "Frame/Frames.h"
#include "State/CommandBuffers.h"
#include "Image/Images.h"
#include "Buffer/Buffers.h"
#include "Shader/Shaders.h"
#include "Sampler/VkSamplers.h"
#include "Pipeline/Layouts.h"
#include "Pipeline/VkPipelines.h"
#include "Descriptor/DescriptorSets.h"
#include "Buffer/UniformBuffers.h"
#include "Pipeline/VkPipelineConfig.h"
#include "Util/VMAUtil.h"
#include "Util/SpaceUtil.h"
#include "Util/RenderPassAttachment.h"

#include "Vulkan/VulkanDebugUtil.h"

#include <Wired/GPU/VulkanSurfaceDetails.h>

#include <NEON/Common/Log/ILogger.h>

#ifdef WIRED_IMGUI
    #include <imgui_impl_vulkan.h>
#endif

#include <algorithm>

namespace Wired::GPU
{

WiredGPUVkImpl::WiredGPUVkImpl(const NCommon::ILogger* pLogger, WiredGPUVkInput input)
    : m_global(std::make_unique<Global>())
    , m_input(std::move(input))
    , m_frames(std::make_unique<Frames>(m_global.get()))
    , m_commandBuffers(std::make_unique<CommandBuffers>(m_global.get()))
    , m_images(std::make_unique<Images>(m_global.get()))
    , m_buffers(std::make_unique<Buffers>(m_global.get()))
    , m_shaders(std::make_unique<Shaders>(m_global.get()))
    , m_samplers(std::make_unique<VkSamplers>(m_global.get()))
    , m_layouts(std::make_unique<Layouts>(m_global.get()))
    , m_pipelines(std::make_unique<VkPipelines>(m_global.get()))
    , m_uniformBuffers(std::make_unique<UniformBuffers>(m_global.get()))
    , m_usages(std::make_unique<Usages>())
{
    m_global->pLogger = pLogger;
    m_global->pCommandBuffers = m_commandBuffers.get();
    m_global->pImages = m_images.get();
    m_global->pBuffers = m_buffers.get();
    m_global->pShaders = m_shaders.get();
    m_global->pSamplers = m_samplers.get();
    m_global->pLayouts = m_layouts.get();
    m_global->pPipelines = m_pipelines.get();
    m_global->pUniformBuffers = m_uniformBuffers.get();
    m_global->pUsages = m_usages.get();
}

WiredGPUVkImpl::~WiredGPUVkImpl() = default;

bool WiredGPUVkImpl::Initialize()
{
    m_global->pLogger->Info("WiredGPUVkImpl: Initializing");

    if (!CreateVkInstance())
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::Initialize: Failed to create VkInstance");
        return false;
    }

    return true;
}

bool WiredGPUVkImpl::CreateVkInstance()
{
    m_global->pLogger->Info("WiredGPUVkImpl: Creating VkInstance");

    //
    // Resolve global vulkan calls so we can call funcs to create a vkInstance
    //
    m_global->vk.vkGetInstanceProcAddr = m_input.pfnVkGetInstanceProcAddr;
    if (!ResolveGlobalCalls(m_global->vk))
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::CreateVkInstance: Failed to resolve global vulkan calls");
        return false;
    }

    //
    // Create a vkInstance
    //
    const auto instance = VulkanInstance::Create(m_global.get(),
                                                 m_input.applicationName,
                                                 m_input.applicationVersion,
                                                 m_input.requiredInstanceExtensions,
                                                 m_input.supportSurfaceOutput);
    if (!instance)
    {
        std::string errorDetails;
        switch (instance.error())
        {
            case GPU::InstanceCreateError::VulkanGlobalFuncsMissing: errorDetails = "Failed to retrieve global vulkan functions"; break;
            case GPU::InstanceCreateError::InvalidVulkanInstanceVersion: errorDetails = "Unsupported vulkan version"; break;
            case GPU::InstanceCreateError::MissingRequiredInstanceExtension: errorDetails = "Missing required vulkan instance extensions"; break;
            case GPU::InstanceCreateError::CreateInstanceFailed: errorDetails = "Call to vkCreateInstance() failed"; break;
            case GPU::InstanceCreateError::VulkanInstanceFuncsMissing: errorDetails = "Failed to retrieve instance vulkan functions"; break;
        }

        m_global->pLogger->Fatal("WiredGPUVkImpl::CreateVkInstance: Failed to create Vulkan instance, detailed: {}", errorDetails);
        return false;
    }
    m_global->instance = *instance;

    MarkDebugExtensionAvailable(m_global->instance.IsInstanceExtensionEnabled(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));

    return true;
}

void WiredGPUVkImpl::Destroy()
{
    m_global->pLogger->Info("WiredGPUVkImpl: Destroying");

    DestroyVkInstance();
}

void WiredGPUVkImpl::DestroyVkInstance()
{
    m_global->pLogger->Info("WiredGPUVkImpl: Destroying VkInstance");

    MarkDebugExtensionAvailable(false);

    m_global->instance.Destroy();
    m_global->instance = {};
}

std::optional<std::vector<std::string>> WiredGPUVkImpl::GetSuitablePhysicalDeviceNames() const
{
    std::vector<std::string> deviceNames;

    std::ranges::transform(
        VulkanPhysicalDevice::GetSuitablePhysicalDevices(m_global.get(), m_global->instance, m_global->surface),
        std::back_inserter(deviceNames), [](const VulkanPhysicalDevice& physicalDevice){
            return std::string(physicalDevice.GetPhysicalDeviceProperties().properties.deviceName);
        }
    );

    return deviceNames;
}

void WiredGPUVkImpl::SetRequiredPhysicalDevice(const std::string& physicalDeviceName)
{
    m_global->requiredPhysicalDeviceName = physicalDeviceName;
}

VkInstance WiredGPUVkImpl::GetVkInstance() const
{
    return m_global->instance.GetVkInstance();
}

bool WiredGPUVkImpl::StartUp(const std::optional<const SurfaceDetails*>& surfaceDetails,
                             const std::optional<ImGuiGlobals>& imGuiGlobals,
                             const GPUSettings& gpuSettings)
{
    m_global->pLogger->Info("WiredGPUVkImpl: Starting Up");

    if (surfaceDetails)
    {
        const auto vulkanSurfaceDetails = dynamic_cast<const VulkanSurfaceDetails*>(*surfaceDetails);
        m_global->surface = VulkanSurface(m_global.get(), vulkanSurfaceDetails->vkSurface, vulkanSurfaceDetails->pixelSize);
    }

    m_global->gpuSettings = gpuSettings;

    //
    // Choose a physical device to use
    //
    auto physicalDevice = VulkanPhysicalDevice::ChoosePhysicalDevice(m_global.get());
    if (!physicalDevice)
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::StartUp: No suitable physical device found");
        return false;
    }
    m_global->physicalDevice = *physicalDevice;

    //
    // Create a logical device from the physical device
    //
    auto deviceResult = VulkanDevice::Create(m_global.get());
    if (!physicalDevice)
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::StartUp: No suitable physical device found");
        return false;
    }
    m_global->device = VulkanDevice(m_global.get(), deviceResult->vkDevice);

    m_global->commandQueue = VulkanQueue::CreateFrom(m_global.get(), deviceResult->vkCommandQueue, deviceResult->commandQueueFamilyIndex, "Commands");
    if (deviceResult->vkPresentQueue)
    {
        m_global->presentQueue = VulkanQueue::CreateFrom(m_global.get(), *deviceResult->vkPresentQueue, *deviceResult->presentQueueFamilyIndex, "Present");
    }

    //
    // Create a swap chain if we have a surface to present to
    //
    if (m_global->surface)
    {
        const auto swapChain = VulkanSwapChain::Create(m_global.get());
        if (!swapChain)
        {
            m_global->pLogger->Fatal("WiredGPUVkImpl::StartUp: Failed to create a vulkan swap chain");
            return false;
        }
        m_global->swapChain = *swapChain;
    }

    //
    // Initialize VMA
    //
    const VmaVulkanFunctions vmaFunctions = GatherVMAFunctions(m_global->vk);

    VmaAllocatorCreateInfo vmaCreateInfo{};
    vmaCreateInfo.vulkanApiVersion = REQUIRED_VULKAN_DEVICE_VERSION;
    vmaCreateInfo.instance = m_global->instance.GetVkInstance();
    vmaCreateInfo.physicalDevice = m_global->physicalDevice.GetVkPhysicalDevice();
    vmaCreateInfo.device = m_global->device.GetVkDevice();
    vmaCreateInfo.pVulkanFunctions = &vmaFunctions;
    vmaCreateInfo.flags = 0;

    VmaAllocator vmaAllocator{VK_NULL_HANDLE};

    auto result = vmaCreateAllocator(&vmaCreateInfo, &vmaAllocator);
    if (result != VK_SUCCESS)
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::StartUp: Failed to initialize VMA, result code: {}", (uint32_t)result);
        return false;
    }
    m_global->vma = vmaAllocator;

    //
    // Initialize ImGui
    //
    if (imGuiGlobals)
    {
        // Make the ImGui context current
        #ifdef WIRED_IMGUI
            ImGui::SetCurrentContext(imGuiGlobals->pImGuiContext);
            ImGui::SetAllocatorFunctions(imGuiGlobals->pImGuiMemAllocFunc, imGuiGlobals->pImGuiMemFreeFunc, nullptr);
        #endif

        if (!InitImGui())
        {
            m_global->pLogger->Fatal("WiredGPUVkImpl::StartUp: Failed to initialize ImGui");
            return false;
        }

        m_global->imGuiActive = true;
    }

    //
    // Initialize frames
    //
    if (!m_frames->Create())
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::StartUp: Failed to create frames");
        return false;
    }

    //
    // Initialize uniform buffers
    //
    if (!m_uniformBuffers->Create())
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::StartUp: Failed to create uniform buffers");
        return false;
    }

    return true;
}

void WiredGPUVkImpl::ShutDown()
{
    m_global->pLogger->Info("WiredGPUVkImpl: Shutting Down");

    // Let all in-progress work finish before destroying anything
    m_global->vk.vkDeviceWaitIdle(m_global->device.GetVkDevice());

    //
    // Destroy run-time resources
    //
    if (m_global->imGuiActive)
    {
        DestroyImGui();
    }

    m_uniformBuffers->Destroy();
    m_pipelines->Destroy();
    m_layouts->Destroy();
    m_samplers->Destroy();
    m_shaders->Destroy();
    m_buffers->Destroy();
    m_images->Destroy();
    m_commandBuffers->Destroy();

    for (auto& descriptorSet : m_descriptorSets)
    {
        descriptorSet.second->Destroy();
    }
    m_descriptorSets.clear();

    for (auto& commandPool : m_commandPools)
    {
        commandPool.second->Destroy();
    }
    m_commandPools.clear();

    m_usages->Reset();

    //
    // Destroy StartUp framework
    //
    m_frames->Destroy();

    if (m_global->vma != VK_NULL_HANDLE)
    {
        vmaDestroyAllocator(m_global->vma);
        m_global->vma = VK_NULL_HANDLE;
    }

    if (m_global->swapChain)
    {
        m_global->swapChain->Destroy(true);
        m_global->swapChain = std::nullopt;
    }

    if (m_global->presentQueue) { m_global->presentQueue->Destroy(); }
    m_global->presentQueue = std::nullopt;
    m_global->commandQueue.Destroy();
    m_global->commandQueue = {};

    // Before destroying the device, let all the destruction work we just scheduled above finish
    m_global->vk.vkDeviceWaitIdle(m_global->device.GetVkDevice());

    m_global->device.Destroy();
    m_global->device = {};
    m_global->physicalDevice = {};
    m_global->surface = std::nullopt;

    m_global->ids.Reset();
}

bool WiredGPUVkImpl::InitImGui()
{
    #ifdef WIRED_IMGUI
        m_global->pLogger->Info("WiredGPUVkImpl: Initializing ImGui");

        // ImGui can only be rendered onto the swap chain
        const VkFormat colorAttachmentFormat =  m_global->swapChain->GetSwapChainConfig().surfaceFormat.format;

        VkPipelineRenderingCreateInfo vkPipelineRenderingCreateInfo{};
        vkPipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        vkPipelineRenderingCreateInfo.colorAttachmentCount = 1;
        vkPipelineRenderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.ApiVersion = REQUIRED_VULKAN_INSTANCE_VERSION;
        initInfo.Instance = m_global->instance.GetVkInstance();
        initInfo.PhysicalDevice = m_global->physicalDevice.GetVkPhysicalDevice();
        initInfo.Device = m_global->device.GetVkDevice();
        initInfo.QueueFamily = m_global->commandQueue.GetQueueFamilyIndex();
        initInfo.Queue = m_global->commandQueue.GetVkQueue();
        // Surely we're not going to use ImGui to display more than 50 images at once, right? Bump this up when it asserts, or make it configurable.
        initInfo.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE + 50;
        initInfo.UseDynamicRendering = true;
        initInfo.PipelineRenderingCreateInfo = vkPipelineRenderingCreateInfo;
        initInfo.Subpass = 0;
        initInfo.MinImageCount = std::max(m_global->gpuSettings.framesInFlight, 2U); // ImGui asserts at least a min of 2
        initInfo.ImageCount = (uint32_t)m_global->swapChain->GetImageCount();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.MinAllocationSize = 1024 * 1024; // Silences a validation layer warning about overly small allocations
        return ImGui_ImplVulkan_Init(&initInfo);
    #else
        return true;
    #endif
}

void WiredGPUVkImpl::DestroyImGui()
{
    #ifdef WIRED_IMGUI
        m_global->pLogger->Info("WiredGPUVkImpl: Destroying ImGui");

        ImGui_ImplVulkan_Shutdown();
    #endif
}

void WiredGPUVkImpl::RecreateSwapChain()
{
    //
    // Create a new swap chain
    //
    m_global->pLogger->Info("WiredGPUVkImpl: Recreating swap chain");

    const auto swapChain = VulkanSwapChain::Create(m_global.get());
    if (!swapChain)
    {
        m_global->pLogger->Fatal("WiredGPUVkImpl::RecreateSwapChain: Failed to create vulkan swap chain");
        return;
    }

    //
    // If a previous swap chain existed, destroy it now
    //
    if (m_global->swapChain)
    {
        m_global->swapChain->Destroy(false);
    }

    //
    // Swap us over to using the new swapchain
    //
    m_global->swapChain = *swapChain;
}

void WiredGPUVkImpl::OnSurfaceDetailsChanged(const SurfaceDetails* pSurfaceDetails)
{
    m_global->pLogger->Info("WiredGPUVkImpl: Received new surface details");

    //
    // Wait for all operations to complete as we're going to tear down resources
    //
    m_global->vk.vkDeviceWaitIdle(m_global->device.GetVkDevice());

    //
    // Update our surface with the latest surface data
    //
    const auto vulkanSurfaceDetails = dynamic_cast<const VulkanSurfaceDetails*>(pSurfaceDetails);

    m_global->surface = VulkanSurface(m_global.get(), vulkanSurfaceDetails->vkSurface, vulkanSurfaceDetails->pixelSize);

    //
    // Recreate the swap chain to match the new surface
    //
    RecreateSwapChain();
}

void WiredGPUVkImpl::OnGPUSettingsChanged(const GPUSettings& gpuSettings)
{
    m_global->pLogger->Info("WiredGPUVkImpl: Received new GPU settings");

    //
    // Wait for all operations to complete
    //
    m_global->vk.vkDeviceWaitIdle(m_global->device.GetVkDevice());

    //
    // Update settings
    //
    const auto presentModeChanged = m_global->gpuSettings.presentMode != gpuSettings.presentMode;

    m_global->gpuSettings = gpuSettings;

    //
    // Handle an updated FIF count. Note that Frames takes care of no-op if the FIF hasn't changed
    //
    m_frames->OnRenderSettingsChanged();

    //
    // Handle an updated present mode
    //
    if (presentModeChanged)
    {
        RecreateSwapChain();
    }

    // TODO: Handle a change in samplerAnisotropy
}

void WiredGPUVkImpl::StartFrame()
{
    RunCleanUp(false);

    m_frames->StartFrame();
}

void WiredGPUVkImpl::EndFrame()
{
    m_frames->EndFrame();
}

std::expected<CopyPass, bool> WiredGPUVkImpl::BeginCopyPass(CommandBufferId commandBufferId, const std::string& tag)
{
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::BeginCopyPass: No such command buffer exists: {}", commandBufferId.id);
        return std::unexpected(false);
    }

    if (!(*commandBuffer)->BeginCopyPass())
    {
        return std::unexpected(false);
    }

    BeginCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer(), std::format("CopyPass-{}", tag));

    return CopyPass{.commandBufferId = commandBufferId};
}

bool WiredGPUVkImpl::EndCopyPass(CopyPass copyPass)
{
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(copyPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::EndCopyPass: No such command buffer exists: {}", copyPass.commandBufferId.id);
        return false;
    }

    // Finish command buffer section for the copy pass
    EndCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer());

    return (*commandBuffer)->EndCopyPass();
}

std::expected<RenderPassAttachment, bool> GetColorRenderPassAttachment(Global* pGlobal,
                                                                       CommandBuffer* pCommandBuffer,
                                                                       const ColorRenderAttachment& colorRenderAttachment)
{
    RenderPassAttachment renderPassAttachment{};
    renderPassAttachment.type = RenderPassAttachment::Type::Color;

    const auto gpuImage = pGlobal->pImages->GetImage(colorRenderAttachment.imageId, colorRenderAttachment.cycle, pCommandBuffer);
    if (!gpuImage)
    {
        pGlobal->pLogger->Error("GetColorRenderPassAttachment: No such image exists or failed to cycle: {}", colorRenderAttachment.imageId.id);
        return std::unexpected(false);
    }

    renderPassAttachment.gpuImage = *gpuImage;

    VkAttachmentLoadOp vkLoadOp{};
    switch (colorRenderAttachment.loadOp)
    {
        case LoadOp::Load: vkLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD; break;
        case LoadOp::Clear: vkLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; break;
        case LoadOp::DontCare: vkLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; break;
    }

    VkAttachmentStoreOp vkStoreOp{};
    switch (colorRenderAttachment.storeOp)
    {
        case StoreOp::Store: vkStoreOp = VK_ATTACHMENT_STORE_OP_STORE; break;
        case StoreOp::DontCare: vkStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; break;
    }

    const auto clearColor = colorRenderAttachment.clearColor;

    uint32_t imageViewIndex = 0;
    if (gpuImage->imageData.imageDef.numLayers > 1)
    {
        // If the image is multi-layered, then the first ImageView wraps the entire image and
        // every subsequent ImageView targets a specific layer.
        imageViewIndex = colorRenderAttachment.layer + 1;
    }

    renderPassAttachment.vkRenderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    renderPassAttachment.vkRenderingAttachmentInfo.imageView = gpuImage->imageViewDatas.at(imageViewIndex).vkImageView;
    renderPassAttachment.vkRenderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    renderPassAttachment.vkRenderingAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
    renderPassAttachment.vkRenderingAttachmentInfo.resolveImageView = VK_NULL_HANDLE;
    renderPassAttachment.vkRenderingAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderPassAttachment.vkRenderingAttachmentInfo.loadOp = vkLoadOp;
    renderPassAttachment.vkRenderingAttachmentInfo.storeOp = vkStoreOp;
    renderPassAttachment.vkRenderingAttachmentInfo.clearValue = VkClearValue { .color = {.float32 = { clearColor.r, clearColor.g, clearColor.b, clearColor.a }}};

    renderPassAttachment.vkImageSubresourceRange = VkImageSubresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = colorRenderAttachment.mipLevel,
        .levelCount = 1,
        .baseArrayLayer = colorRenderAttachment.layer,
        .layerCount = 1
    };

    return renderPassAttachment;
}

std::expected<RenderPassAttachment, bool> GetDepthRenderPassAttachment(Global* pGlobal,
                                                                       CommandBuffer* pCommandBuffer,
                                                                       const DepthRenderAttachment& depthRenderAttachment)
{
    RenderPassAttachment renderPassAttachment{};
    renderPassAttachment.type = RenderPassAttachment::Type::Depth;

    const auto gpuImage = pGlobal->pImages->GetImage(depthRenderAttachment.imageId, depthRenderAttachment.cycle, pCommandBuffer);
    if (!gpuImage)
    {
        pGlobal->pLogger->Error("GetDepthRenderPassAttachment: No such image exists or failed to cycle: {}", depthRenderAttachment.imageId.id);
        return std::unexpected(false);
    }

    renderPassAttachment.gpuImage = *gpuImage;

    VkAttachmentLoadOp vkLoadOp{};
    switch (depthRenderAttachment.loadOp)
    {
        case LoadOp::Load: vkLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD; break;
        case LoadOp::Clear: vkLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; break;
        case LoadOp::DontCare: vkLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; break;
    }

    VkAttachmentStoreOp vkStoreOp{};
    switch (depthRenderAttachment.storeOp)
    {
        case StoreOp::Store: vkStoreOp = VK_ATTACHMENT_STORE_OP_STORE; break;
        case StoreOp::DontCare: vkStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; break;
    }

    uint32_t imageViewIndex = 0;
    if (gpuImage->imageData.imageDef.numLayers > 1)
    {
        // If the image is multi-layered, then the first ImageView wraps the entire image and
        // every subsequent ImageView targets a specific layer.
        imageViewIndex = depthRenderAttachment.layer + 1;
    }

    renderPassAttachment.vkRenderingAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    renderPassAttachment.vkRenderingAttachmentInfo.imageView = gpuImage->imageViewDatas.at(imageViewIndex).vkImageView;
    renderPassAttachment.vkRenderingAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    renderPassAttachment.vkRenderingAttachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
    renderPassAttachment.vkRenderingAttachmentInfo.resolveImageView = VK_NULL_HANDLE;
    renderPassAttachment.vkRenderingAttachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderPassAttachment.vkRenderingAttachmentInfo.loadOp = vkLoadOp;
    renderPassAttachment.vkRenderingAttachmentInfo.storeOp = vkStoreOp;
    renderPassAttachment.vkRenderingAttachmentInfo.clearValue = VkClearValue {
        .depthStencil = {
            .depth = depthRenderAttachment.clearDepth,
            .stencil = 0
        }
    };

    renderPassAttachment.vkImageSubresourceRange = VkImageSubresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
        .baseMipLevel = depthRenderAttachment.mipLevel,
        .levelCount = 1,
        .baseArrayLayer = depthRenderAttachment.layer,
        .layerCount = 1
    };

    return renderPassAttachment;
}

std::expected<RenderPass, bool> WiredGPUVkImpl::BeginRenderPass(CommandBufferId commandBufferId,
                                                                const std::vector<ColorRenderAttachment>& colorAttachments,
                                                                const std::optional<DepthRenderAttachment>& depthAttachment,
                                                                const NCommon::Point2DUInt& renderOffset,
                                                                const NCommon::Size2DUInt& renderExtent,
                                                                const std::string& tag)
{
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::BeginRenderPass: No such command buffer exists: {}", commandBufferId.id);
        return std::unexpected(false);
    }

    if (colorAttachments.empty() && !depthAttachment)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::BeginRenderPass: Need to provide at least one attachment");
        return std::unexpected(false);
    }

    if (!(*commandBuffer)->BeginRenderPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::BeginRenderPass: Command buffer failed to start render pass: {}", commandBufferId.id);
        return std::unexpected(false);
    }

    // Start a command buffer section for the render pass. (Finished in EndRenderPass)
    BeginCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer(), std::format("RenderPass-{}", tag));

    //
    // ColorRenderAttachment -> RenderPassAttachment
    //
    std::vector<RenderPassAttachment> colorRenderPassAttachments;

    for (const auto& colorAttachment : colorAttachments)
    {
        const auto colorRenderPassAttachment = GetColorRenderPassAttachment(m_global.get(), *commandBuffer, colorAttachment);
        if (!colorRenderPassAttachment) { return std::unexpected(false); }
        colorRenderPassAttachments.push_back(*colorRenderPassAttachment);
    }

    //
    // DepthRenderAttachment -> RenderPassAttachment
    //
    std::optional<RenderPassAttachment> depthRenderPassAttachment{};

    if (depthAttachment)
    {
        const auto result = GetDepthRenderPassAttachment(m_global.get(), *commandBuffer, *depthAttachment);
        if (!result) { return std::unexpected(false); }
        depthRenderPassAttachment = *result;
    }

    //
    // Barrier attachments to attachment usage (barriered back to default in EndRenderPass)
    //
    for (const auto& colorRenderPassAttachment : colorRenderPassAttachments)
    {
        m_images->BarrierImageRangeForUsage(
            *commandBuffer,
            colorRenderPassAttachment.gpuImage,
            colorRenderPassAttachment.vkImageSubresourceRange,
            ImageUsageMode::ColorAttachment
        );
    }

    if (depthAttachment)
    {
        m_images->BarrierImageRangeForUsage(
            *commandBuffer,
            depthRenderPassAttachment->gpuImage,
            depthRenderPassAttachment->vkImageSubresourceRange,
            ImageUsageMode::DepthAttachment
        );
    }

    //
    // Begin dynamic rendering
    //
    VkRenderingInfo vkRenderingInfo{};
    vkRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    vkRenderingInfo.flags = 0; // TODO: Secondary command buffers?
    vkRenderingInfo.renderArea = VkRect2D {
        .offset = {
            .x = (int32_t)renderOffset.x, .y = (int32_t)renderOffset.y
        },
        .extent =  {
            .width = renderExtent.w, .height = renderExtent.h
        }
    };
    vkRenderingInfo.layerCount = 1;
    vkRenderingInfo.viewMask = 0;

    (*commandBuffer)->CmdBeginRendering(vkRenderingInfo, colorRenderPassAttachments, depthRenderPassAttachment);

    return RenderPass{.commandBufferId = commandBufferId};
}

bool WiredGPUVkImpl::EndRenderPass(RenderPass renderPass)
{
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(renderPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::EndRenderPass: No such command buffer exists: {}", renderPass.commandBufferId.id);
        return false;
    }

    if (!(*commandBuffer)->IsInRenderPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::EndRenderPass: Command buffer has no active render pass: {}", renderPass.commandBufferId.id);
        return false;
    }

    const auto& renderPassState = (*commandBuffer)->GetPassState();
    const auto renderPassColorAttachments = renderPassState->renderPassColorAttachments;
    const auto renderPassDepthAttachment = renderPassState->renderPassDepthAttachment;

    (*commandBuffer)->CmdEndRendering();

    //
    // Barrier attachments to default usage
    //
    for (const auto& colorRenderPassAttachment : renderPassColorAttachments)
    {
        m_images->BarrierImageRangeToDefaultUsage(
            *commandBuffer,
            colorRenderPassAttachment.gpuImage,
            colorRenderPassAttachment.vkImageSubresourceRange,
            ImageUsageMode::ColorAttachment
        );
    }

    if (renderPassDepthAttachment)
    {
        m_images->BarrierImageRangeToDefaultUsage(
            *commandBuffer,
            renderPassDepthAttachment->gpuImage,
            renderPassDepthAttachment->vkImageSubresourceRange,
            ImageUsageMode::DepthAttachment
        );
    }

    const auto result = (*commandBuffer)->EndRenderPass();

    // Finish command buffer section for the render pass
    EndCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer());

    return result;
}

std::expected<ComputePass, bool> WiredGPUVkImpl::BeginComputePass(CommandBufferId commandBufferId, const std::string& tag)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::BeginComputePass: No such command buffer exists: {}", commandBufferId.id);
        return std::unexpected(false);
    }

    if ((*commandBuffer)->IsInAnyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::BeginComputePass: Can't start a pass within another pass: {}", commandBufferId.id);
        return std::unexpected(false);
    }

    //
    // Execute
    //
    if (!(*commandBuffer)->BeginComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::BeginComputePass: Command buffer failed to start compute pass: {}", commandBufferId.id);
        return std::unexpected(false);
    }

    // Start command buffer section for the compute pass. (Finished in EndComputePass)
    BeginCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer(), std::format("ComputePass-{}", tag));

    return ComputePass{.commandBufferId = commandBufferId};
}

bool WiredGPUVkImpl::EndComputePass(ComputePass computePass)
{
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(computePass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::EndComputePass: No such command buffer exists: {}", computePass.commandBufferId.id);
        return false;
    }

    if (!(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::EndComputePass: Command buffer has no active compute pass: {}", computePass.commandBufferId.id);
        return false;
    }

    // Finish command buffer section for the compute pass
    EndCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer());

    return (*commandBuffer)->EndComputePass();
}

bool WiredGPUVkImpl::CreateShader(const ShaderSpec& shaderSpec)
{
    return m_shaders->CreateShader(shaderSpec);
}

void WiredGPUVkImpl::DestroyShader(const std::string& shaderName)
{
    m_shaders->DestroyShader(shaderName, false);
}

std::expected<PipelineId, bool> WiredGPUVkImpl::CreateGraphicsPipeline(const GraphicsPipelineParams& params)
{
    VkGraphicsPipelineConfig graphicsPipelineConfig{};
    graphicsPipelineConfig.vertShaderName = params.vertexShaderName;
    graphicsPipelineConfig.fragShaderName = params.fragmentShaderName;

    // ColorRenderAttachment -> PipelineColorAttachment
    for (const auto& colorAttachment : params.colorAttachments)
    {
        const auto gpuImage = m_images->GetImage(colorAttachment.imageId, false);
        if (!gpuImage)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::CreateGraphicsPipeline: No such color attachment image exists: {}", colorAttachment.imageId.id);
            return std::unexpected(false);
        }

        graphicsPipelineConfig.colorAttachments.push_back(PipelineColorAttachment{
            .vkFormat = gpuImage->imageData.imageDef.vkFormat,
            .enableColorBlending = true
        });
    }

    // DepthRenderAttachment -> PipelineDepthAttachment
    if (params.depthAttachment)
    {
        const auto gpuImage = m_images->GetImage(params.depthAttachment->imageId, false);
        if (!gpuImage)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::CreateGraphicsPipeline: No such depth attachment image exists: {}", params.depthAttachment->imageId.id);
            return std::unexpected(false);
        }

        graphicsPipelineConfig.depthAttachment = PipelineDepthAttachment{
            .vkFormat = gpuImage->imageData.imageDef.vkFormat
        };
    }

    graphicsPipelineConfig.viewport = params.viewport;

    graphicsPipelineConfig.cullFace = params.cullFace;
    graphicsPipelineConfig.depthBias = params.depthBiasEnabled ? DepthBias::Enabled : DepthBias::Disabled;
    graphicsPipelineConfig.polygonFillMode = params.wireframeFillMode ? PolygonFillMode::Line : PolygonFillMode::Fill;

    graphicsPipelineConfig.depthTestEnabled = params.depthTestEnabled;
    graphicsPipelineConfig.depthWriteEnabled = params.depthWriteEnabled;

    return m_pipelines->CreateGraphicsPipeline(graphicsPipelineConfig);
}

std::expected<PipelineId, bool> WiredGPUVkImpl::CreateComputePipeline(const ComputePipelineParams& params)
{
    VkComputePipelineConfig computePipelineConfig{};
    computePipelineConfig.computeShaderFileName = params.shaderName;

    return m_pipelines->CreateComputePipeline(computePipelineConfig);
}

void WiredGPUVkImpl::DestroyPipeline(PipelineId pipelineId)
{
    m_pipelines->DestroyPipeline(pipelineId, false);
}

std::expected<ImageId, bool> WiredGPUVkImpl::CreateImage(CommandBufferId commandBufferId,
                                                         const ImageCreateParams& params,
                                                         const std::string& tag)
{
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CreateImage: No such command buffer exists: {}", commandBufferId.id);
        return std::unexpected(false);
    }

    return m_images->CreateFromParams(*commandBuffer, params, tag);
}

void WiredGPUVkImpl::DestroyImage(ImageId imageId)
{
    m_images->DestroyImage(imageId, false);
}

bool WiredGPUVkImpl::GenerateMipMaps(CommandBufferId commandBufferId, ImageId imageId)
{
    //
    // Fetch data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::GenerateMipMaps: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    const auto gpuImage = m_images->GetImage(imageId, false);
    if (!gpuImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::GenerateMipMaps: No such image exists: {}", imageId.id);
        return false;
    }

    //
    // Validate
    //
    if ((*commandBuffer)->IsInAnyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::GenerateMipMaps: Must not be in an active pass: {}", commandBufferId.id);
        return false;
    }

    if (gpuImage->imageData.imageDef.vkImageType != VK_IMAGE_TYPE_2D)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::GenerateMipMaps: Must be a 2D image: {}", imageId.id);
        return false;
    }

    const auto formatProperties = m_global->physicalDevice.GetPhysicalDeviceFormatProperties(gpuImage->imageData.imageDef.vkFormat);
    if (!(formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    {
        m_global->pLogger->Error("WiredGPUVkImpl::GenerateMipMaps: Image has a format which is not linearly filterable: {}", imageId.id);
        return false;
    }

    //
    // Execute
    //
    const auto copyPass = BeginCopyPass(commandBufferId, std::format("GenerateMipMaps-{}", imageId.id));

    const auto mipLevels = gpuImage->imageData.imageDef.numMipLevels;

    auto mipWidth = gpuImage->imageData.imageDef.vkExtent.width;
    auto mipHeight = gpuImage->imageData.imageDef.vkExtent.height;

    for (uint32_t mipLevel = 1; mipLevel < mipLevels; ++mipLevel)
    {
        // Blit from the previous mip level to this mip level
        CmdBlitImage(
            *copyPass,
            imageId,
            ImageRegion{
                .layerIndex = 0,
                .mipLevel = mipLevel - 1,
                .offsets = {
                    NCommon::Point3DUInt{0, 0, 0},
                    NCommon::Point3DUInt{mipWidth, mipHeight, 1}
                }
            },
            imageId,
            ImageRegion{
                .layerIndex = 0,
                .mipLevel = mipLevel,
                .offsets = {
                    NCommon::Point3DUInt{0, 0, 0},
                    NCommon::Point3DUInt{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1}
                }
            },
            Filter::Linear,
            false
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    EndCopyPass(*copyPass);

    return true;
}

NCommon::Size2DUInt WiredGPUVkImpl::GetSwapChainSize() const
{
    if (!m_global->swapChain)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::GetSwapChainSize: Only valid to be called when a swap chain exists");
        return {0,0};
    }

    const auto swapChainConfig = m_global->swapChain->GetSwapChainConfig();

    return {swapChainConfig.extent.width, swapChainConfig.extent.height};
}

std::expected<BufferId, bool> WiredGPUVkImpl::CreateTransferBuffer(const TransferBufferCreateParams& transferBufferCreateParams, const std::string& tag)
{
    return m_buffers->CreateTransferBuffer(transferBufferCreateParams.usageFlags, transferBufferCreateParams.byteSize, transferBufferCreateParams.sequentiallyWritten, tag);
}

std::expected<BufferId, bool> WiredGPUVkImpl::CreateBuffer(const BufferCreateParams& bufferCreateParams, const std::string& tag)
{
    return m_buffers->CreateBuffer(bufferCreateParams.usageFlags, bufferCreateParams.byteSize, bufferCreateParams.dedicatedMemory, tag);
}

std::expected<void*, bool> WiredGPUVkImpl::MapBuffer(BufferId bufferId, bool cycle)
{
    return m_buffers->MapBuffer(bufferId, cycle);
}

bool WiredGPUVkImpl::UnmapBuffer(BufferId bufferId)
{
    return m_buffers->UnmapBuffer(bufferId);
}

void WiredGPUVkImpl::DestroyBuffer(BufferId bufferId)
{
    m_buffers->DestroyBuffer(bufferId, false);
}

std::expected<SamplerId, bool> WiredGPUVkImpl::CreateSampler(const SamplerInfo& samplerInfo, const std::string& tag)
{
    return m_samplers->CreateSampler(samplerInfo, tag);
}

void WiredGPUVkImpl::DestroySampler(SamplerId samplerId)
{
    m_samplers->DestroySampler(samplerId, false);
}

std::expected<CommandBufferId, bool> WiredGPUVkImpl::AcquireCommandBuffer(bool primary, const std::string& tag)
{
    //
    // Ensure the calling thread has a command pool created for it
    //
    const auto commandPool = EnsureThreadCommandPool();
    if (!commandPool)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::AcquireCommandBuffer: Failed to ensure thread command pool");
        return std::unexpected(false);
    }

    //
    // Acquire a command buffer
    //
    const auto commandBufferType = primary ? CommandBufferType::Primary : CommandBufferType::Secondary;

    const auto commandBuffer = m_commandBuffers->AcquireCommandBuffer(*commandPool, commandBufferType, tag);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::AcquireCommandBuffer: Failed to acquire command buffer");
        return std::unexpected(false);
    }
    const auto commandBufferId = (*commandBuffer)->GetId();

    //
    // Begin the command buffer
    //
    (*commandBuffer)->GetVulkanCommandBuffer().Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    //
    // If there's an active frame, associate the command buffer with it
    //
    auto& currentFrame = m_frames->GetCurrentFrame();
    if (currentFrame.IsActiveState())
    {
        currentFrame.AssociateCommandBuffer(commandBufferId);
    }

    return commandBufferId;
}

bool WiredGPUVkImpl::CmdClearColorImage(CopyPass copyPass,
                                        ImageId imageId,
                                        const ImageSubresourceRange& subresourceRange,
                                        const glm::vec4& color,
                                        bool cycle)
{
    //
    // Fetch data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(copyPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdClearColorImage: No such command buffer exists: {}", copyPass.commandBufferId.id);
        return false;
    }

    const auto gpuImage = m_images->GetImage(imageId, cycle, *commandBuffer);
    if (!gpuImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdClearColorImage: No such image exists: {}", imageId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInCopyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdClearColorImage: Command buffer is not in copy pass state");
        return false;
    }

    if (subresourceRange.imageAspect != ImageAspect::Color)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdClearColorImage: Cleared image range must be of color type: {}", imageId.id);
        return false;
    }

    if (gpuImage->imageData.imageDef.numLayers < (subresourceRange.baseArrayLayer + subresourceRange.layerCount))
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdClearColorImage: Layer count mismatch: {}", imageId.id);
        return false;
    }

    if (gpuImage->imageData.imageDef.numMipLevels < (subresourceRange.baseMipLevel + subresourceRange.levelCount))
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdClearColorImage: Mip level count mismatch: {}", imageId.id);
        return false;
    }

    //
    // Execute
    //
    const VkClearColorValue vkClearColorValue{ .float32 = { color.r, color.g, color.b, color.a } };

    const VkImageSubresourceRange vkImageSubresourceRange{
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = subresourceRange.baseMipLevel,
        .levelCount = subresourceRange.levelCount,
        .baseArrayLayer = subresourceRange.baseArrayLayer,
        .layerCount = subresourceRange.layerCount
    };

    m_images->BarrierImageRangeForUsage(*commandBuffer, *gpuImage, vkImageSubresourceRange, ImageUsageMode::TransferDst);

        (*commandBuffer)->CmdClearColorImage(*gpuImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &vkClearColorValue, 1, &vkImageSubresourceRange);

    m_images->BarrierImageRangeToDefaultUsage(*commandBuffer, *gpuImage, vkImageSubresourceRange, ImageUsageMode::TransferDst);

    return true;
}

bool WiredGPUVkImpl::CmdBlitImage(CopyPass copyPass,
                                  ImageId sourceImageId,
                                  const ImageRegion& sourceRegion,
                                  ImageId destImageId,
                                  const ImageRegion& destRegion,
                                  Filter filter,
                                  bool cycle)
{
    //
    // Fetch data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(copyPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBlitImage: No such command buffer exists: {}", copyPass.commandBufferId.id);
        return false;
    }

    const auto sourceGpuImage = m_images->GetImage(sourceImageId, false);
    if (!sourceGpuImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBlitImage: No such source image exists: {}", sourceImageId.id);
        return false;
    }

    const auto destGpuImage = m_images->GetImage(destImageId, cycle, *commandBuffer);
    if (!destGpuImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBlitImage: No such dest image exists: {}", destImageId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInCopyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBlitImage: Command buffer is not in copy pass state");
        return false;
    }

    const VkExtent3D sourceImageExtent = sourceGpuImage->imageData.imageDef.vkExtent;
    auto sourceRegionOffset0 = VkOffset3D {.x = (int32_t)sourceRegion.offsets[0].x, .y = (int32_t)sourceRegion.offsets[0].y, .z = (int32_t)sourceRegion.offsets[0].z};
    auto sourceRegionOffset1 = VkOffset3D {.x = (int32_t)sourceRegion.offsets[1].x, .y = (int32_t)sourceRegion.offsets[1].y, .z = (int32_t)sourceRegion.offsets[1].z};

    const VkExtent3D destImageExtent = destGpuImage->imageData.imageDef.vkExtent;
    auto destRegionOffset0 = VkOffset3D {.x = (int32_t)destRegion.offsets[0].x, .y = (int32_t)destRegion.offsets[0].y, .z = (int32_t)destRegion.offsets[0].z};
    auto destRegionOffset1 = VkOffset3D {.x = (int32_t)destRegion.offsets[1].x, .y = (int32_t)destRegion.offsets[1].y, .z = (int32_t)destRegion.offsets[1].z};

    if (!AreAllOffsetsWithinExtent({sourceRegionOffset0, sourceRegionOffset1}, sourceImageExtent))
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBlitImage: Source region offsets aren't within source image extent: {}", sourceImageId.id);
        return false;
    }

    if (!AreAllOffsetsWithinExtent({destRegionOffset0, destRegionOffset1}, destImageExtent))
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBlitImage: Dest region offsets aren't within dest image extent: {}", destImageId.id);
        return false;
    }

    // If not 3D image, region zs must be 0,1
    if (sourceGpuImage->imageData.imageDef.vkImageType != VK_IMAGE_TYPE_3D)
    {
        sourceRegionOffset0.z = 0;
        sourceRegionOffset1.z = 1;
    }
    if (destGpuImage->imageData.imageDef.vkImageType != VK_IMAGE_TYPE_3D)
    {
        destRegionOffset0.z = 0;
        destRegionOffset1.z = 1;
    }

    const VkImageSubresourceRange vkSourceSubresourceRange = {
        .aspectMask = Images::GetImageAspectFlags(*sourceGpuImage),
        .baseMipLevel = sourceRegion.mipLevel,
        .levelCount = 1,
        .baseArrayLayer = sourceRegion.layerIndex,
        .layerCount = 1
    };

    const VkImageSubresourceRange vkDestSubresourceRange = {
        .aspectMask = Images::GetImageAspectFlags(*destGpuImage),
        .baseMipLevel = destRegion.mipLevel,
        .levelCount = 1,
        .baseArrayLayer = destRegion.layerIndex,
        .layerCount = 1
    };

    VkFilter vkFilter{};
    switch (filter)
    {
        case Filter::Linear: vkFilter = VK_FILTER_LINEAR; break;
        case Filter::Nearest: vkFilter = VK_FILTER_NEAREST; break;
    }

    //
    // Execute
    //
    m_images->BarrierImageRangeForUsage(*commandBuffer, *sourceGpuImage, vkSourceSubresourceRange, ImageUsageMode::TransferSrc);
    m_images->BarrierImageRangeForUsage(*commandBuffer, *destGpuImage, vkDestSubresourceRange, ImageUsageMode::TransferDst);

        const VkImageBlit vkImageBlit{
            .srcSubresource = {
                .aspectMask = Images::GetImageAspectFlags(*sourceGpuImage),
                .mipLevel = sourceRegion.mipLevel,
                .baseArrayLayer = sourceRegion.layerIndex,
                .layerCount = 1
            },
            .srcOffsets = {sourceRegionOffset0, sourceRegionOffset1},
            .dstSubresource = {
                .aspectMask = Images::GetImageAspectFlags(*destGpuImage),
                .mipLevel = destRegion.mipLevel,
                .baseArrayLayer = destRegion.layerIndex,
                .layerCount = 1
            },
            .dstOffsets = {destRegionOffset0, destRegionOffset1}
        };

        (*commandBuffer)->CmdBlitImage(
            *sourceGpuImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            *destGpuImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            vkImageBlit,
            vkFilter
        );

    m_images->BarrierImageRangeToDefaultUsage(*commandBuffer, *sourceGpuImage, vkSourceSubresourceRange, ImageUsageMode::TransferSrc);
    m_images->BarrierImageRangeToDefaultUsage(*commandBuffer, *destGpuImage, vkDestSubresourceRange, ImageUsageMode::TransferDst);

    return true;
}

bool WiredGPUVkImpl::CmdUploadDataToBuffer(CopyPass copyPass,
                                           BufferId sourceTransferBufferId,
                                           const std::size_t& sourceByteOffset,
                                           BufferId destBufferId,
                                           const std::size_t& destByteOffset,
                                           const std::size_t& copyByteSize,
                                           bool cycle)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(copyPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: No such command buffer exists: {}", copyPass.commandBufferId.id);
        return false;
    }

    const auto sourceBuffer = m_buffers->GetBuffer(sourceTransferBufferId, false);
    if (!sourceBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: No such transfer buffer exists: {}", sourceTransferBufferId.id);
        return false;
    }

    const auto destBuffer = m_buffers->GetBuffer(destBufferId, cycle);
    if (!destBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: Failed to find or cycle dest buffer: {}", destBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInCopyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: Command buffer is not in copy pass state");
        return false;
    }

    if (!sourceBuffer->bufferDef.isTransferBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: sourceTransferBufferId must be a transfer buffer: {}", sourceTransferBufferId.id);
        return false;
    }

    if (destBuffer->bufferDef.isTransferBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: destBufferId must not be a transfer buffer: {}", destBufferId.id);
        return false;
    }

    if (sourceByteOffset + copyByteSize > sourceBuffer->bufferDef.byteSize)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: source region is out of bounds of the buffer's size");
        return false;
    }

    if (destByteOffset + copyByteSize > destBuffer->bufferDef.byteSize)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToBuffer: dest region is out of bounds of the buffer's size");
        return false;
    }

    //
    // Execute
    //
    m_buffers->BarrierBufferRangeForUsage(*commandBuffer, *sourceBuffer, sourceByteOffset, copyByteSize, BufferUsageMode::TransferSrc);
    m_buffers->BarrierBufferRangeForUsage(*commandBuffer, *destBuffer, destByteOffset, copyByteSize, BufferUsageMode::TransferDst);

        VkBufferCopy2 vkCopyRegion{};
        vkCopyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
        vkCopyRegion.srcOffset = sourceByteOffset;
        vkCopyRegion.dstOffset = destByteOffset;
        vkCopyRegion.size = copyByteSize;

        VkCopyBufferInfo2 vkCopyBufferInfo{};
        vkCopyBufferInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
        vkCopyBufferInfo.srcBuffer = sourceBuffer->vkBuffer;
        vkCopyBufferInfo.dstBuffer = destBuffer->vkBuffer;
        vkCopyBufferInfo.regionCount = 1;
        vkCopyBufferInfo.pRegions = &vkCopyRegion;

        (*commandBuffer)->CmdCopyBuffer2(&vkCopyBufferInfo);

    m_buffers->BarrierBufferRangeToDefaultUsage(*commandBuffer, *sourceBuffer, sourceByteOffset, copyByteSize, BufferUsageMode::TransferSrc);
    m_buffers->BarrierBufferRangeToDefaultUsage(*commandBuffer, *destBuffer, destByteOffset, copyByteSize, BufferUsageMode::TransferDst);

    return true;
}

bool WiredGPUVkImpl::CmdUploadDataToImage(CopyPass copyPass,
                                          BufferId sourceTransferBufferId,
                                          const std::size_t& sourceByteOffset,
                                          ImageId destImageId,
                                          const ImageRegion& destRegion,
                                          const std::size_t& copyByteSize,
                                          bool cycle)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(copyPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToImage: No such command buffer exists: {}", copyPass.commandBufferId.id);
        return false;
    }

    const auto sourceBuffer = m_buffers->GetBuffer(sourceTransferBufferId, false);
    if (!sourceBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToImage: No such transfer buffer exists: {}", sourceTransferBufferId.id);
        return false;
    }

    const auto destImage = m_images->GetImage(destImageId, cycle, *commandBuffer);
    if (!destImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToImage: Failed to find or cycle image: {}", destImageId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInCopyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToImage: Command buffer has no copy pass started");
        return false;
    }

    if (sourceByteOffset + copyByteSize > sourceBuffer->bufferDef.byteSize)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdUploadDataToImage: Copy size is larger than the source buffer's size");
        return false;
    }

    // TODO: Verify whether the byte size being copied can fit in the image

    const VkImageSubresourceRange vkDestSubresourceRange = {
        .aspectMask = Images::GetImageAspectFlags(*destImage),
        .baseMipLevel = destRegion.mipLevel,
        .levelCount = 1,
        .baseArrayLayer = destRegion.layerIndex,
        .layerCount = 1
    };

    //
    // Execute
    //
    m_buffers->BarrierBufferRangeForUsage(*commandBuffer, *sourceBuffer, sourceByteOffset, copyByteSize, BufferUsageMode::TransferSrc);
    m_images->BarrierImageRangeForUsage(*commandBuffer, *destImage, vkDestSubresourceRange, ImageUsageMode::TransferDst);

        VkBufferImageCopy2 vkCopyRegion{};
        vkCopyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2;
        vkCopyRegion.bufferOffset = 0;
        vkCopyRegion.bufferRowLength = 0;
        vkCopyRegion.bufferImageHeight = 0;
        vkCopyRegion.imageSubresource = {
            .aspectMask = m_images->GetImageAspectFlags(*destImage),
            .mipLevel = destRegion.mipLevel,
            .baseArrayLayer = destRegion.layerIndex,
            .layerCount = 1
        };
        vkCopyRegion.imageOffset = {
            .x = (int32_t)destRegion.offsets[0].x,
            .y = (int32_t)destRegion.offsets[0].y,
            .z = (int32_t)destRegion.offsets[0].z
        };

        const uint32_t copyWidth = destRegion.offsets[1].x - destRegion.offsets[0].x;
        const uint32_t copyHeight = destRegion.offsets[1].y - destRegion.offsets[0].y;
        const uint32_t copyDepth = destRegion.offsets[1].z - destRegion.offsets[0].z;
        vkCopyRegion.imageExtent = VkExtent3D {
            .width = copyWidth,
            .height = copyHeight,
            .depth = copyDepth
        };

        VkCopyBufferToImageInfo2 vkCopyBufferToImageInfo{};
        vkCopyBufferToImageInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_TO_IMAGE_INFO_2;
        vkCopyBufferToImageInfo.srcBuffer = sourceBuffer->vkBuffer;
        vkCopyBufferToImageInfo.dstImage = destImage->imageData.vkImage;
        vkCopyBufferToImageInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        vkCopyBufferToImageInfo.regionCount = 1;
        vkCopyBufferToImageInfo.pRegions = &vkCopyRegion;

        (*commandBuffer)->CmdCopyBufferToImage2(&vkCopyBufferToImageInfo);

    m_images->BarrierImageRangeToDefaultUsage(*commandBuffer, *destImage, vkDestSubresourceRange, ImageUsageMode::TransferDst);
    m_buffers->BarrierBufferRangeToDefaultUsage(*commandBuffer, *sourceBuffer, 0, copyByteSize, BufferUsageMode::TransferSrc);

    return true;
}

bool WiredGPUVkImpl::CmdCopyBufferToBuffer(CopyPass copyPass,
                                           BufferId sourceBufferId,
                                           const std::size_t& sourceByteOffset,
                                           BufferId destBufferId,
                                           const std::size_t& destByteOffset,
                                           const std::size_t& copyByteSize,
                                           bool cycle)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(copyPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdCopyBufferToBuffer: No such command buffer exists: {}", copyPass.commandBufferId.id);
        return false;
    }

    const auto sourceBuffer = m_buffers->GetBuffer(sourceBufferId, false);
    if (!sourceBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdCopyBufferToBuffer: No such source buffer exists: {}", sourceBufferId.id);
        return false;
    }

    const auto destBuffer = m_buffers->GetBuffer(destBufferId, cycle);
    if (!destBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdCopyBufferToBuffer: Failed to find or cycle dest buffer: {}", destBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInCopyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdCopyBufferToBuffer: Command buffer is not in copy pass state");
        return false;
    }

    if (sourceByteOffset + copyByteSize > sourceBuffer->bufferDef.byteSize)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdCopyBufferToBuffer: source region is out of bounds of the buffer's size");
        return false;
    }

    if (destByteOffset + copyByteSize > destBuffer->bufferDef.byteSize)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdCopyBufferToBuffer: dest region is out of bounds of the buffer's size");
        return false;
    }

    //
    // Execute
    //
    m_buffers->BarrierBufferRangeForUsage(*commandBuffer, *sourceBuffer, sourceByteOffset, copyByteSize, BufferUsageMode::TransferSrc);
    m_buffers->BarrierBufferRangeForUsage(*commandBuffer, *destBuffer, destByteOffset, copyByteSize, BufferUsageMode::TransferDst);

    VkBufferCopy2 vkCopyRegion{};
    vkCopyRegion.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
    vkCopyRegion.srcOffset = sourceByteOffset;
    vkCopyRegion.dstOffset = destByteOffset;
    vkCopyRegion.size = copyByteSize;

    VkCopyBufferInfo2 vkCopyBufferInfo{};
    vkCopyBufferInfo.sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
    vkCopyBufferInfo.srcBuffer = sourceBuffer->vkBuffer;
    vkCopyBufferInfo.dstBuffer = destBuffer->vkBuffer;
    vkCopyBufferInfo.regionCount = 1;
    vkCopyBufferInfo.pRegions = &vkCopyRegion;

    (*commandBuffer)->CmdCopyBuffer2(&vkCopyBufferInfo);

    m_buffers->BarrierBufferRangeToDefaultUsage(*commandBuffer, *sourceBuffer, sourceByteOffset, copyByteSize, BufferUsageMode::TransferSrc);
    m_buffers->BarrierBufferRangeToDefaultUsage(*commandBuffer, *destBuffer, destByteOffset, copyByteSize, BufferUsageMode::TransferDst);

    return true;
}

bool WiredGPUVkImpl::CmdExecuteCommands(CommandBufferId primaryCommandBufferId, const std::vector<CommandBufferId>& secondaryCommandBufferIds)
{
    //
    // Fetch data
    //
    const auto primaryCommandBuffer = m_commandBuffers->GetCommandBuffer(primaryCommandBufferId);
    if (!primaryCommandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::ExecuteCommandBuffer: No such primary command buffer exists: {}", primaryCommandBufferId.id);
        return false;
    }

    if ((*primaryCommandBuffer)->GetType() != CommandBufferType::Primary)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::ExecuteCommandBuffer: Must be a primary command buffer: {}", primaryCommandBufferId.id);
        return false;
    }
    std::vector<CommandBuffer*> secondaryCommandBuffers;

    for (const auto& secondaryCommandBufferId : secondaryCommandBufferIds)
    {
        const auto secondaryCommandBuffer = m_commandBuffers->GetCommandBuffer(secondaryCommandBufferId);
        if (!secondaryCommandBuffer)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::ExecuteCommandBuffer: No such secondary command buffer exists: {}", secondaryCommandBufferId.id);
            continue;
        }

        if ((*secondaryCommandBuffer)->GetType() != CommandBufferType::Secondary)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::ExecuteCommandBuffer: Must be a secondary command buffer: {}", secondaryCommandBufferId.id);
            continue;
        }

        if ((*secondaryCommandBuffer)->IsInAnyPass())
        {
            m_global->pLogger->Error("WiredGPUVkImpl::ExecuteCommandBuffer: Secondary command buffer is in an open pass: {}", secondaryCommandBufferId.id);
            continue;
        }

        //
        // End the recording of each secondary command buffer
        //
        (*secondaryCommandBuffer)->GetVulkanCommandBuffer().End();

        // Keep track of the new secondary command buffer
        secondaryCommandBuffers.push_back(*secondaryCommandBuffer);
    }

    //
    // Execute
    //
    (*primaryCommandBuffer)->CmdExecuteCommands(secondaryCommandBuffers);

    return true;
}

bool WiredGPUVkImpl::CmdBindPipeline(RenderOrComputePass pass, PipelineId pipelineId)
{
    //
    // Fetch Data
    //
    const auto commandBufferId = std::visit([](auto&& obj) -> decltype(auto) { return obj.commandBufferId; }, pass);

    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindPipeline: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    const auto pipeline = m_pipelines->GetPipeline(pipelineId);
    if (!pipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindPipeline: No such pipeline exists: {}", pipelineId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass() && !(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindPipeline: Only allowed inside a render or compute pass");
        return false;
    }

    //
    // Execute
    //
    (*commandBuffer)->CmdBindPipeline(*pipeline);

    return true;
}

bool WiredGPUVkImpl::CmdBindVertexBuffers(RenderPass renderPass, uint32_t firstBinding, const std::vector<BufferBinding>& bindings)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(renderPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindVertexBuffers: No such command buffer exists: {}", renderPass.commandBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindVertexBuffers: Only allowed inside a render pass");
        return false;
    }

    (void)firstBinding;
    if (bindings.size() != 1)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindVertexBuffers: Only binding one vertex buffer is supported at the moment");
        return false;
    }

    //
    // Execute
    //
    return (*commandBuffer)->CmdBindVertexBuffer(bindings.at(0));
}

bool WiredGPUVkImpl::CmdBindIndexBuffer(RenderPass renderPass, const BufferBinding& binding, IndexType indexType)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(renderPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindIndexBuffer: No such command buffer exists: {}", renderPass.commandBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindIndexBuffer: Only allowed inside a render pass");
        return false;
    }

    //
    // Execute
    //
    return (*commandBuffer)->CmdBindIndexBuffer(binding, indexType);
}

std::expected<BufferUsageMode, bool> GetGraphicsBufferUsageMode(const VkBufferBinding& bufferBinding)
{
    switch (bufferBinding.vkDescriptorType)
    {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return BufferUsageMode::GraphicsUniformRead;

        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return BufferUsageMode::GraphicsStorageRead;

        default:
            return std::unexpected(false);
    }
}

void WiredGPUVkImpl::BarrierGraphicsSetResourcesForUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings)
{
    for (const auto& bufferBinding: setBindings.bufferBindings)
    {
        const auto bufferUsageMode = GetGraphicsBufferUsageMode(bufferBinding.second);
        if (!bufferUsageMode)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::BarrierGraphicsSetResourcesForUsage: Unsupported buffer descriptor type");
            return;
        }

        m_buffers->BarrierBufferRangeForUsage(
            pCommandBuffer,
            bufferBinding.second.gpuBuffer,
            bufferBinding.second.dynamicByteOffset ? *bufferBinding.second.dynamicByteOffset : bufferBinding.second.byteOffset,
            bufferBinding.second.byteSize,
            *bufferUsageMode
        );
    }
    for (const auto& imageViewBinding: setBindings.imageViewBindings)
    {
        const auto& imageViewData = imageViewBinding.second.gpuImage.imageViewDatas.at(imageViewBinding.second.imageViewIndex);

        m_images->BarrierImageRangeForUsage(
            pCommandBuffer,
            imageViewBinding.second.gpuImage,
            imageViewData.imageViewDef.vkImageSubresourceRange,
            ImageUsageMode::GraphicsStorageRead
        );
    }
    for (const auto& imageViewSamplerBindings : setBindings.imageViewSamplerBindings)
    {
        for (const auto& imageViewSamplerBinding : imageViewSamplerBindings.second.arrayBindings)
        {
            const auto& imageViewData = imageViewSamplerBinding.second.gpuImage.imageViewDatas.at(imageViewSamplerBinding.second.imageViewIndex);

            m_images->BarrierImageRangeForUsage(
                pCommandBuffer,
                imageViewSamplerBinding.second.gpuImage,
                imageViewData.imageViewDef.vkImageSubresourceRange,
                ImageUsageMode::GraphicsSampled
            );
        }
    }
}

void WiredGPUVkImpl::BarrierGraphicsSetResourcesToDefaultUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings)
{
    for (const auto& bufferBinding: setBindings.bufferBindings)
    {
        const auto bufferUsageMode = GetGraphicsBufferUsageMode(bufferBinding.second);
        if (!bufferUsageMode)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::BarrierGraphicsSetResourcesToDefaultUsage: Unsupported buffer descriptor type");
            return;
        }

        m_buffers->BarrierBufferRangeToDefaultUsage(
            pCommandBuffer,
            bufferBinding.second.gpuBuffer,
            bufferBinding.second.dynamicByteOffset ? *bufferBinding.second.dynamicByteOffset : bufferBinding.second.byteOffset,
            bufferBinding.second.byteSize,
            *bufferUsageMode
        );
    }
    for (const auto& imageViewBinding: setBindings.imageViewBindings)
    {
        const auto& imageViewData = imageViewBinding.second.gpuImage.imageViewDatas.at(imageViewBinding.second.imageViewIndex);

        m_images->BarrierImageRangeToDefaultUsage(
            pCommandBuffer,
            imageViewBinding.second.gpuImage,
            imageViewData.imageViewDef.vkImageSubresourceRange,
            ImageUsageMode::GraphicsStorageRead
        );
    }
    for (const auto& imageViewSamplerBindings : setBindings.imageViewSamplerBindings)
    {
        for (const auto& imageViewSamplerBinding : imageViewSamplerBindings.second.arrayBindings)
        {
            const auto& imageViewData = imageViewSamplerBinding.second.gpuImage.imageViewDatas.at(imageViewSamplerBinding.second.imageViewIndex);

            m_images->BarrierImageRangeToDefaultUsage(
                pCommandBuffer,
                imageViewSamplerBinding.second.gpuImage,
                imageViewData.imageViewDef.vkImageSubresourceRange,
                ImageUsageMode::GraphicsSampled
            );
        }
    }
}

std::expected<BufferUsageMode, bool> GetComputeBufferUsageMode(const VkBufferBinding& bufferBinding)
{
    switch (bufferBinding.vkDescriptorType)
    {
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return BufferUsageMode::ComputeUniformRead;

        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            if (bufferBinding.shaderWriteable)
                return BufferUsageMode::ComputeStorageReadWrite;
            else
                return BufferUsageMode::ComputeStorageRead;

        default:
            return std::unexpected(false);
    }
}

void WiredGPUVkImpl::BarrierComputeSetResourcesForUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings)
{
    for (const auto& bufferBinding: setBindings.bufferBindings)
    {
        const auto bufferUsageMode = GetComputeBufferUsageMode(bufferBinding.second);
        if (!bufferUsageMode)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::BarrierComputeSetResourcesForUsage: Unsupported buffer descriptor type");
            return;
        }

        m_buffers->BarrierBufferRangeForUsage(
            pCommandBuffer,
            bufferBinding.second.gpuBuffer,
            bufferBinding.second.byteOffset,
            bufferBinding.second.byteSize,
            *bufferUsageMode
        );
    }
    for (const auto& imageViewBinding: setBindings.imageViewBindings)
    {
        const auto& imageViewData = imageViewBinding.second.gpuImage.imageViewDatas.at(imageViewBinding.second.imageViewIndex);

        m_images->BarrierImageRangeForUsage(
            pCommandBuffer,
            imageViewBinding.second.gpuImage,
            imageViewData.imageViewDef.vkImageSubresourceRange,
            imageViewBinding.second.shaderWriteable ? ImageUsageMode::ComputeStorageReadWrite : ImageUsageMode::ComputeStorageRead
        );
    }
    for (const auto& imageViewSamplerBindings : setBindings.imageViewSamplerBindings)
    {
        for (const auto& imageViewSamplerBinding : imageViewSamplerBindings.second.arrayBindings)
        {
            const auto& imageViewData = imageViewSamplerBinding.second.gpuImage.imageViewDatas.at(imageViewSamplerBinding.second.imageViewIndex);

            m_images->BarrierImageRangeForUsage(
                pCommandBuffer,
                imageViewSamplerBinding.second.gpuImage,
                imageViewData.imageViewDef.vkImageSubresourceRange,
                ImageUsageMode::ComputeSampled
            );
        }
    }
}

void WiredGPUVkImpl::BarrierComputeSetResourcesToDefaultUsage(CommandBuffer* pCommandBuffer, const SetBindings& setBindings)
{
    for (const auto& bufferBinding: setBindings.bufferBindings)
    {
        const auto bufferUsageMode = GetComputeBufferUsageMode(bufferBinding.second);
        if (!bufferUsageMode)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::BarrierComputeSetResourcesToDefaultUsage: Unsupported buffer descriptor type");
            return;
        }

        m_buffers->BarrierBufferRangeToDefaultUsage(
            pCommandBuffer,
            bufferBinding.second.gpuBuffer,
            bufferBinding.second.byteOffset,
            bufferBinding.second.byteSize,
            *bufferUsageMode
        );
    }
    for (const auto& imageViewBinding: setBindings.imageViewBindings)
    {
        const auto& imageViewData = imageViewBinding.second.gpuImage.imageViewDatas.at(imageViewBinding.second.imageViewIndex);

        m_images->BarrierImageRangeToDefaultUsage(
            pCommandBuffer,
            imageViewBinding.second.gpuImage,
            imageViewData.imageViewDef.vkImageSubresourceRange,
            imageViewBinding.second.shaderWriteable ? ImageUsageMode::ComputeStorageReadWrite : ImageUsageMode::ComputeStorageRead
        );
    }
    for (const auto& imageViewSamplerBindings : setBindings.imageViewSamplerBindings)
    {
        for (const auto& imageViewSamplerBinding : imageViewSamplerBindings.second.arrayBindings)
        {
            const auto& imageViewData = imageViewSamplerBinding.second.gpuImage.imageViewDatas.at(imageViewSamplerBinding.second.imageViewIndex);

            m_images->BarrierImageRangeToDefaultUsage(
                pCommandBuffer,
                imageViewSamplerBinding.second.gpuImage,
                imageViewData.imageViewDef.vkImageSubresourceRange,
                ImageUsageMode::ComputeSampled
            );
        }
    }
}

void WiredGPUVkImpl::BindDescriptorSetsNeedingRefresh(CommandBuffer* pCommandBuffer, PassState& passState)
{
    const auto descriptorSets = *EnsureThreadDescriptorSets();

    // Note that we're relying on external logic being correct; if any set X needs refreshed, every set
    // after it should also have been marked as needing refresh, so we should have contiguous set indices
    // that we're updating - which CmdBindDescriptorSets relies on.
    unsigned int lowestSetWritten = 4;
    std::vector<VulkanDescriptorSet> setsWritten;
    std::vector<uint32_t> dynamicOffsets;

    for (unsigned int set = 0; set < 4; ++set)
    {
        // Only bind the set if the render pass state says it needs refreshing
        if (!passState.setsNeedingRefresh[set]) { continue; }

        const auto setBindings = passState.setBindings.at(set);

        std::expected<VulkanDescriptorSet, bool> vulkanDescriptorSet;

        // Obtain a descriptor set
        vulkanDescriptorSet = descriptorSets->GetVulkanDescriptorSet(
            DescriptorSetRequest{
                .descriptorSetLayout =  passState.boundPipeline->GetDescriptorLayout(set),
                .bindings = setBindings
            },
            std::format("DS{}", set)
        );

        lowestSetWritten = std::min(set, lowestSetWritten);
        setsWritten.push_back(*vulkanDescriptorSet);

        // Record any dynamic offsets the set's bindings are requesting. Start by recording which binding
        // index has a dynamic offset
        std::vector<std::pair<uint32_t, uint32_t>> bindingIndexToDynamicOffset;

        for (const auto& bufferBinding : setBindings.bufferBindings)
        {
            if (bufferBinding.second.dynamicByteOffset)
            {
                bindingIndexToDynamicOffset.emplace_back(bufferBinding.first, *bufferBinding.second.dynamicByteOffset);
            }
        }

        // Sort the dynamic offsets by binding index, as we need to supply them in binding order
        std::ranges::sort(bindingIndexToDynamicOffset, [](const auto& o1, const auto& o2){
           return o1.first < o2.first;
        });

        // Record the dynamic offsets in binding order
        for (const auto& it : bindingIndexToDynamicOffset)
        {
            dynamicOffsets.push_back(it.second);
        }

        // Update pass state to show that we bound the DS
        passState.setsNeedingRefresh[set] = false;
    }

    if (!setsWritten.empty())
    {
        pCommandBuffer->CmdBindDescriptorSets(*passState.boundPipeline, lowestSetWritten, setsWritten, dynamicOffsets);
    }
}

bool WiredGPUVkImpl::CmdDrawIndexed(RenderPass renderPass,
                                    uint32_t indexCount,
                                    uint32_t instanceCount,
                                    uint32_t firstIndex,
                                    int32_t vertexOffset,
                                    uint32_t firstInstance)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(renderPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexed: No such command buffer exists: {}", renderPass.commandBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexed: Only allowed inside a render pass");
        return false;
    }

    auto& renderPassState = (*commandBuffer)->GetPassState();

    if (!renderPassState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexed: Can't draw without a bound pipeline");
        return false;
    }

    //
    // Execute
    //

    // Bind descriptor sets that the pass state reports need refreshing
    BindDescriptorSetsNeedingRefresh(*commandBuffer, *renderPassState);

    // Barrier all bound resources for usage
    for (const auto& setBindings : renderPassState->setBindings)
    {
        BarrierGraphicsSetResourcesForUsage(*commandBuffer, setBindings);
    }

    // Draw
    (*commandBuffer)->CmdDrawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);

    // Barrier all resources back to their default usage
    for (const auto& setBindings : renderPassState->setBindings)
    {
        BarrierGraphicsSetResourcesToDefaultUsage(*commandBuffer, setBindings);
    }

    return true;
}

bool WiredGPUVkImpl::CmdDrawIndexedIndirect(RenderPass renderPass,
                                            BufferId bufferId,
                                            const std::size_t& byteOffset,
                                            uint32_t drawCount,
                                            uint32_t stride)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(renderPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirect: No such command buffer exists: {}", renderPass.commandBufferId.id);
        return false;
    }

    const auto buffer = m_buffers->GetBuffer(bufferId, false);
    if (!buffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirect: No such buffer exists: {}", bufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirect: Only allowed inside a render pass");
        return false;
    }

    auto& renderPassState = (*commandBuffer)->GetPassState();

    if (!renderPassState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirect: Can't draw without a bound pipeline");
        return false;
    }

    //
    // Execute
    //

    // Bind descriptor sets that the pass state reports need refreshing
    BindDescriptorSetsNeedingRefresh(*commandBuffer, *renderPassState);

    //
    // Barrier all bound resources for usage
    //
    for (const auto& setBindings : renderPassState->setBindings)
    {
        BarrierGraphicsSetResourcesForUsage(*commandBuffer, setBindings);
    }

    //
    // Draw
    //
    (*commandBuffer)->CmdDrawIndexedIndirect(buffer->vkBuffer, byteOffset, drawCount, stride);

    //
    // Barrier all resources back to their default usage
    //
    for (const auto& setBindings : renderPassState->setBindings)
    {
        BarrierGraphicsSetResourcesToDefaultUsage(*commandBuffer, setBindings);
    }

    return true;
}

bool WiredGPUVkImpl::CmdDrawIndexedIndirectCount(RenderPass renderPass,
                                                BufferId commandsBufferId,
                                                const std::size_t& commandsByteOffset,
                                                BufferId countsBufferId,
                                                const std::size_t& countByteOffset,
                                                uint32_t maxDrawCount,
                                                uint32_t stride)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(renderPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirectCount: No such command buffer exists: {}", renderPass.commandBufferId.id);
        return false;
    }

    const auto drawCommandsBuffer = m_buffers->GetBuffer(commandsBufferId, false);
    if (!drawCommandsBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirectCount: No such draw commands buffer exists: {}", commandsBufferId.id);
        return false;
    }

    const auto drawCountsBuffer = m_buffers->GetBuffer(countsBufferId, false);
    if (!drawCommandsBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirectCount: No such draw counts buffer exists: {}", countsBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirectCount: Only allowed inside a render pass");
        return false;
    }

    auto& renderPassState = (*commandBuffer)->GetPassState();

    if (!renderPassState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDrawIndexedIndirectCount: Can't draw without a bound pipeline");
        return false;
    }

    //
    // Execute
    //

    // Bind descriptor sets that the pass state reports need refreshing
    BindDescriptorSetsNeedingRefresh(*commandBuffer, *renderPassState);

    //
    // Barrier all bound resources for usage
    //
    for (const auto& setBindings : renderPassState->setBindings)
    {
        BarrierGraphicsSetResourcesForUsage(*commandBuffer, setBindings);
    }

    //
    // Draw
    //
    (*commandBuffer)->CmdDrawIndexedIndirectCount(
        drawCommandsBuffer->vkBuffer,
        commandsByteOffset,
        drawCountsBuffer->vkBuffer,
        countByteOffset,
        maxDrawCount,
        stride
    );

    //
    // Barrier all resources back to their default usage
    //
    for (const auto& setBindings : renderPassState->setBindings)
    {
        BarrierGraphicsSetResourcesToDefaultUsage(*commandBuffer, setBindings);
    }

    return true;
}

bool WiredGPUVkImpl::CmdDispatch(ComputePass computePass, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(computePass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDispatch: No such command buffer exists: {}", computePass.commandBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDispatch: Only allowed inside a compute pass");
        return false;
    }

    auto& computePassState = (*commandBuffer)->GetPassState();

    if (!computePassState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdDispatch: Can't dispatch without a bound pipeline");
        return false;
    }

    //
    // Execute
    //

    // Bind descriptor sets that the pass state reports need refreshing
    BindDescriptorSetsNeedingRefresh(*commandBuffer, *computePassState);

    //
    // Barrier all bound resources for usage
    //
    for (const auto& setBindings : computePassState->setBindings)
    {
        BarrierComputeSetResourcesForUsage(*commandBuffer, setBindings);
    }

    //
    // Dispatch
    //
    (*commandBuffer)->CmdDispatch(groupCountX, groupCountY, groupCountZ);

    //
    // Barrier all resources back to their default usage
    //
    for (const auto& setBindings : computePassState->setBindings)
    {
        BarrierComputeSetResourcesToDefaultUsage(*commandBuffer, setBindings);
    }

    return true;
}

#ifdef WIRED_IMGUI
bool WiredGPUVkImpl::CmdRenderImGuiDrawData(RenderPass renderPass, ImDrawData* pDrawData)
{
    if (!m_global->imGuiActive) { return false; }

    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(renderPass.commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdRenderImGuiDrawData: No such command buffer exists: {}", renderPass.commandBufferId.id);
        return false;
    }

    // Look up all the images that ImGui is referencing for the current frame
    std::vector<GPUImage> referencedImages;

    for (const auto& imGuiReference : m_frames->GetCurrentFrame().GetImGuiImageReferences())
    {
        const auto image = m_images->GetImage(imGuiReference.imageId, false);
        if (!image)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::CmdRenderImGuiDrawData: Frame referenced image doesn't exist: {}", imGuiReference.imageId.id);
            continue;
        }

        referencedImages.push_back(*image);
    }

    //
    // Execute
    //
    CmdBufferSectionLabel sectionLabel(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer(), "DrawImGui");

    // Barrier all ImGui referenced images for graphics sampled usage, so ImGui shaders/draws can sample from them
    for (const auto& image : referencedImages)
    {
        m_images->BarrierWholeImageForUsage(*commandBuffer, image, ImageUsageMode::GraphicsSampled);
    }

    // Record the ImGui draw commands into the command buffer
    ImGui_ImplVulkan_RenderDrawData(pDrawData, (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer(), VK_NULL_HANDLE);

    // Barrier all ImGui referenced images back to default usage
    for (const auto& image : referencedImages)
    {
        m_images->BarrierWholeImageToDefaultUsage(*commandBuffer, image, ImageUsageMode::GraphicsSampled);
    }

    return true;
}

std::optional<ImTextureID> WiredGPUVkImpl::CreateImGuiImageReference(ImageId imageId, SamplerId samplerId)
{
    // GetNextFrame, not GetCurrentFrame, since references are created in advance of a frame being started, so
    // associate any references with the next frame, not the current frame
    return m_frames->GetNextFrame().CreateImGuiImageReference(imageId, samplerId);
}
#endif

bool WiredGPUVkImpl::CmdBindUniformData(RenderOrComputePass pass, const std::string& bindPoint, const void *pData, const std::size_t& byteSize)
{
    //
    // Fetch Data
    //
    const auto commandBufferId = std::visit([](auto&& obj) -> decltype(auto) { return obj.commandBufferId; }, pass);

    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindUniformData: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass() && !(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindUniformData: Only allowed inside a render or compute pass");
        return false;
    }

    if (byteSize > UNIFORM_BUFFER_BYTE_SIZE)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindUniformData: Max uniform byte size is: {}", UNIFORM_BUFFER_BYTE_SIZE);
        return false;
    }

    const auto& passState = (*commandBuffer)->GetPassState();

    if (!passState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindUniformData: Can't bind data without a bound pipeline");
        return false;
    }

    //
    // Execute
    //

    // Ask uniform buffers system for an unused uniform buffer
    const auto dynamicUniformBuffer = m_uniformBuffers->GetFreeUniformBuffer();
    if (!dynamicUniformBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindUniformData: Failed to get free uniform buffer");
        return false;
    }

    // Write the uniform data to the buffer
    const auto pBufferData = m_buffers->MapBuffer(dynamicUniformBuffer->bufferId, false);
        memcpy((std::byte*)*pBufferData + (dynamicUniformBuffer->byteOffset), pData, byteSize);
    m_buffers->UnmapBuffer(dynamicUniformBuffer->bufferId);

    // Tell the active command buffer to bind the uniform buffer
    return (*commandBuffer)->BindBuffer(
        bindPoint,
        VkBufferBinding{
            .gpuBuffer = *m_buffers->GetBuffer(dynamicUniformBuffer->bufferId, false),
            .vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .shaderWriteable = false,
            .byteOffset = 0,
            .byteSize = byteSize,
            .dynamicByteOffset = (uint32_t)dynamicUniformBuffer->byteOffset
        }
    );
}

bool WiredGPUVkImpl::CmdBindStorageReadBuffer(RenderOrComputePass pass, const std::string& bindPoint, BufferId bufferId)
{
    //
    // Fetch Data
    //
    const auto commandBufferId = std::visit([](auto&& obj) -> decltype(auto) { return obj.commandBufferId; }, pass);

    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadBuffer: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    const auto gpuBuffer = m_buffers->GetBuffer(bufferId, false);
    if (!gpuBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadBuffer: No such buffer exists: {}", bufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass() && !(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadBuffer: Only allowed inside a render or compute pass");
        return false;
    }

    const auto& passState = (*commandBuffer)->GetPassState();

    if (!passState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadBuffer: Can't bind storage buffer without a bound pipeline");
        return false;
    }

    //
    // Execute
    //
    return (*commandBuffer)->BindBuffer(
        bindPoint,
        VkBufferBinding{
            .gpuBuffer = *gpuBuffer,
            .vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .shaderWriteable = false,
            .byteOffset = 0,
            .byteSize = gpuBuffer->bufferDef.byteSize,
            .dynamicByteOffset = std::nullopt }
    );
}

bool WiredGPUVkImpl::CmdBindStorageReadWriteBuffer(RenderOrComputePass pass, const std::string& bindPoint, BufferId bufferId)
{
    //
    // Fetch Data
    //
    const auto commandBufferId = std::visit([](auto&& obj) -> decltype(auto) { return obj.commandBufferId; }, pass);

    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteBuffer: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    const auto gpuBuffer = m_buffers->GetBuffer(bufferId, false);
    if (!gpuBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteBuffer: No such buffer exists: {}", bufferId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteBuffer: Only allowed inside a compute pass");
        return false;
    }

    const auto& passState = (*commandBuffer)->GetPassState();

    if (!passState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteBuffer: Can't bind storage buffer without a bound pipeline");
        return false;
    }

    //
    // Execute
    //
    return (*commandBuffer)->BindBuffer(
        bindPoint,
        VkBufferBinding{
            .gpuBuffer = *gpuBuffer,
            .vkDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .shaderWriteable = true,
            .byteOffset = 0,
            .byteSize = gpuBuffer->bufferDef.byteSize,
            .dynamicByteOffset = std::nullopt }
    );
}

bool WiredGPUVkImpl::CmdBindImageViewSampler(RenderOrComputePass pass, const std::string& bindPoint, uint32_t arrayIndex, ImageId imageId, SamplerId samplerId)
{
    //
    // Fetch Data
    //
    const auto commandBufferId = std::visit([](auto&& obj) -> decltype(auto) { return obj.commandBufferId; }, pass);

    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindImageViewSampler: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    const auto gpuImage = m_images->GetImage(imageId, false);
    if (!gpuImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindImageViewSampler: No such image exists: {}", imageId.id);
        return false;
    }

    const auto sampler = m_samplers->GetSampler(samplerId);
    if (!sampler)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindImageViewSampler: No such sampler exists: {}", samplerId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass() && !(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindImageViewSampler: Only allowed inside a render pass");
        return false;
    }

    const auto& renderPassState = (*commandBuffer)->GetPassState();

    if (!renderPassState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindImageViewSampler: Can't bind image sampler without a bound pipeline");
        return false;
    }

    //
    // Execute
    //
    return (*commandBuffer)->BindImageViewSampler(
        bindPoint,
        arrayIndex,
        VkImageViewSamplerBinding{
            .gpuImage = *gpuImage,
            .imageViewIndex = 0,
            .vkSampler = sampler->GetVkSampler()
        }
    );
}

bool WiredGPUVkImpl::CmdBindStorageReadImage(RenderOrComputePass pass, const std::string& bindPoint, ImageId imageId)
{
    //
    // Fetch Data
    //
    const auto commandBufferId = std::visit([](auto&& obj) -> decltype(auto) { return obj.commandBufferId; }, pass);

    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadImage: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    const auto gpuImage = m_images->GetImage(imageId, false);
    if (!gpuImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadImage: No such image exists: {}", imageId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass() && !(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadImage: Only allowed inside a render pass");
        return false;
    }

    const auto& renderPassState = (*commandBuffer)->GetPassState();

    if (!renderPassState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadImage: Can't bind image sampler without a bound pipeline");
        return false;
    }

    //
    // Execute
    //
    return (*commandBuffer)->BindImageView(
        bindPoint,
        VkImageViewBinding{
            .gpuImage = *gpuImage,
            .imageViewIndex = 0,
            .shaderWriteable = false
        }
    );
}

bool WiredGPUVkImpl::CmdBindStorageReadWriteImage(RenderOrComputePass pass, const std::string& bindPoint, ImageId imageId)
{
    //
    // Fetch Data
    //
    const auto commandBufferId = std::visit([](auto&& obj) -> decltype(auto) { return obj.commandBufferId; }, pass);

    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteImage: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    const auto gpuImage = m_images->GetImage(imageId, false);
    if (!gpuImage)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteImage: No such image exists: {}", imageId.id);
        return false;
    }

    //
    // Validate
    //
    if (!(*commandBuffer)->IsInRenderPass() && !(*commandBuffer)->IsInComputePass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteImage: Only allowed inside a render pass");
        return false;
    }

    const auto& renderPassState = (*commandBuffer)->GetPassState();

    if (!renderPassState->boundPipeline)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdBindStorageReadWriteImage: Can't bind image sampler without a bound pipeline");
        return false;
    }

    //
    // Execute
    //
    return (*commandBuffer)->BindImageView(
        bindPoint,
        VkImageViewBinding{
            .gpuImage = *gpuImage,
            .imageViewIndex = 0,
            .shaderWriteable = true
        }
    );
}

void WiredGPUVkImpl::CmdPushDebugSection(CommandBufferId commandBufferId, const std::string& sectionName)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdPushDebugSection: No such command buffer exists: {}", commandBufferId.id);
        return;
    }

    //
    // Execute
    //
    BeginCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer(), sectionName);
}

void WiredGPUVkImpl::CmdPopDebugSection(CommandBufferId commandBufferId)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdPopDebugSection: No such command buffer exists: {}", commandBufferId.id);
        return;
    }

    //
    // Execute
    //
    EndCommandBufferSection(m_global.get(), (*commandBuffer)->GetVulkanCommandBuffer().GetVkCommandBuffer());
}

bool WiredGPUVkImpl::HasTimestampSupport() const
{
    return m_frames->GetCurrentFrame().GetTimestamps().has_value();
}

void WiredGPUVkImpl::SyncDownFrameTimestamps()
{
    //
    // Fetch Data
    //
    const auto timestamps = m_frames->GetCurrentFrame().GetTimestamps();
    if (!timestamps)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::SyncDownFrameTimestamps: Frame doesn't have timestamps support");
        return;
    }

    //
    // Execute
    //
    (*timestamps)->SyncDownTimestamps();
}

void WiredGPUVkImpl::ResetFrameTimestampsForRecording(CommandBufferId commandBufferId)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::ResetFrameTimestampsForRecording: No such command buffer exists: {}", commandBufferId.id);
        return;
    }

    const auto timestamps = m_frames->GetCurrentFrame().GetTimestamps();
    if (!timestamps)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::ResetFrameTimestampsForRecording: Frame doesn't have timestamps support");
        return;
    }

    //
    // Execute
    //
    (*timestamps)->ResetForRecording(*commandBuffer);
}

void WiredGPUVkImpl::CmdWriteTimestampStart(CommandBufferId commandBufferId, const std::string& name)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdWriteTimestampStart: No such command buffer exists: {}", commandBufferId.id);
        return;
    }

    const auto timestamps = m_frames->GetCurrentFrame().GetTimestamps();
    if (!timestamps)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdWriteTimestampStart: Frame doesn't have timestamps support");
        return;
    }

    //
    // Execute
    //
    (*timestamps)->WriteTimestampStart(*commandBuffer, name, 1); // TODO Multiview: If in multi-viewed render pass, provide multiview count
}

void WiredGPUVkImpl::CmdWriteTimestampFinish(CommandBufferId commandBufferId, const std::string& name)
{
    //
    // Fetch Data
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdWriteTimestampFinish: No such command buffer exists: {}", commandBufferId.id);
        return;
    }

    const auto timestamps = m_frames->GetCurrentFrame().GetTimestamps();
    if (!timestamps)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CmdWriteTimestampFinish: Frame doesn't have timestamps support");
        return;
    }

    //
    // Execute
    //
    (*timestamps)->WriteTimestampFinish(*commandBuffer, name);
}

std::optional<float> WiredGPUVkImpl::GetTimestampDiffMs(const std::string& name, uint32_t offset) const
{
    const auto timestamps = m_frames->GetCurrentFrame().GetTimestamps();
    if (!timestamps)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::GetTimestampDiffMs: Frame doesn't have timestamps support");
        return std::nullopt;
    }

    return (*timestamps)->GetTimestampDiffMs(name, offset);
}

std::expected<ImageId, SurfaceError> WiredGPUVkImpl::AcquireSwapChainImage(CommandBufferId commandBufferId)
{
    // Can't acquire a swap chain image if we're running in headless mode and don't have a swap chain
    if (!m_global->swapChain)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::AcquireSwapChainImage: Can't call AcquireSwapChainImage() in headless mode");
        return std::unexpected(SurfaceError::Other);
    }

    auto& currentFrame = m_frames->GetCurrentFrame();

    if (!currentFrame.IsActiveState())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::AcquireSwapChainImage: A frame must be started");
        return std::unexpected(SurfaceError::Other);
    }

    //
    // Fetch the specified command buffer
    //
    const auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::AcquireSwapChainImage: No such command buffer exists: {}", commandBufferId.id);
        return std::unexpected(SurfaceError::Other);
    }

    if ((*commandBuffer)->GetType() != CommandBufferType::Primary)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::AcquireSwapChainImage: Command buffer must be a primary command buffer: {}", commandBufferId.id);
        return std::unexpected(SurfaceError::Other);
    }

    const auto& vulkanCommandBuffer = (*commandBuffer)->GetVulkanCommandBuffer();
    const auto vkImageAvailableSemaphore = currentFrame.GetSwapChainImageAvailableSemaphore();
    const auto vkPresentWorkFinishedSemaphore = currentFrame.GetPresentWorkFinishedSemaphore();

    //
    // Configure the command buffer for doing presentation work when it's submitted
    //
    (*commandBuffer)->ConfigureForPresentation(
        // Wait on the swap chain image available semaphore
        SemaphoreOp(vkImageAvailableSemaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT),
        // Signal on the present work finished semaphore
        SemaphoreOp(vkPresentWorkFinishedSemaphore, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
    );

    //
    // Acquire the next swap chain image index. May block.
    //
    uint32_t swapChainImageIndex{0};

    const auto result = m_global->vk.vkAcquireNextImageKHR(
        m_global->device.GetVkDevice(),
        m_global->swapChain->GetVkSwapChain(),
        UINT64_MAX,
        vkImageAvailableSemaphore,
        VK_NULL_HANDLE,
        &swapChainImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        m_global->pLogger->Info("WiredGPUVkImpl::AcquireSwapChainImage: vkAcquireNextImageKHR() reports swap chain is out of date or suboptimal");
        return std::unexpected(SurfaceError::SurfaceInvalidated);
    }
    else if (result == VK_ERROR_SURFACE_LOST_KHR)
    {
        m_global->pLogger->Info("WiredGPUVkImpl::AcquireSwapChainImage: vkAcquireNextImageKHR() reports surface has been lost");
        return std::unexpected(SurfaceError::SurfaceLost);
    }
    else if (result != VK_SUCCESS)
    {
        m_global->pLogger->Info("WiredGPUVkImpl::AcquireSwapChainImage: vkAcquireNextImageKHR() other error");
        return std::unexpected(SurfaceError::Other);
    }

    //
    // From the index returned, look up the Image that wraps it
    //
    const auto swapChainImageId = m_global->swapChain->GetImageId(swapChainImageIndex);

    const auto swapChainImage = m_images->GetImage(swapChainImageId, false);
    if (!swapChainImage)
    {
        m_global->pLogger->Info("WiredGPUVkImpl::AcquireSwapChainImage: Failed to fetch swap chain image");
        return std::unexpected(SurfaceError::Other);
    }

    //
    // Store the swap chain image index we retrieved in the frame state, to remember it for later presentation
    //
    currentFrame.SetSwapChainPresentIndex(swapChainImageIndex);

    //
    // Transition the swap chain image from Undefined to ColorAttachmentOptimal layout, which matches the
    // default usage that the Images system records for it. Note that this is a raw/custom barrier since
    // the srcStageMask has to form a dependency chain with the wait semaphore stage mask used when the
    // command buffer is submitted, which the normal barrier system wouldn't do.
    //
    vulkanCommandBuffer.CmdPipelineBarrier2(Barrier{
        .imageBarriers = {
            ImageBarrier{
                .vkImage = swapChainImage->imageData.vkImage,
                .subresourceRange = OneLayerOneMipColorResource,
                .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = VK_ACCESS_2_NONE,
                .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
        },
        .bufferBarriers = {}
    });

    return swapChainImageId;
}

std::expected<bool, SurfaceError> WiredGPUVkImpl::SubmitCommandBuffer(CommandBufferId commandBufferId)
{
    auto& currentFrame = m_frames->GetCurrentFrame();

    //
    // Find the specified command buffer
    //
    auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::SubmitCommandBuffer: No such command buffer exists: {}", commandBufferId.id);
        return false;
    }

    if ((*commandBuffer)->GetType() != CommandBufferType::Primary)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::SubmitCommandBuffer: Can only submit primary command buffers: {}", commandBufferId.id);
        return false;
    }

    if ((*commandBuffer)->IsInAnyPass())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::SubmitCommandBuffer: Command buffer is in an open pass: {}", commandBufferId.id);
        return false;
    }

    if ((*commandBuffer)->IsConfiguredForPresentation() && !currentFrame.IsActiveState())
    {
        m_global->pLogger->Error("WiredGPUVkImpl::SubmitCommandBuffer: Submitting for presentation requires an active frame");
        return false;
    }

    auto& vulkanCommandBuffer = (*commandBuffer)->GetVulkanCommandBuffer();

    //
    // If configured for presentation, transition the swap chain image to present src layout as the last
    // command in the command buffer
    //
    if ((*commandBuffer)->IsConfiguredForPresentation())
    {
        const auto swapChainImageId = m_global->swapChain->GetImageId(currentFrame.GetSwapChainPresentIndex());

        const auto swapChainGPUImage = m_images->GetImage(swapChainImageId, false);
        if (!swapChainGPUImage)
        {
            m_global->pLogger->Error("WiredGPUVkImpl::SubmitCommandBuffer: Swap chain image doesn't exist: {}", swapChainImageId.id);
            return false;
        }

        m_images->BarrierWholeImageForUsage(*commandBuffer, *swapChainGPUImage, ImageUsageMode::PresentSrc);
    }

    //
    // End the command buffer's recording
    //
    vulkanCommandBuffer.End();

    //
    // Submit the command buffer
    //
    if (!m_global->commandQueue.SubmitBatch(
        {vulkanCommandBuffer},
        WaitOn((*commandBuffer)->GetWaitSemaphores()),
        SignalOn((*commandBuffer)->GetSignalSemaphores()),
        (*commandBuffer)->GetVkFence(),
        (*commandBuffer)->GetTag()))
    {
        m_global->pLogger->Error("WiredGPUVkImpl::SubmitCommandBuffer: Failed to submit command buffer: {}", commandBufferId.id);
        return false;
    }

    //
    // If configured for presentation, present the swap chain image now that we've
    // submitted all the work for the frame
    //
    if ((*commandBuffer)->IsConfiguredForPresentation())
    {
        const auto result = PresentSwapChainImage(currentFrame.GetSwapChainPresentIndex(), currentFrame.GetPresentWorkFinishedSemaphore());
        if (!result)
        {
            return std::unexpected(result.error());
        }
    }

    return true;
}

void WiredGPUVkImpl::CancelCommandBuffer(CommandBufferId commandBufferId)
{
    auto commandBuffer = m_commandBuffers->GetCommandBuffer(commandBufferId);
    if (!commandBuffer)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::CancelCommandBuffer: No such command buffer exists: {}", commandBufferId.id);
        return;
    }

    // No longer have the command buffer reference its resources
    (*commandBuffer)->ReleaseTrackedResources();

    // If the active frame is associated with the command buffer ... un-associate it
    auto& currentFrame = m_frames->GetCurrentFrame();
    if (currentFrame.IsActiveState())
    {
        currentFrame.UnAssociateCommandBuffer(commandBufferId);
    }

    // Destroy the command buffer
    m_commandBuffers->DestroyCommandBuffer(commandBufferId);
}

std::expected<VulkanCommandPool*, bool> WiredGPUVkImpl::EnsureThreadCommandPool()
{
    //
    // Returns the thread pool associated with the current thread, or creates one
    // if none exists
    //

    std::lock_guard<std::mutex> lock(m_commandPoolsMutex);

    const auto threadId = std::this_thread::get_id();
    const auto threadIdHash = std::hash<std::thread::id>{}(threadId);

    const auto it = m_commandPools.find(threadId);
    if (it != m_commandPools.cend())
    {
        return it->second.get();
    }

    const auto commandPoolExpect = VulkanCommandPool::Create(m_global.get(), m_global->commandQueue.GetQueueFamilyIndex(), 0, std::format("{}", threadIdHash));
    if (!commandPoolExpect)
    {
        m_global->pLogger->Error("WiredGPUVkImpl::EnsureThreadCommandPool: Failed to create command pool for thread");
        return std::unexpected(false);
    }

    auto commandPool = std::make_unique<VulkanCommandPool>(*commandPoolExpect);
    auto pCommandPool = commandPool.get();

    m_commandPools.insert({threadId, std::move(commandPool)});

    return pCommandPool;
}

std::expected<DescriptorSets*, bool> WiredGPUVkImpl::EnsureThreadDescriptorSets()
{
    //
    // Returns the descriptor sets associated with the current thread, or creates one
    // if none exists
    //

    std::lock_guard<std::mutex> lock(m_descriptorSetsMutex);

    const auto threadId = std::this_thread::get_id();
    const auto threadIdHash = std::hash<std::thread::id>{}(threadId);

    const auto it = m_descriptorSets.find(threadId);
    if (it != m_descriptorSets.cend())
    {
        return it->second.get();
    }

    auto descriptorSets = std::make_unique<DescriptorSets>(m_global.get(), std::format("{}", threadIdHash));
    const auto pDescriptorSets = descriptorSets.get();

    m_descriptorSets.insert({threadId, std::move(descriptorSets)});

    return pDescriptorSets;
}

std::expected<bool, SurfaceError> WiredGPUVkImpl::PresentSwapChainImage(uint32_t swapChainImageIndex, VkSemaphore waitSemaphore)
{
    QueueSectionLabel submitSection(m_global.get(), m_global->presentQueue->GetVkQueue(), "Present");

    VkSwapchainKHR swapChain = m_global->swapChain->GetVkSwapChain();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain;
    presentInfo.pImageIndices = &swapChainImageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;

    const auto result = m_global->vk.vkQueuePresentKHR(m_global->presentQueue->GetVkQueue(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        m_global->pLogger->Info("WiredGPUVkImpl::PresentSwapChainImage: vkQueuePresentKHR() reports swap chain is out of date or suboptimal");
        return std::unexpected(SurfaceError::SurfaceInvalidated);
    }
    else if (result == VK_ERROR_SURFACE_LOST_KHR)
    {
        m_global->pLogger->Info("WiredGPUVkImpl::PresentSwapChainImage: vkQueuePresentKHR() reports surface has been lost");
        return std::unexpected(SurfaceError::SurfaceLost);
    }
    else if (result != VK_SUCCESS)
    {
        m_global->pLogger->Info("WiredGPUVkImpl::PresentSwapChainImage: vkQueuePresentKHR() other error");
        return std::unexpected(SurfaceError::Other);
    }

    return true;
}

void WiredGPUVkImpl::RunCleanUp(bool isIdleCleanUp)
{
    // Destroys finished command buffers and un-references their resources
    m_commandBuffers->RunCleanUp();

    // Destroys resources marked for destroy that are no longer referenced
    // by a command buffer (also other various cleanup tasks)
    m_images->RunCleanUp();
    m_buffers->RunCleanUp();
    m_samplers->RunCleanUp();
    m_pipelines->RunCleanUp();
    m_shaders->RunCleanUp();
    {
        std::lock_guard<std::mutex> lock(m_descriptorSetsMutex);
        for (auto& descriptorSets : m_descriptorSets)
        {
            descriptorSets.second->RunCleanUp(isIdleCleanUp);
        }
    }
    m_uniformBuffers->RunCleanUp();

    // Erases the usage tracking for resources which no longer have any references
    m_usages->ForgetZeroUsageItems();
}

}
