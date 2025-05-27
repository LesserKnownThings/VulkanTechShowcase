#include "DescriptorRegistry.h"
#include "Rendering/RenderingInterface.h"

DescriptorRegistry::DescriptorRegistry(RenderingInterface* inRenderingInterface)
	: renderingInterface(inRenderingInterface)
{
}

void DescriptorRegistry::Initialize()
{
	if (renderingInterface)
	{
		renderingInterface->CreateGlobalUniformBuffers(matricesBuffers);
		renderingInterface->CreateLightBuffer(lightBuffer);
		renderingInterface->CreateGlobalDescriptorLayouts(globalLayout, lightLayout);
		renderingInterface->AllocateGlobalDescriptorSet(globalLayout, globalDescriptorSets);
		renderingInterface->AllocateLightDescriptorSet(lightLayout, lightDescriptorSet);
	}
}

void DescriptorRegistry::UnInitialize()
{
	renderingInterface->DestroyBuffer(lightBuffer);

	for (const AllocatedBuffer& buffer : matricesBuffers)
	{
		renderingInterface->DestroyBuffer(buffer);
	}

	renderingInterface->DestroyDescriptorSetLayout(lightLayout);
	renderingInterface->DestroyDescriptorSetLayout(globalLayout);
}
