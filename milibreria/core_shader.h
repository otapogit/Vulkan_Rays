#pragma once
#include <vulkan/vulkan_core.h>

namespace core {

	VkShaderModule CreateShaderModuleFromBinary(VkDevice& device, const char* pFilename);
	VkShaderModule CreateShaderModuleFromText(VkDevice& device, const char* pFilename);
}