#pragma once

#include "Camera/Camera.h"
#include "SystemBase.h"

#include <unordered_map>
#include <vector>

class CameraSystem : public SystemBase
{
public:
	~CameraSystem();
	CameraSystem();

	Camera* CreateCamera(const CameraData& data);

private:
	void HandleRenderFrameReset();

	std::unordered_map<uint32_t, Camera> cameras;
};