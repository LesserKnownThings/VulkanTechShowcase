#pragma once

#include "Window.h"

#include <vector>

class MainWindow : public Window
{
public:
	void Initialize() override;
	void UnInitialize() override;
	void Draw() override;

private:	
	void PostDraw();

	std::vector<Window*> windows{};
};