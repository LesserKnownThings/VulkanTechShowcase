#include "SDLInterface.h"

#include <iostream>
#include <SDL3/SDL.h>

namespace Utilities
{
	void CheckError()
	{
		const char* error = SDL_GetError();
		if (*error != '\0')
		{
			std::cerr << "SDL Error: " << error << std::endl;
		}

		SDL_ClearError();
	}
}

bool SDLInterface::Initialize()
{
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS))
	{
		Utilities::CheckError();
		return false;
	}
	return true;
}

void SDLInterface::UnInitialize()
{
	SDL_Quit();
}
