#include "SystemBase.h"

uint32_t SystemBase::GenerateHandle()
{
	uint32_t currentID = 0;
	if (!freeHandles.empty())
	{
		currentID = freeHandles.front();
		freeHandles.pop();
		instanceCount++;
	}
	else
	{
		// TODO add a check for max light count reached
		currentID = instanceCount++;
	}
	return currentID;
}
