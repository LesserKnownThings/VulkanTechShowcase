#pragma once

#include "Window.h"

#include <cstdint>

struct BoneNode;

class SkeletonViewerWindow : public Window
{
public:
	virtual void Initialize() override;
	virtual void UnInitialize() override;

	virtual void Draw() override;

private:
	void DrawBoneTree(const BoneNode& node);

	uint32_t handle;
};