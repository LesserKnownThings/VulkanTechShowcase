#include "PBRPipeline.h"
#include "AssetManager/Model/MeshData.h"
#include "Rendering/Descriptors/DescriptorRegistry.h"
#include "Rendering/Light/Light.h"
#include "Rendering/RenderingInterface.h"
#include "Rendering/Vulkan/PushConstant.h"
#include "Rendering/Vulkan/RenderUtilities.h"

#include <cstdint>
#include <iostream>

std::vector<VkPushConstantRange> PBRPipeline::GetPipelinePushConstants()
{
	std::vector<VkPushConstantRange> pushConstants(2);

	pushConstants[0].offset = 0;
	pushConstants[0].size = sizeof(EntityTransformModel);
	pushConstants[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pushConstants[1].offset = sizeof(EntityTransformModel);
	pushConstants[1].size = sizeof(LightConstant);
	pushConstants[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	return pushConstants;
}

bool PBRPipeline::CreateVertexSpecializationInfo(VkSpecializationInfo& outInfo)
{
	return false;
}

bool PBRPipeline::CreateFragmentSpecializationInfo(VkSpecializationInfo& outInfo)
{
	uint32_t data[4] = { MAX_LIGHTS, LIGHT_TYPE_DIRECTIONAL, LIGHT_TYPE_POINT, LIGHT_TYPE_SPOT };
	outInfo.pData = data;
	return true;
}

std::vector<VkSpecializationMapEntry> PBRPipeline::CreateVertexSpecializationMap(size_t& dataSize)
{
	return {};
}

std::vector<VkSpecializationMapEntry> PBRPipeline::CreateFragmentSpecializationMap(size_t& dataSize)
{
	std::vector<VkSpecializationMapEntry> map =
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
		}
	};

	dataSize = sizeof(uint32_t) * map.size();
	return map;
}

void PBRPipeline::CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo)
{
	VkVertexInputBindingDescription* bindingDescription = new VkVertexInputBindingDescription[1];
	VkVertexInputAttributeDescription* attributeDescriptions = new VkVertexInputAttributeDescription[5];

	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Vertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

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

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = bindingDescription;

	vertexInputInfo.vertexAttributeDescriptionCount = 5;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;
}

void PBRPipeline::CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts)
{
	// The order in which the layouts are pushed matters
	if (const DescriptorRegistry* registry = renderingInterface->GetDescriptorRegistry())
	{
		VkDescriptorSetLayout globalLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(registry->GetGlobalLayout().layout);
		VkDescriptorSetLayout lightLayout = RenderUtilities::GenericHandleToDescriptorSetLayout(registry->GetLightLayout().layout);

		outDescriptorSetLayouts.push_back(globalLayout);
		outDescriptorSetLayouts.push_back(lightLayout);

		flags = MATRICES_DESCRIPTOR_FLAG | LIGHT_DESCRIPTOR_FLAG;
	}

	std::array<VkDescriptorSetLayoutBinding, 1> uboLayoutBinding = {};

	// Albedo
	uboLayoutBinding[0].binding = 0;
	uboLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uboLayoutBinding[0].descriptorCount = 1;
	uboLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding[0].pImmutableSamplers = nullptr;
	descriptorBindings.emplace_back(DescriptorBindingInfo
		{
			EDescriptorOwner::Material, 0, 0,
			static_cast<uint32_t>(uboLayoutBinding[0].descriptorType),
			static_cast<uint32_t>(uboLayoutBinding[0].stageFlags),
			"albedo"
		});

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = uboLayoutBinding.size();
	createInfo.pBindings = uboLayoutBinding.data();

	VkDescriptorSetLayout samplerLayout;
	if (vkCreateDescriptorSetLayout(context.device, &createInfo, nullptr, &samplerLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout for samplers!" << std::endl;
	}
	outDescriptorSetLayouts.push_back(samplerLayout);
	materialLayout = DescriptorSetLayoutInfo
	{
		RenderUtilities::DescriptorSetLayoutToGenericHandle(samplerLayout),
		2,
		EDescriptorOwner::Material
	};
}
