#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <vector>

#include <fstream>
#include <iostream>
#include <string>
namespace core {

	bool HasStencilComponent(VkFormat Format);
	VkFormat FindSupportedFormat(VkPhysicalDevice Device, const std::vector<VkFormat>& Candidates,
		VkImageTiling Tiling, VkFormatFeatureFlags Features);
	const char* Get_DebugSeverityString(VkDebugUtilsMessageTypeFlagBitsEXT severity);
	const char* Get_DebugType(VkDebugUtilsMessageTypeFlagsEXT severity);
	int GetBytesPerTexFormat(VkFormat Format);
	VkDeviceAddress GetBufferDeviceAddress(VkDevice device, VkBuffer buffer);

}