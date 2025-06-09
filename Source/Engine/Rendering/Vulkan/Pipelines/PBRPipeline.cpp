#include "PBRPipeline.h"
#include "AssetManager/Animation/BoneData.h"
#include "AssetManager/Model/MeshData.h"
#include "Rendering/Descriptors/DescriptorRegistry.h"
#include "Rendering/Descriptors/Semantics.h"
#include "Rendering/Light/Light.h"
#include "Rendering/Light/Shadow.h"
#include "Rendering/RenderingInterface.h"
#include "Rendering/Vulkan/PushConstant.h"
#include "Rendering/Vulkan/RenderUtilities.h"
#include "Utilities/FileHelper.h"

#include <array>
#include <cstdint>
#include <iostream>

PBRPipeline::PBRPipeline(const VkContext& inContext, RenderingInterface* inRenderingInterface)
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

	VkSpecializationInfo vertSpecializationInfo{};
	uint32_t data[2] =
	{
		MAX_BONES,
		MAX_BONE_INFLUENCE
	};
	vertSpecializationInfo.pData = data;
	vertSpecializationInfo.dataSize = sizeof(uint32_t) * 2;

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
		},
	};
	vertSpecializationInfo.pMapEntries = vertSpecializationMap.data();
	vertSpecializationInfo.mapEntryCount = vertSpecializationMap.size();
	vertShaderStageInfo.pSpecializationInfo = &vertSpecializationInfo;

	shaderStages[0] = vertShaderStageInfo;

	// Fragment
	const std::string fragShaderPath = shaderPath + "/frag.spv";
	auto fragShaderCode = FileHelper::ReadFile(fragShaderPath);

	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.pNext = nullptr;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkSpecializationInfo fragSpecializationInfo{};
	uint32_t fragmentData[5] =
	{
		MAX_LIGHTS,
		static_cast<uint32_t>(ELightType::Directional),
		static_cast<uint32_t>(ELightType::Point),
		static_cast<uint32_t>(ELightType::Spot),
		MAX_SM
	};
	fragSpecializationInfo.dataSize = sizeof(uint32_t) * 5;
	fragSpecializationInfo.pData = fragmentData;

	std::array<VkSpecializationMapEntry, 5> fragSpecializationMap =
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
		},
		VkSpecializationMapEntry
		{
			.constantID = 2,
			.offset = sizeof(uint32_t) * 2,
			.size = sizeof(uint32_t)
		},
		VkSpecializationMapEntry
		{
			.constantID = 3,
			.offset = sizeof(uint32_t) * 3,
			.size = sizeof(uint32_t)
		},
		VkSpecializationMapEntry
		{
			.constantID = 4,
			.offset = sizeof(uint32_t) * 4,
			.size = sizeof(uint32_t)
		}
	};

	fragSpecializationInfo.mapEntryCount = fragSpecializationMap.size();
	fragSpecializationInfo.pMapEntries = fragSpecializationMap.data();
	fragShaderStageInfo.pSpecializationInfo = &fragSpecializationInfo;

	shaderStages[1] = fragShaderStageInfo;

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;

	std::array<VkVertexInputBindingDescription, 1> bindingDescriptions{};
	std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};

	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, tangent);

	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex, biTangent);

	attributeDescriptions[4].binding = 0;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(Vertex, uv);

	attributeDescriptions[5].binding = 0;
	attributeDescriptions[5].location = 5;
	attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SINT;
	attributeDescriptions[5].offset = offsetof(Vertex, boneIDs);

	attributeDescriptions[6].binding = 0;
	attributeDescriptions[6].location = 6;
	attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attributeDescriptions[6].offset = offsetof(Vertex, weights);

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
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;
	rasterizer.pNext = nullptr;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_TRUE;
	multisampling.rasterizationSamples = context.msaaSamples;
	multisampling.minSampleShading = 0.2f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	multisampling.pNext = nullptr;

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
	colorBlending.pNext = nullptr;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.pNext = nullptr;

	// Global layouts added first and the pipeline specific layouts will be added later
	std::array<VkDescriptorSetLayout, 5> descriptorSetLayouts{};
	if (const DescriptorRegistry* registry = renderingInterface->GetDescriptorRegistry())
	{
		VkDescriptorSetLayout cameraMatricesLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(registry->GetCameraMatricesLayout().layout);
		VkDescriptorSetLayout lightLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(registry->GetLightLayout().layout);
		VkDescriptorSetLayout animationLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(registry->GetAnimationLayout().layout);
		VkDescriptorSetLayout shadowLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(registry->GetShadowLayout().layout);

		descriptorSetLayouts[0] = cameraMatricesLayout;
		descriptorSetLayouts[1] = lightLayout;
		descriptorSetLayouts[2] = shadowLayout;
		descriptorSetLayouts[3] = animationLayout;		
	}

	CreatePipelineDescriptorLayoutSets(descriptorSetLayouts);

	flags = MATRICES_DESCRIPTOR_FLAG | LIGHT_DESCRIPTOR_FLAG | ANIMATION_DESCRIPTOR_FLAG;

	std::array<VkPushConstantRange, 1> pushConstants{};
	pushConstants[0].offset = 0;
	pushConstants[0].size = sizeof(SharedConstant);
	pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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
	pipelineInfo.renderPass = context.renderPass;
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

void PBRPipeline::CreatePipelineDescriptorLayoutSets(std::array<VkDescriptorSetLayout, 5>& outDescriptorSetLayouts)
{
	std::array<VkDescriptorSetLayoutBinding, 1> samplersLayoutBinding = {};

	// Albedo
	samplersLayoutBinding[0].binding = 0;
	samplersLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplersLayoutBinding[0].descriptorCount = 1;
	samplersLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplersLayoutBinding[0].pImmutableSamplers = nullptr;
	descriptorBindings.emplace_back(
		DescriptorBindingInfo
		{
			static_cast<uint32_t>(samplersLayoutBinding[0].binding),
			static_cast<uint32_t>(samplersLayoutBinding[0].descriptorType),
			static_cast<uint32_t>(samplersLayoutBinding[0].stageFlags),
			Semantics::AlbedoSampler
		});

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = samplersLayoutBinding.size();
	createInfo.pBindings = samplersLayoutBinding.data();
	createInfo.pNext = nullptr;

	VkDescriptorSetLayout samplerLayout;
	if (vkCreateDescriptorSetLayout(context.device, &createInfo, nullptr, &samplerLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout for samplers!" << std::endl;
	}
	outDescriptorSetLayouts[4] = samplerLayout;

	descriptorLayouts.emplace_back(DescriptorSetLayoutInfo
		{
			RenderUtilities::DescriptorSetLayoutToGenericHandle(samplerLayout),
			4,
			EDescriptorOwner::Material
		});
}
