#include "CameraSystem.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"

CameraSystem::~CameraSystem()
{
	GameEngine->GetRenderingSystem()->onRenderFrameReset.Clear(this);
}

CameraSystem::CameraSystem()
{
	GameEngine->GetRenderingSystem()->onRenderFrameReset.Bind(this, &CameraSystem::HandleRenderFrameReset);
}

Camera* CameraSystem::CreateCamera(const CameraData& data)
{
	uint32_t handle = GenerateHandle();
	cameras.emplace(handle, Camera{ data });
	return &cameras[handle];
}

void CameraSystem::HandleRenderFrameReset()
{
	for (auto& it : cameras)
	{
		it.second.projectionChanged = false;
		it.second.viewChanged = false;
	}
}
