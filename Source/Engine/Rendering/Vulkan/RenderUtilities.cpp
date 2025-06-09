#include "RenderUtilities.h"

void RenderUtilities::SetDebugName(VkDevice device, uintptr_t objectHandle, VkObjectType objectType, const char* name)
{
	VkDebugUtilsObjectNameInfoEXT nameInfo =
	{
	   .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
	   .pNext = NULL,
	   .objectType = objectType,
	   .objectHandle = objectHandle,
	   .pObjectName = name,
	};
	vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
}
