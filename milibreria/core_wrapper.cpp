
#include "core_wrapper.h"

namespace core {
	void BeginCommandBuffer(VkCommandBuffer CommandBuffer, VkCommandBufferUsageFlags UsageFlags) {

		VkCommandBufferBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.pNext = NULL;
		BeginInfo.flags = UsageFlags;
		BeginInfo.pInheritanceInfo = NULL;

		VkResult res = vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
		CHECK_VK_RESULT(res, "vkBeginCommandBuffer\n");

	}

	VkSemaphore CreateSemaphore(VkDevice Device) {
		VkSemaphoreCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		CreateInfo.pNext = NULL;
		CreateInfo.flags = 0;

		VkSemaphore Semaphore;
		VkResult res = vkCreateSemaphore(Device, &CreateInfo, NULL, &Semaphore);
		CHECK_VK_RESULT(res, "vkCreateSemaphore\n");
		return Semaphore;
	}
}