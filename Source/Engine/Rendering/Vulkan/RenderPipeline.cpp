#include "RenderPipeline.h"
#include "EngineName.h"
#include "Rendering/SharedUniforms.h"
#include "Rendering/Vulkan/PushConstant.h"
#include "Rendering/Vulkan/RenderUtilities.h"
#include "Utilities/FileHelper.h"

#include <array>
#include <iostream>
#include <filesystem>
#include <volk.h>

namespace fs = std::filesystem;

void RenderPipeline::Initialize(const VkContext& inContext, RenderingInterface* inRenderingInterface)
{
	context = inContext;
	renderingInterface = inRenderingInterface;

	const std::string shaderPath = GetShaderPath();

	fs::path shader{ shaderPath };
	name = shader.filename().string();

	const std::string vertShaderPath = shaderPath + "/vert.spv";
	const std::string fragShaderPath = shaderPath + "/frag.spv";

	auto vertShaderCode = FileHelper::ReadFile(vertShaderPath);
	auto fragShaderCode = FileHelper::ReadFile(fragShaderPath);

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	VkSpecializationInfo vertSpecializationInfo{};
	if (CreateVertexSpecializationInfo(vertSpecializationInfo))
	{
		std::vector<VkSpecializationMapEntry> vertexMap = CreateVertexSpecializationMap(vertSpecializationInfo.dataSize);
		vertSpecializationInfo.pMapEntries = vertexMap.data();
		vertShaderStageInfo.pSpecializationInfo = &vertSpecializationInfo;
	}

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	VkSpecializationInfo fragSpecializationInfo{};
	if (CreateFragmentSpecializationInfo(fragSpecializationInfo))
	{
		std::vector<VkSpecializationMapEntry> fragmentMap = CreateFragmentSpecializationMap(fragSpecializationInfo.dataSize);
		fragSpecializationInfo.pMapEntries = fragmentMap.data();
		fragShaderStageInfo.pSpecializationInfo = &fragSpecializationInfo;
	}

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	CreateVertexInputInfo(vertexInputInfo);

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(context.swapChainExtent.width);
	viewport.height = static_cast<float>(context.swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = context.swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	//rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // TODO need to test this more with assimp
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = context.msaaSamples;
	multisampling.minSampleShading = 0.2f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	// Global layouts added first and the pipeline specific layouts will be added later
	std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
	CreatePipelineDescriptorLayoutSets(descriptorSetLayouts);
	std::vector<VkPushConstantRange> pushConstants = GetPipelinePushConstants();

	pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
	pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();

	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

	if (vkCreatePipelineLayout(context.device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create pipeline layout!" << std::endl;
	}

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = context.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
	{
		std::cerr << "Failed to create graphics pipeline!" << std::endl;

		vkDestroyShaderModule(context.device, fragShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertShaderModule, nullptr);
	}

	vkDestroyShaderModule(context.device, fragShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertShaderModule, nullptr);
}

void RenderPipeline::DestroyPipeline()
{
	VkDescriptorSetLayout layout = RenderUtilities::GenericHandleToDescriptorSetLayout(materialLayout.layout);
	vkDestroyDescriptorSetLayout(context.device, layout, nullptr);

	vkDestroyPipeline(context.device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);
}

void RenderPipeline::Bind(VkCommandBuffer cmdBuffer)
{
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

VkShaderModule RenderPipeline::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		std::cerr << "Failed to create shader module!" << std::endl;
	}

	return shaderModule;
}

void RenderPipeline::AllocateMaterialDescriptorSet(GenericHandle& outDescriptorSet)
{
	VkDescriptorSetLayout layout = RenderUtilities::GenericHandleToDescriptorSetLayout(materialLayout.layout);

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = context.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &layout;

	VkDescriptorSet set;
	if (vkAllocateDescriptorSets(context.device, &allocInfo, &set) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate descriptor set!" << std::endl;
	}

	outDescriptorSet = RenderUtilities::DescriptorSetToGenericHandle(set);
}

void RenderPipeline::UpdateMaterialDescriptorSet(const std::unordered_map<EngineName, DescriptorDataProvider>& dataProviders)
{
	std::vector<VkWriteDescriptorSet> writes;

	for (const DescriptorBindingInfo& layoutInfo : descriptorBindings)
	{
		auto it = dataProviders.find(layoutInfo.semantic);
		if (it != dataProviders.end())
		{
			const DescriptorDataProvider& provider = it->second;

			VkDescriptorImageInfo imageInfo{};
			if (provider.providerType == EDescriptorDataProviderType::Texture || provider.providerType == EDescriptorDataProviderType::All)
			{
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = RenderUtilities::GenericHandleToImageView(provider.texture.view);
				imageInfo.sampler = RenderUtilities::GenericHandleToImageSampler(provider.texture.sampler);
			}

			VkDescriptorBufferInfo bufferInfo{};
			if (provider.providerType == EDescriptorDataProviderType::Buffer || provider.providerType == EDescriptorDataProviderType::All)
			{
				bufferInfo.buffer = RenderUtilities::GenericHandleToBuffer(provider.dataProviderBuffer.buffer.buffer);
				bufferInfo.offset = provider.dataProviderBuffer.offset;
				bufferInfo.range = provider.dataProviderBuffer.range;
			}

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = RenderUtilities::GenericHandleToDescriptorSet(provider.descriptorSet);
			write.dstBinding = layoutInfo.binding;
			write.dstArrayElement = 0;
			write.descriptorType = static_cast<VkDescriptorType>(layoutInfo.type);
			write.descriptorCount = 1;
			write.pImageInfo = &imageInfo;
			write.pBufferInfo = &bufferInfo;
			writes.push_back(write);
		}
	}

	vkUpdateDescriptorSets(context.device, writes.size(), writes.data(), 0, nullptr);
}

bool RenderPipeline::SupportsCamera() const
{
	return (flags & MATRICES_DESCRIPTOR_FLAG) != 0;
}

bool RenderPipeline::SupportsLight() const
{
	return (flags & LIGHT_DESCRIPTOR_FLAG) != 0;
}
