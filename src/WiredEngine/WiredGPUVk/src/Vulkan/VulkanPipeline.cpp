/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanPipeline.h"
#include "VulkanDebugUtil.h"
#include "VulkanDescriptorSetLayout.h"

#include "../Global.h"

#include "../Shader/Shaders.h"
#include "../Pipeline/Layouts.h"
#include "../Util/SPVUtil.h"
#include "../Util/VulkanUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <algorithm>
#include <cassert>
#include <unordered_set>
#include <array>
#include <optional>

namespace Wired::GPU
{

std::optional<SpvReflectDescriptorSet> GetModuleReflectDescriptorSet(const SpvReflectShaderModule& module, uint32_t set)
{
    for (uint32_t x = 0; x < module.descriptor_set_count; ++x)
    {
        if (module.descriptor_sets[x].set == set)
        {
            return module.descriptor_sets[x];
        }
    }

    return std::nullopt;
}

std::expected<VulkanDescriptorSetLayout, bool> GetOrCreateDescriptorSetLayout(Global* pGlobal,
                                                                              const std::vector<VulkanShaderModule*>& shaderModules,
                                                                              uint32_t set,
                                                                              const std::string& tag)
{
    // Map of descriptor set binding index to the spv reflection details of that binding index
    std::unordered_map<uint32_t, SpvReflectDescriptorBinding> setBindingReflectInfos;

    // Records which shader module stages include this descriptor set
    VkShaderStageFlags moduleSetUsagesFlags{0};

    //
    // Loop through the modules and compile information about how they use the descriptor set
    //
    for (const auto& module : shaderModules)
    {
        // Get the reflection info of this module's usage of the descriptor set, if any
        const auto reflectDescriptorSetOpt = GetModuleReflectDescriptorSet(module->GetSpvReflectInfo(), set);
        if (reflectDescriptorSetOpt.has_value())
        {
            // Mark this module as using this descriptor set
            moduleSetUsagesFlags = moduleSetUsagesFlags | SpvToVkShaderStageFlags(module->GetSpvReflectInfo().shader_stage).value();

            // Save the detail's of the descriptor set's bindings for later usage. Note that we're assuming
            // that any module that uses this descriptor set is required to use all the same bindings as
            // other modules.
            for (uint32_t x = 0; x < reflectDescriptorSetOpt->binding_count; ++x)
            {
                const auto setBinding = reflectDescriptorSetOpt->bindings[x];

                const auto it = setBindingReflectInfos.find(setBinding->binding);
                if (it == setBindingReflectInfos.cend())
                {
                    setBindingReflectInfos.insert(std::make_pair(setBinding->binding, *setBinding));
                }
            }
        }
    }

    //
    // Generate details about the descriptor set's bindings
    //
    std::vector<DescriptorSetLayoutBinding> bindings;

    std::ranges::transform(setBindingReflectInfos, std::back_inserter(bindings), [&](const auto& spvBindingInfo) {
        // From reflection there's no way to know whether a descriptor set layout binding for a uniform buffer
        // should be configured as a normal or dynamic uniform binding. At the moment the GPUVk uses dynamic
        // uniforms everywhere, so treat all uniform bindings as dynamic uniform bindings.
        //
        // If in the future we need to support both, try something like a unique naming of each bind point, e.g.
        // u_Buffer vs u_dyn_Buffer, or maybe it needs to be specified code-side and passed in as a vector
        // of bind points which should be dynamic.
        auto vkDescriptorType = SpvToVkDescriptorType(spvBindingInfo.second.descriptor_type).value();
        if (vkDescriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            vkDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        }

        return DescriptorSetLayoutBinding{
            .bindPoint = spvBindingInfo.second.name,
            .set = spvBindingInfo.second.set,
            .vkDescriptorSetLayoutBinding = VkDescriptorSetLayoutBinding{
                .binding = spvBindingInfo.second.binding,
                .descriptorType = vkDescriptorType,
                .descriptorCount =  spvBindingInfo.second.count,
                .stageFlags = moduleSetUsagesFlags,
                .pImmutableSamplers = nullptr
            }
        };
    });

    const auto descriptorSetLayout = pGlobal->pLayouts->GetOrCreateDescriptorSetLayout(bindings, tag);
    if (!descriptorSetLayout)
    {
        return std::unexpected(false);
    }

    return *descriptorSetLayout;
}

std::expected<std::array<VulkanDescriptorSetLayout, 4>, bool> GetOrCreateDescriptorSetLayouts(Global* pGlobal,
                                                                                              const std::vector<VulkanShaderModule*>& shaderModules,
                                                                                              const std::string& tag)
{
    std::array<VulkanDescriptorSetLayout, 4> descriptorSetLayouts;

    //
    // Compile the set of unique descriptor set indices that exist across all the shader modules
    //
    std::unordered_set<uint32_t> uniqueDescriptorSets;

    for (const auto& module : shaderModules)
    {
        const SpvReflectShaderModule reflectInfo = module->GetSpvReflectInfo();

        for (uint32_t x = 0; x < reflectInfo.descriptor_set_count; ++x)
        {
            uniqueDescriptorSets.insert(reflectInfo.descriptor_sets[x].set);
        }
    }

    //
    // All shaders use up to 4 descriptor sets. Create a descriptor set layout which represents
    // the shaders' usage of each set. If the combination of shaders doesn't make use of a given
    // set, an empty/stub descriptor set layout is created for that set index.
    //
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    for (unsigned int set = 0; set < 4; ++set)
    {
        std::expected<VulkanDescriptorSetLayout, bool> descriptorSetLayout;

        if (uniqueDescriptorSets.contains(set))
        {
            descriptorSetLayout = GetOrCreateDescriptorSetLayout(pGlobal, shaderModules, set, std::format("{}-{}", tag, set));
        }
        else
        {
            descriptorSetLayout = GetOrCreateDescriptorSetLayout(pGlobal, {}, set, std::format("{}-stub", tag));
        }

        if (!descriptorSetLayout)
        {
            pGlobal->pLogger->Error("CreateDescriptorSetLayouts: Failed to create descriptor set layout: {} for: {}", set, tag);
            return std::unexpected(false);
        }
        descriptorSetLayouts[set] = *descriptorSetLayout;
    }

    return descriptorSetLayouts;
}

std::optional<std::pair<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>>
GetModuleVertexInputDescriptions(const SpvReflectShaderModule& module)
{
    // Only look at vertex shaders for input attributes
    if (module.shader_stage != SPV_REFLECT_SHADER_STAGE_VERTEX_BIT)
    {
        return std::nullopt;
    }

    uint32_t count = 0;
    spvReflectEnumerateInputVariables(&module, &count, nullptr);

    std::vector<SpvReflectInterfaceVariable*> input_vars(count);
    spvReflectEnumerateInputVariables(&module, &count, input_vars.data());

    VkVertexInputBindingDescription binding_description{};
    binding_description.binding = 0;
    binding_description.stride = 0;  // computed below
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    attribute_descriptions.reserve(input_vars.size());

    for (const auto& input_var : input_vars)
    {
        const SpvReflectInterfaceVariable& refl_var = *input_var;

        // Skip over builtin variables like gl_InstanceId
        if (refl_var.decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN)
        {
            continue;
        }

        VkVertexInputAttributeDescription attr_desc{};
        attr_desc.location = refl_var.location;
        attr_desc.binding = binding_description.binding;
        attr_desc.format = static_cast<VkFormat>(refl_var.format);
        attr_desc.offset = 0;  // final offset computed below after sorting.
        attribute_descriptions.push_back(attr_desc);
    }

    // Sort attributes by location
    std::sort(std::begin(attribute_descriptions), std::end(attribute_descriptions),
      [](const VkVertexInputAttributeDescription& a, const VkVertexInputAttributeDescription& b) {
          return a.location < b.location;
      });

    // Compute final offsets of each attribute, and total vertex stride.

    for (auto& attribute : attribute_descriptions)
    {
        attribute.offset = binding_description.stride;
        binding_description.stride += GetVkFormatByteSize(attribute.format);
    }

    return std::make_pair(attribute_descriptions, binding_description);
}

std::optional<std::pair<std::vector<VkVertexInputAttributeDescription>, VkVertexInputBindingDescription>>
GenerateVertexInputDescriptions(const std::vector<VulkanShaderModule*>& shaderModules)
{
    for (const auto& module : shaderModules)
    {
        const auto vertexInputAttributeDescriptions = GetModuleVertexInputDescriptions(module->GetSpvReflectInfo());
        if (vertexInputAttributeDescriptions.has_value())
        {
            return *vertexInputAttributeDescriptions;
        }
    }

    return std::nullopt;
}

std::expected<VkPipelineLayout, bool> GetOrCreateGraphicsPipelineLayout(Global* pGlobal,
                                                                        const VkGraphicsPipelineConfig& config,
                                                                        const std::array<VulkanDescriptorSetLayout, 4>& descriptorSetLayouts,
                                                                        const std::string& tag)
{
    std::array<VkDescriptorSetLayout, 4> vkDescriptorSetLayouts{};
    for (unsigned int x = 0; x < 4; x++)
    {
        vkDescriptorSetLayouts[x] = descriptorSetLayouts[x].GetVkDescriptorSetLayout();
    }

    std::vector<VkPushConstantRange> pushConstantRanges;
    if (config.vkPushConstantRanges) { pushConstantRanges = *config.vkPushConstantRanges; }

    const auto vkPipelineLayout = pGlobal->pLayouts->GetOrCreatePipelineLayout(vkDescriptorSetLayouts, pushConstantRanges, tag);
    if (!vkPipelineLayout)
    {
        pGlobal->pLogger->Error("CreatePipelineLayout: Call to GetOrCreatePipelineLayout() failed");
        return std::unexpected(false);
    }

    return *vkPipelineLayout;
}

std::expected<VkPipeline, bool> CreateGraphicsPipeline(Global* pGlobal,
                                                       const VkGraphicsPipelineConfig& config,
                                                       const std::vector<VulkanShaderModule*>& shaderModules,
                                                       VkPipelineLayout vkPipelineLayout)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    for (const auto& shaderModule : shaderModules)
    {
        VkPipelineShaderStageCreateInfo shaderStageInfo{};
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.module = shaderModule->GetVkShaderModule();
        shaderStageInfo.pName = shaderModule->GetSpvReflectInfo().entry_point_name;

        switch (shaderModule->GetShaderSpec().shaderType)
        {
            case ShaderType::Vertex: shaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; break;
            case ShaderType::Fragment: shaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
            case ShaderType::Compute: shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT; break;
        }

        shaderStages.push_back(shaderStageInfo);
    }

    //
    // Depth buffer configuration
    //
    VkPipelineDepthStencilStateCreateInfo depthStencil{};

    if (config.depthAttachment)
    {
        // Note reversed z-axis for depth attachment
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = config.depthTestEnabled;
        depthStencil.depthWriteEnable = config.depthWriteEnabled;
        depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL; // reversed
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};
    }

    //
    // Dynamic rendering configuration
    //
    std::vector<VkFormat> colorAttachmentFormats;

    std::ranges::transform(config.colorAttachments, std::back_inserter(colorAttachmentFormats), [](const auto& colorAttachment) {
        return colorAttachment.vkFormat;
    });

    VkFormat vkDepthAttachmentFormat{VK_FORMAT_UNDEFINED};
    if (config.depthAttachment)
    {
        vkDepthAttachmentFormat = config.depthAttachment->vkFormat;
    }

    VkFormat vkStencilAttachmentFormat{VK_FORMAT_UNDEFINED};

    VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo{};
    pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount    = (uint32_t)colorAttachmentFormats.size();
    pipelineRenderingCreateInfo.pColorAttachmentFormats = colorAttachmentFormats.data();
    pipelineRenderingCreateInfo.depthAttachmentFormat   = vkDepthAttachmentFormat;
    pipelineRenderingCreateInfo.stencilAttachmentFormat = vkStencilAttachmentFormat;

    //
    // Configure vertex input state
    //
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
    std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions;

    const auto vertexInputDescriptions = GenerateVertexInputDescriptions(shaderModules);
    if (vertexInputDescriptions.has_value())
    {
        vertexInputAttributeDescriptions = vertexInputDescriptions->first;
        vertexInputBindingDescriptions.push_back(vertexInputDescriptions->second);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // Vertex attribute descriptions
    if (!vertexInputAttributeDescriptions.empty())
    {
        vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)vertexInputAttributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
    }
    else
    {
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    }

    // Vertex binding description
    if (!vertexInputBindingDescriptions.empty())
    {
        vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)vertexInputBindingDescriptions.size();
        vertexInputInfo.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
    }
    else
    {
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
    }

    //
    // Configure vertex assembly stage
    //
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    if (config.primitiveRestartEnable)
    {
        inputAssembly.primitiveRestartEnable = VK_TRUE;
    }
    else
    {
        inputAssembly.primitiveRestartEnable = VK_FALSE;
    }

    switch (config.primitiveTopology)
    {
        case PrimitiveTopology::TriangleList: inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
        case PrimitiveTopology::TriangleFan: inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
        case PrimitiveTopology::PatchList: inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; break;
    }

    //
    // Configure viewport/scissoring state
    //
    // Note: y and height are adjusted; using maintenance1 to flip the y-axis
    //
    VkViewport viewport{};
    viewport.x = (float)config.viewport.x;
    viewport.y = (float)config.viewport.h - (float)config.viewport.y;
    viewport.width = (float)config.viewport.w;
    viewport.height = (float)config.viewport.h * -1.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { (int32_t)config.viewport.x, (int32_t)config.viewport.y };
    scissor.extent = {config.viewport.w, config.viewport.h };

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    //
    // Configure rasterizer stage
    //
    VkCullModeFlags vkCullModeFlags{};

    switch (config.cullFace)
    {
        case CullFace::None:
            vkCullModeFlags = VK_CULL_MODE_NONE;
        break;
        case CullFace::Front:
            vkCullModeFlags = VK_CULL_MODE_FRONT_BIT;
        break;
        case CullFace::Back:
            vkCullModeFlags = VK_CULL_MODE_BACK_BIT;
        break;
    }

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vkCullModeFlags;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    if (config.depthBias == DepthBias::Enabled)
    {
        // From: https://blogs.igalia.com/itoral/2017/10/02/working-with-lights-and-shadows-part-iii-rendering-the-shadows/
        rasterizer.depthBiasEnable = VK_TRUE;
        rasterizer.depthBiasConstantFactor = -2.0f; // Reversed z-axis
        rasterizer.depthBiasSlopeFactor = -1.1f; // Reversed z-axis
        rasterizer.depthBiasClamp = 0.0f;
    }
    else
    {
        rasterizer.depthBiasEnable = VK_FALSE;
    }

    switch (config.polygonFillMode)
    {
        case PolygonFillMode::Fill: rasterizer.polygonMode = VK_POLYGON_MODE_FILL; break;
        case PolygonFillMode::Line: rasterizer.polygonMode = VK_POLYGON_MODE_LINE; break;
    }
    if (rasterizer.polygonMode != VK_POLYGON_MODE_FILL && !pGlobal->physicalDevice.GetPhysicalDeviceFeatures().features.fillModeNonSolid)
    {
        pGlobal->pLogger->Error("CreateGraphicsPipeline: polygonMode != fill, but fillModeNonSolid feature is not enabled, ignoring");
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    }

    //
    // Configure multisampling
    //
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    //
    // Configure color blending
    //
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

    std::ranges::transform(config.colorAttachments, std::back_inserter(colorBlendAttachments),
       [](const auto& colorAttachment){
           VkPipelineColorBlendAttachmentState colorBlendAttachment{};
           colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
           colorBlendAttachment.blendEnable = colorAttachment.enableColorBlending;
           colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
           colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
           colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
           colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
           colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
           colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

           return colorBlendAttachment;
       });

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = (uint32_t)colorBlendAttachments.size();
    colorBlending.pAttachments = colorBlendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    //
    // Configure tesselation
    //
    const bool doesTesselation = config.tescShaderName.has_value() || config.teseShaderName.has_value();

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{};
    tessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellationStateCreateInfo.pNext = nullptr;
    tessellationStateCreateInfo.flags = 0;
    tessellationStateCreateInfo.patchControlPoints = config.tesselationNumControlPoints;

    //
    // Create the pipeline
    //
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &pipelineRenderingCreateInfo;
    pipelineInfo.stageCount = (uint32_t)shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = vkPipelineLayout;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (config.depthAttachment)
    {
        pipelineInfo.pDepthStencilState = &depthStencil;
    }

    if (doesTesselation)
    {
        pipelineInfo.pTessellationState = &tessellationStateCreateInfo;
    }

    VkPipeline vkPipeline{VK_NULL_HANDLE};
    const auto result = pGlobal->vk.vkCreateGraphicsPipelines(pGlobal->device.GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline);
    if (result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("CreatePipeline: Call to vkCreateGraphicsPipelines() failed, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)vkPipeline, std::format("Pipeline-{}", config.GetUniqueKey()));

    return vkPipeline;
}

std::expected<VulkanPipeline, bool> VulkanPipeline::Create(Global* pGlobal, const VkGraphicsPipelineConfig& config)
{
    //
    // Look up shader module data
    //
    std::vector<VulkanShaderModule*> shaderModules;

    const std::vector<std::optional<std::string>> shaderNames = {
        config.vertShaderName,
        config.fragShaderName,
        config.tescShaderName,
        config.teseShaderName
    };

    for (const auto& shaderNameOpt : shaderNames)
    {
        if (!shaderNameOpt) { continue; }

        const auto shaderModuleOpt = pGlobal->pShaders->GetVulkanShaderModule(*shaderNameOpt);
        if (!shaderModuleOpt)
        {
            pGlobal->pLogger->Error("VulkanPipeline::Create: Failed to find pipeline shader: {}", *shaderNameOpt);
            return std::unexpected(false);
        }
        shaderModules.push_back(*shaderModuleOpt);
    }

    std::vector<VkShaderModule> vkShaderModules;
    for (const auto& shaderModule : shaderModules)
    {
        vkShaderModules.push_back(shaderModule->GetVkShaderModule());
    }

    //
    // Create DescriptorSetLayouts for the pipeline
    //
    auto descriptorSetLayouts = GetOrCreateDescriptorSetLayouts(pGlobal, shaderModules, std::format("{}", config.GetUniqueKey()));
    if (!descriptorSetLayouts)
    {
        return std::unexpected(false);
    }

    //
    // Create pipeline layout
    //
    const auto vkPipelineLayout = GetOrCreateGraphicsPipelineLayout(pGlobal, config, *descriptorSetLayouts, std::format("{}", config.GetUniqueKey()));
    if (!vkPipelineLayout)
    {
        for (auto& descriptorSetLayout : *descriptorSetLayouts) { descriptorSetLayout.Destroy(); }
        return std::unexpected(false);
    }

    //
    // Create pipeline
    //
    const auto vkPipeline = CreateGraphicsPipeline(pGlobal, config, shaderModules, *vkPipelineLayout);
    if (!vkPipeline)
    {
        for (auto& descriptorSetLayout : *descriptorSetLayouts){ descriptorSetLayout.Destroy(); }
        pGlobal->vk.vkDestroyPipelineLayout(pGlobal->device.GetVkDevice(), *vkPipelineLayout, nullptr);
        return std::unexpected(false);
    }

    return VulkanPipeline(pGlobal, Type::Graphics, config.GetUniqueKey(), vkShaderModules, *descriptorSetLayouts, *vkPipelineLayout, *vkPipeline);
}

std::expected<VkPipelineLayout, bool> GetOrCreateComputePipelineLayout(Global* pGlobal,
                                                                       const VkComputePipelineConfig& config,
                                                                       const std::array<VulkanDescriptorSetLayout, 4>& descriptorSetLayouts,
                                                                       const std::string& tag)
{
    std::array<VkDescriptorSetLayout, 4> vkDescriptorSetLayouts{};
    for (unsigned int x = 0; x < 4; x++)
    {
        vkDescriptorSetLayouts[x] = descriptorSetLayouts[x].GetVkDescriptorSetLayout();
    }

    std::vector<VkPushConstantRange> pushConstantRanges;
    if (config.vkPushConstantRanges) { pushConstantRanges = *config.vkPushConstantRanges; }

    const auto vkPipelineLayout = pGlobal->pLayouts->GetOrCreatePipelineLayout(vkDescriptorSetLayouts, pushConstantRanges, tag);
    if (!vkPipelineLayout)
    {
        pGlobal->pLogger->Error("CreatePipelineLayout: Call to GetOrCreatePipelineLayout() failed");
        return std::unexpected(false);
    }

    return *vkPipelineLayout;
}

std::expected<VkPipeline, bool> CreateComputePipeline(Global* pGlobal,
                                                      const VkComputePipelineConfig& config,
                                                      VulkanShaderModule* pShaderModule,
                                                      VkPipelineLayout vkPipelineLayout)
{
    //
    // Shader stage configuration
    //
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    if (config.computeShaderFileName.empty())
    {
        pGlobal->pLogger->Error("VulkanPipeline::Create: Compute shader name is empty: {}", config.computeShaderFileName);
        return std::unexpected(false);
    }

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = pShaderModule->GetVkShaderModule();
    shaderStageInfo.pName = pShaderModule->GetSpvReflectInfo().entry_point_name;

    shaderStages.push_back(shaderStageInfo);

    //
    // Create the pipeline
    //
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = vkPipelineLayout;
    pipelineInfo.stage = shaderStages.at(0);
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    VkPipeline vkPipeline{VK_NULL_HANDLE};
    const auto result = pGlobal->vk.vkCreateComputePipelines(pGlobal->device.GetVkDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPipeline);
    if (result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("CreateComputePipeline: Call to vkCreateComputePipelines failed, error code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    SetDebugName(pGlobal->vk, pGlobal->device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)vkPipeline, std::format("Pipeline-{}", config.GetUniqueKey()));

    return vkPipeline;
}

std::expected<VulkanPipeline, bool> VulkanPipeline::Create(Global* pGlobal, const VkComputePipelineConfig& config)
{
    //
    // Fetch shader module data
    //
    const auto shaderModuleOpt = pGlobal->pShaders->GetVulkanShaderModule(config.computeShaderFileName);
    if (!shaderModuleOpt)
    {
        pGlobal->pLogger->Error("VulkanPipeline::Create: No such shader module exists: {}", config.computeShaderFileName);
        return std::unexpected(false);
    }

    const std::vector<VkShaderModule> vkShaderModules = { (*shaderModuleOpt)->GetVkShaderModule() };

    //
    // Create DescriptorSetLayouts for the pipeline
    //
    auto descriptorSetLayouts = GetOrCreateDescriptorSetLayouts(pGlobal, {*shaderModuleOpt}, std::format("{}", config.GetUniqueKey()));
    if (!descriptorSetLayouts)
    {
        return std::unexpected(false);
    }

    //
    // Create the pipeline layout
    //
    const auto vkPipelineLayout = GetOrCreateComputePipelineLayout(pGlobal, config, *descriptorSetLayouts, std::format("{}", config.GetUniqueKey()));
    if (!vkPipelineLayout)
    {
        for (auto& descriptorSetLayout : *descriptorSetLayouts){ descriptorSetLayout.Destroy(); }
        return std::unexpected(false);
    }

    //
    // Create the pipeline
    //
    const auto vkPipeline = CreateComputePipeline(pGlobal, config, *shaderModuleOpt, *vkPipelineLayout);
    if (!vkPipeline)
    {
        for (auto& descriptorSetLayout : *descriptorSetLayouts){ descriptorSetLayout.Destroy(); }
        pGlobal->vk.vkDestroyPipelineLayout(pGlobal->device.GetVkDevice(), *vkPipelineLayout, nullptr);
        return std::unexpected(false);
    }

    return VulkanPipeline(pGlobal, Type::Compute, config.GetUniqueKey(), vkShaderModules, *descriptorSetLayouts, *vkPipelineLayout, *vkPipeline);
}

VulkanPipeline::VulkanPipeline(Global* pGlobal,
                               Type type,
                               const std::size_t& configHash,
                               std::vector<VkShaderModule> vkShaderModules,
                               std::array<VulkanDescriptorSetLayout, 4> descriptorSetLayouts,
                               VkPipelineLayout vkPipelineLayout,
                               VkPipeline vkPipeline)
    : m_pGlobal(pGlobal)
    , m_type(type)
    , m_configHash(configHash)
    , m_vkShaderModules(std::move(vkShaderModules))
    , m_descriptorSetLayouts(std::move(descriptorSetLayouts))
    , m_vkPipelineLayout(vkPipelineLayout)
    , m_vkPipeline(vkPipeline)
{

}

VulkanPipeline::~VulkanPipeline()
{
    m_pGlobal = nullptr;
    m_configHash = {0};
    m_descriptorSetLayouts = {};
    m_vkPipelineLayout = VK_NULL_HANDLE;
    m_vkPipeline = VK_NULL_HANDLE;
}

void VulkanPipeline::Destroy()
{
    // Note that we only destroy the pipeline itself; the pipeline layout and descriptor set layouts
    // are owned by the Layouts system and can outlive this pipeline that uses them

    if (m_vkPipeline != VK_NULL_HANDLE)
    {
        RemoveDebugName(m_pGlobal->vk, m_pGlobal->device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_vkPipeline);
        m_pGlobal->vk.vkDestroyPipeline(m_pGlobal->device.GetVkDevice(), m_vkPipeline, nullptr);
        m_vkPipeline = VK_NULL_HANDLE;
    }

    m_descriptorSetLayouts = {};
    m_vkPipelineLayout = VK_NULL_HANDLE;
    m_configHash = {0};
}

VkPipelineBindPoint VulkanPipeline::GetPipelineBindPoint() const noexcept
{
    switch (m_type)
    {
        case Type::Graphics: return VK_PIPELINE_BIND_POINT_GRAPHICS;
        case Type::Compute: return VK_PIPELINE_BIND_POINT_COMPUTE;
    }

    assert(false);
    return {};
}

std::optional<DescriptorSetLayoutBinding> VulkanPipeline::GetBindingDetails(const std::string& bindPoint) const
{
    for (const auto& descriptorSetLayout : m_descriptorSetLayouts)
    {
        const auto bindingDetails = descriptorSetLayout.GetBindingDetails(bindPoint);
        if (bindingDetails)
        {
            return bindingDetails;
        }
    }

    return std::nullopt;
}

}
