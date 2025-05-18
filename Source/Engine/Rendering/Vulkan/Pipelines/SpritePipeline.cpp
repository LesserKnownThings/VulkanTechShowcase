#include "SpritePipeline.h"

#include <array>
#include <glm/glm.hpp>

void SpritePipeline::CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo)
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	//bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription* attributeDescriptions = new VkVertexInputAttributeDescription[1];

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	//attributeDescriptions[0].offset = offsetof(Vertex, position);

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

	vertexInputInfo.vertexAttributeDescriptionCount = 1;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	/*VkVertexInputBindingDescription* bindingDescription = new VkVertexInputBindingDescription[2];

	bindingDescription[0].binding = 0;
	bindingDescription[0].stride = sizeof(Vertex);
	bindingDescription[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	bindingDescription[1].binding = 1;
	bindingDescription[1].stride = sizeof(InstanceData);
	bindingDescription[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

	VkVertexInputAttributeDescription* attributeDescriptions = new VkVertexInputAttributeDescription[6];

	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);
	
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);
		
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, uv);

	attributeDescriptions[3].binding = 1;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(InstanceData, offset);

	attributeDescriptions[4].binding = 1;
	attributeDescriptions[4].location = 4;
	attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[4].offset = offsetof(InstanceData, scale);

	attributeDescriptions[5].binding = 1;
	attributeDescriptions[5].location = 5;
	attributeDescriptions[5].format = VK_FORMAT_R32_SINT;
	attributeDescriptions[5].offset = offsetof(InstanceData, textureIndex);

	vertexInputInfo.vertexBindingDescriptionCount = 2;
	vertexInputInfo.pVertexBindingDescriptions = bindingDescription;

	vertexInputInfo.vertexAttributeDescriptionCount = 6;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;*/
}
