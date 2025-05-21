#include "PBRPipeline.h"
#include "AssetManager/Model/MeshData.h"
#include "AssetManager/Texture/Texture.h"
#include "Rendering/RenderingInterface.h"
#include "Rendering/Vulkan/RenderUtilities.h"

#include <iostream>

void PBRPipeline::DestroyPipeline()
{
	vkDestroyDescriptorSetLayout(context.device, samplerSetLayout, nullptr);
	RenderPipeline::DestroyPipeline();
}

void PBRPipeline::AllocateDescriptorSet(GenericHandle& outDescriptorSet)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = context.descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &samplerSetLayout;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(context.device, &allocInfo, &descriptorSet) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate global descriptor sets!" << std::endl;
	}

	outDescriptorSet = RenderUtilities::DescriptorSetToGenericHandle(descriptorSet);
}

void PBRPipeline::UpdateDescriptorSet(GenericHandle descriptorSet, const TextureSetKey& key)
{
	const int32_t descriptorsCount = key.views.size();

	VkWriteDescriptorSet* descriptorWrites = new VkWriteDescriptorSet[descriptorsCount]{};
	for (int32_t i = 0; i < descriptorsCount; ++i)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = RenderUtilities::GenericHandleToImageView(key.views[i]);
		imageInfo.sampler = RenderUtilities::GenericHandleToImageSampler(key.samplers[i]);

		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i].dstSet = RenderUtilities::GenericHandleToDescriptorSet(descriptorSet);
		descriptorWrites[i].dstBinding = i;
		descriptorWrites[i].dstArrayElement = 0;
		descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[i].descriptorCount = 1;
		descriptorWrites[i].pImageInfo = &imageInfo;
	}

	vkUpdateDescriptorSets(context.device, descriptorsCount, descriptorWrites, 0, nullptr);
	delete[] descriptorWrites;
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

	if (vkCreateDescriptorSetLayout(context.device, &createInfo, nullptr, &samplerSetLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout for samplers!" << std::endl;
	}

	outDescriptorSetLayouts.push_back(samplerSetLayout);
}
