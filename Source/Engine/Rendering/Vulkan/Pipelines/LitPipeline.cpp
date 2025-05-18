#include "LitPipeline.h"
#include "AssetManager/Model/MeshData.h"

#include <iostream>

void LitPipeline::DestroyPipeline()
{
	vkDestroyDescriptorSetLayout(context.device, samplerLayout, nullptr);
	RenderPipeline::DestroyPipeline();
}

void LitPipeline::CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo)
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

void LitPipeline::CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts)
{
	const uint32_t bindingCount = 4;
	VkDescriptorSetLayoutBinding* uboLayoutBinding = new VkDescriptorSetLayoutBinding[bindingCount];
	uboLayoutBinding[0].binding = 0;
	uboLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uboLayoutBinding[0].descriptorCount = 1;
	uboLayoutBinding[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding[0].pImmutableSamplers = nullptr;

	uboLayoutBinding[1].binding = 1;
	uboLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uboLayoutBinding[1].descriptorCount = 1;
	uboLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding[1].pImmutableSamplers = nullptr;

	uboLayoutBinding[2].binding = 2;
	uboLayoutBinding[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uboLayoutBinding[2].descriptorCount = 1;
	uboLayoutBinding[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding[2].pImmutableSamplers = nullptr;

	uboLayoutBinding[3].binding = 3;
	uboLayoutBinding[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uboLayoutBinding[3].descriptorCount = 1;
	uboLayoutBinding[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding[3].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = bindingCount;
	createInfo.pBindings = uboLayoutBinding;	

	if (vkCreateDescriptorSetLayout(context.device, &createInfo, nullptr, &samplerLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout for samplers!" << std::endl;
	}

	outDescriptorSetLayouts.push_back(samplerLayout);	
}
