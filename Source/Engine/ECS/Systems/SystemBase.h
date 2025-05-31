#pragma once

#include <cstdint>
#include <queue>

class SystemBase
{
protected:
	virtual uint32_t GenerateHandle();

	uint32_t instanceCount = 0;
	std::queue<uint32_t> freeHandles;
};