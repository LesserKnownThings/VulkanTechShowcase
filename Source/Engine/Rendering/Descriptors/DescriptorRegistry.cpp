#include "DescriptorRegistry.h"
#include "AssetManager/Animation/AnimationData.h"
#include "Camera/CameraMatrices.h"
#include "Rendering/Light/Light.h"
#include "Rendering/RenderingInterface.h"

DescriptorRegistry::DescriptorRegistry(RenderingInterface* inRenderingInterface)
	: renderingInterface(inRenderingInterface)
{
}

void DescriptorRegistry::Initialize()
{
	if (renderingInterface)
	{
		renderingInterface->CreateGlobalDescriptorLayouts(cameraMatricesLayout, lightLayout, animationLayout);

		renderingInterface->CreateBuffer(EBufferType::Uniform, sizeof(CameraMatrices) * MAX_FRAMES_IN_FLIGHT, matriceBuffer);
		renderingInterface->CreateBuffer(EBufferType::Storage, sizeof(LightBufferLayout) * MAX_LIGHTS, lightBuffer);
		renderingInterface->CreateBuffer(EBufferType::Storage, sizeof(AnimationLayout) * MAX_ANIMATED_ENTITIES * MAX_FRAMES_IN_FLIGHT, animationBuffer);

		renderingInterface->CreateDescriptorSet(cameraMatricesLayout, cameraMatricesDescriptorSet);
		renderingInterface->UpdateDescriptorSet(EDescriptorSetType::Uniform, cameraMatricesDescriptorSet, matriceBuffer);

		renderingInterface->CreateDescriptorSet(lightLayout, lightDescriptorSet);
		renderingInterface->UpdateDescriptorSet(EDescriptorSetType::Storage, lightDescriptorSet, lightBuffer);

		renderingInterface->CreateDescriptorSet(animationLayout, animationDescriptorSet);
		renderingInterface->UpdateDynamicDescriptorSet(EDescriptorSetType::StorageDynamic, animationDescriptorSet, animationBuffer, 0, 0, sizeof(AnimationLayout));
	}
}

void DescriptorRegistry::UnInitialize()
{
	renderingInterface->DestroyBuffer(matriceBuffer);
	renderingInterface->DestroyBuffer(lightBuffer);
	renderingInterface->DestroyBuffer(animationBuffer);

	renderingInterface->DestroyDescriptorSetLayout(lightLayout);
	renderingInterface->DestroyDescriptorSetLayout(cameraMatricesLayout);
	renderingInterface->DestroyDescriptorSetLayout(animationLayout);
}
