#include "DescriptorRegistry.h"
#include "AssetManager/Animation/AnimationData.h"
#include "Camera/CameraMatrices.h"
#include "Rendering/Light/Light.h"
#include "Rendering/Light/Shadow.h"
#include "Rendering/Light/ShadowData.h"
#include "Rendering/RenderingInterface.h"

DescriptorRegistry::DescriptorRegistry(RenderingInterface* inRenderingInterface)
	: renderingInterface(inRenderingInterface)
{
}

void DescriptorRegistry::Initialize()
{
	if (renderingInterface)
	{
		renderingInterface->CreateGlobalDescriptorLayouts(cameraMatricesLayout, lightLayout, animationLayout, shadowLayout);

		renderingInterface->CreateBuffer(EBufferType::Uniform, sizeof(CameraMatrices) * MAX_FRAMES_IN_FLIGHT, matriceBuffer);
		renderingInterface->CreateBuffer(EBufferType::Storage, sizeof(LightBufferLayout) * MAX_LIGHTS, lightBuffer);
		renderingInterface->CreateBuffer(EBufferType::Storage, sizeof(AnimationLayout) * MAX_ANIMATED_ENTITIES * MAX_FRAMES_IN_FLIGHT, animationBuffer);

		renderingInterface->CreateBuffer(EBufferType::Uniform, sizeof(glm::mat4) * MAX_SM * MAX_FRAMES_IN_FLIGHT, lightSMBuffer);
		renderingInterface->CreateBuffer(EBufferType::Uniform, sizeof(ShadowData), shadowDataBuffer);

		renderingInterface->CreateDescriptorSet(cameraMatricesLayout, cameraMatricesDescriptorSet);
		renderingInterface->UpdateDescriptorSet(EDescriptorSetType::Uniform, cameraMatricesDescriptorSet, matriceBuffer);

		renderingInterface->CreateDescriptorSet(lightLayout, lightDescriptorSet);
		renderingInterface->UpdateDescriptorSet(EDescriptorSetType::Storage, lightDescriptorSet, lightBuffer);

		renderingInterface->CreateDescriptorSet(animationLayout, animationDescriptorSet);
		renderingInterface->UpdateDynamicDescriptorSet(EDescriptorSetType::StorageDynamic, animationDescriptorSet, animationBuffer, 0, 0, sizeof(AnimationLayout) * MAX_ANIMATED_ENTITIES);

		for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			renderingInterface->CreateDescriptorSet(shadowLayout, shadowDescriptorSets[i]);
			renderingInterface->UpdateDynamicDescriptorSet(EDescriptorSetType::UniformDynamic, shadowDescriptorSets[i], lightSMBuffer, 0, 0, sizeof(glm::mat4) * MAX_SM);
			renderingInterface->UpdateDescriptorSet(EDescriptorSetType::Uniform, shadowDescriptorSets[i], shadowDataBuffer, 2);
		}
	}
}

void DescriptorRegistry::UnInitialize()
{
	renderingInterface->DestroyBuffer(matriceBuffer);
	renderingInterface->DestroyBuffer(lightBuffer);
	renderingInterface->DestroyBuffer(lightSMBuffer);
	renderingInterface->DestroyBuffer(animationBuffer);
	renderingInterface->DestroyBuffer(shadowDataBuffer);

	renderingInterface->DestroyDescriptorSetLayout(lightLayout);
	renderingInterface->DestroyDescriptorSetLayout(cameraMatricesLayout);
	renderingInterface->DestroyDescriptorSetLayout(animationLayout);
	renderingInterface->DestroyDescriptorSetLayout(shadowLayout);
}
