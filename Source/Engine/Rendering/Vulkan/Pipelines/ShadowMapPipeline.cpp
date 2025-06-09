#include "ShadowMapPipeline.h"
#include "AssetManager/Animation/BoneData.h"
#include "AssetManager/Model/MeshData.h"
#include "Rendering/Light/Shadow.h"
#include "Rendering/AbstractData.h"
#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Descriptors/DescriptorRegistry.h"
#include "Rendering/Descriptors/Semantics.h"
#include "Rendering/Light/Shadow.h"
#include "Rendering/RenderingInterface.h"
#include "Rendering/Vulkan/RenderUtilities.h"
#include "Utilities/FileHelper.h"

#include <array>
#include <iostream>

ShadowMapPipeline::ShadowMapPipeline(const VkContext& inContext, RenderingInterface* inRenderingInterface)
	: RenderPipeline(inContext, inRenderingInterface)
{
	// SHADER STAGES
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

	// Vertex
	const std::string vertShaderPath = shaderPath + "/vert.spv";
	auto vertShaderCode = FileHelper::ReadFile(vertShaderPath);

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.pNext = nullptr;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkSpecializationInfo vertexSpecializationInfo{};
	std::array<uint32_t, 2> vertexSpecData =
	{
		MAX_BONES,
		MAX_BONE_INFLUENCE
	};
	vertexSpecializationInfo.pData = vertexSpecData.data();
	vertexSpecializationInfo.dataSize = vertexSpecData.size() * sizeof(uint32_t);

	std::array<VkSpecializationMapEntry, 2> vertSpecializationMap =
	{
		VkSpecializationMapEntry
		{
			.constantID = 0,
			.offset = 0,
			.size = sizeof(uint32_t)
		},
		VkSpecializationMapEntry
		{
			.constantID = 1,
			.offset = sizeof(uint32_t),
			.size = sizeof(uint32_t)
		}
	};

	vertexSpecializationInfo.pMapEntries = vertSpecializationMap.data();
	vertexSpecializationInfo.mapEntryCount = vertSpecializationMap.size();
	vertShaderStageInfo.pSpecializationInfo = &vertexSpecializationInfo;

	shaderStages[0] = vertShaderStageInfo;

	// Geometry
	const std::string geomShaderPath = shaderPath + "/geom.spv";
	auto geomShaderCode = FileHelper::ReadFile(geomShaderPath);

	VkShaderModule geomShaderModule = CreateShaderModule(geomShaderCode);

	VkPipelineShaderStageCreateInfo geomShaderStageInfo{};
	geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	geomShaderStageInfo.pNext = nullptr;
	geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
	geomShaderStageInfo.module = geomShaderModule;
	geomShaderStageInfo.pName = "main";

	VkSpecializationInfo geomSpecializationInfo{};
	uint32_t data[1] =
	{
		MAX_SM
	};
	geomSpecializationInfo.pData = data;
	geomSpecializationInfo.dataSize = sizeof(uint32_t);

	std::array<VkSpecializationMapEntry, 1> geomSpecializationMap =
	{
		VkSpecializationMapEntry
		{
			.constantID = 0,
			.offset = 0,
			.size = sizeof(uint32_t)
		}
	};
	geomSpecializationInfo.pMapEntries = geomSpecializationMap.data();
	geomSpecializationInfo.mapEntryCount = geomSpecializationMap.size();
	geomShaderStageInfo.pSpecializationInfo = &geomSpecializationInfo;

	shaderStages[1] = geomShaderStageInfo;
	// *******

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;

	std::array<VkVertexInputBindingDescription, 1> bindingDescriptions{};
	std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SINT;
	attributeDescriptions[1].offset = offsetof(Vertex, boneIDs);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, weights);

	vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
	vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
	vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	// *************	

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = nullptr;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	inputAssembly.pNext = nullptr;

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
	viewportState.pNext = nullptr;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizer.depthBiasEnable = VK_TRUE;
	rasterizer.depthBiasConstantFactor = 1.25f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 1.75f;
	rasterizer.pNext = nullptr;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;

	VkDescriptorSetLayout animationLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(renderingInterface->GetDescriptorRegistry()->GetAnimationLayout().layout);
	VkDescriptorSetLayout shadowLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(renderingInterface->GetDescriptorRegistry()->GetShadowLayout().layout);

	std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts
	{
		animationLayout,
		shadowLayout
	};

	flags = MATRICES_DESCRIPTOR_FLAG | LIGHT_DESCRIPTOR_FLAG | ANIMATION_DESCRIPTOR_FLAG;

	std::array<VkPushConstantRange, 1> pushConstants{};
	pushConstants[0].offset = 0;

	struct alignas(16) ShadowConstants
	{
		glm::mat4 model;
		uint32_t hasAnimation;
	};

	pushConstants[0].size = sizeof(ShadowConstants);
	pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

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
	depthStencil.pNext = nullptr;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 0;
	colorBlending.pAttachments = nullptr;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = context.shadowPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pNext = nullptr;

	if (vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
	{
		std::cerr << "Failed to create graphics pipeline!" << std::endl;
	}

	for (VkPipelineShaderStageCreateInfo& stage : shaderStages)
	{
		vkDestroyShaderModule(context.device, stage.module, nullptr);
	}
}

EPipelineType ShadowMapPipeline::GetType() const
{
	return EPipelineType::ShadowMap;
}
