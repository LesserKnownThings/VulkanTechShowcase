#pragma once

#include "../RenderPipeline.h"

class SpritePipeline : public RenderPipeline
{
public:

protected:
	void CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
	std::string GetShaderPath() const override { return "Data/Engine/Shaders/Lit"; }
};