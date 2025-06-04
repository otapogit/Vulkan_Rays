#pragma once

#include <vulkan/vulkan.h>
#include <stdio.h>

namespace core {
	class VulkanQueue {
	public:
		VulkanQueue() {}
		~VulkanQueue(){}
		void Init(VkDevice Device, VkSwapchainKHR SwapChain, uint32_t QueueFamily, uint32_t QueueIndex);
		void Destroy();
		uint32_t AcquireNextImage();
		void SubmitSync(VkCommandBuffer CmdBuf);
		void SubmitAsync(VkCommandBuffer CmdBuf);
		void Present(uint32_t ImageIndex);
		void WaitIdle();
	private:
		void CreateSemaphores();

		VkDevice m_device = VK_NULL_HANDLE;
		VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
		VkQueue m_queue = VK_NULL_HANDLE;
		VkSemaphore m_renderCompleteSem;
		VkSemaphore m_presentCompleteSem;

	};
}