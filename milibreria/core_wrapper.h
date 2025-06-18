#pragma once

#include "core.h"
#include "core_utils.h"
#include <vulkan/vulkan_core.h>

namespace core {
	void BeginCommandBuffer(VkCommandBuffer CommandBuffer, VkCommandBufferUsageFlags UsageFlags);
	VkSemaphore CreateSemaphore(VkDevice Device);
	void ImageMemBarrier(VkCommandBuffer CmdBuf, VkImage Image, VkFormat Format,
		VkImageLayout OldLayout, VkImageLayout NewLayout);
	VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags);
	VkSampler CreateTextureSampler(VkDevice Device, VkFilter MinFilter, VkFilter MaxFilter, VkSamplerAddressMode AddressMode);
	
}