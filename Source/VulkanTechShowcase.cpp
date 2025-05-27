#pragma once

#include "Engine.h"

#include <cstdint>
#include <iostream>

int32_t main(int32_t argCount, char* argVars[])
{
	Engine engine;
	if (engine.Initialize())
	{
		engine.Run();
		engine.UnInitialize();
		return 0;
	}
	return 1;
}