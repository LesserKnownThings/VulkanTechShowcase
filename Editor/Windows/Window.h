#pragma once

class Window
{
public:
	virtual void Initialize() = 0;
	virtual void UnInitialize() = 0;

	virtual void Draw() = 0;

	bool IsShowing() const { return isShowing; }

protected:
	bool isShowing = false;
};