#include "core_queue.h"
#include "core_wrapper.h"
namespace core {

	void VulkanQueue::Init(VkDevice Device, VkSwapchainKHR SwapChain, uint32_t QueueFamily, uint32_t QueueIndex) {

		m_device = Device;
		m_swapChain = SwapChain;

		vkGetDeviceQueue(Device, QueueFamily, QueueIndex, &m_queue);

		printf("Queue acquired\n");

		CreateSemaphores();
	}

	void VulkanQueue::Destroy() {

		vkDestroySemaphore(m_device, m_presentCompleteSem, NULL);
		vkDestroySemaphore(m_device, m_renderCompleteSem, NULL);
	}

	void VulkanQueue::CreateSemaphores() {

		m_presentCompleteSem = CreateSemaphore(m_device);
		m_renderCompleteSem = CreateSemaphore(m_device);
	}

	void VulkanQueue::WaitIdle() {

		vkQueueWaitIdle(m_queue);
	}

	uint32_t VulkanQueue::AcquireNextImage() {

		uint32_t ImageIndex = 0;
		//3 ARGUMENTO ES CUANTO ESPERA PARA LA IMAGEN EN NANOSEGUNDOS
		VkResult res = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_presentCompleteSem, NULL, &ImageIndex);
		CHECK_VK_RESULT(res, "vkAcquireNextImageKHR\n");
		return ImageIndex;
	}

	void VulkanQueue::SubmitSync(VkCommandBuffer CmdBuf) {

		VkSubmitInfo SubmitInfo = {};
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.pNext = NULL;
		SubmitInfo.waitSemaphoreCount = 0;
		SubmitInfo.pWaitSemaphores = VK_NULL_HANDLE;
		SubmitInfo.pWaitDstStageMask = VK_NULL_HANDLE;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CmdBuf;
		SubmitInfo.signalSemaphoreCount = 0;
		SubmitInfo.pSignalSemaphores = VK_NULL_HANDLE;

		VkResult res = vkQueueSubmit(m_queue, 1, &SubmitInfo, NULL);
		CHECK_VK_RESULT(res, "vkQueueSubmit\n");  
	}

	void VulkanQueue::SubmitAsync(VkCommandBuffer CmdBuf) {

		VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo SubmitInfo = {};
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.pNext = NULL;
		SubmitInfo.waitSemaphoreCount = 1;
		SubmitInfo.pWaitSemaphores = &m_presentCompleteSem;
		SubmitInfo.pWaitDstStageMask = &waitFlags;
		//Por ahora solo 1 commandBuffer
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = &CmdBuf;
		SubmitInfo.signalSemaphoreCount = 1;
		SubmitInfo.pSignalSemaphores = &m_renderCompleteSem;

		//Por ahora solo 1 cosa a la vez
		VkResult res = vkQueueSubmit(m_queue, 1, &SubmitInfo, NULL);
		CHECK_VK_RESULT(res, "vkQueueSubmit Async\n");

	}

	void VulkanQueue::Present(uint32_t ImageIndex) {

		VkPresentInfoKHR PresentInfo = {};
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.pNext = NULL;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pWaitSemaphores = &m_renderCompleteSem;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pSwapchains = &m_swapChain;
		PresentInfo.pImageIndices = &ImageIndex;

		//a lo mejor necesita getprocaddr
		VkResult res = vkQueuePresentKHR(m_queue, &PresentInfo);
		CHECK_VK_RESULT(res, "vkQueuePresentKHR\n");
	}

}