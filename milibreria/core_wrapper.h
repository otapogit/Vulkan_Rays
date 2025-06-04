#pragma once

#include "core.h"
#include "core_utils.h"
#include <vulkan/vulkan_core.h>

namespace core {
	void BeginCommandBuffer(VkCommandBuffer CommandBuffer, VkCommandBufferUsageFlags UsageFlags);
	VkSemaphore CreateSemaphore(VkDevice Device);
	
}