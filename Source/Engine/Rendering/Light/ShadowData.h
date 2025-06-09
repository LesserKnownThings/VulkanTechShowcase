#pragma once

#include "Rendering/Light/Shadow.h"

#include <array>
#include <cstdint>

struct alignas(16) ShadowData
{
	struct alignas(16) PaddedFloat
	{
		PaddedFloat() = default;
		PaddedFloat(float inValue) : value(inValue) {}

		float value;
	};

	alignas(16) PaddedFloat cascadePlaneDistance[MAX_SM];
	float farPlane = 0.0f;
	int32_t cascadeCount = 0;
};

struct alignas(16) ShadowDebugData
{
	float nearPlane;
	float farPlane;
	int32_t layer;
};
