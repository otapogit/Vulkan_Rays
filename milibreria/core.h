#pragma once
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include "core_utils.h"
#include "physical_device.h"
#include "core_wrapper.h"
#include "core_queue.h"
namespace core {
	class BufferMemory {
	public:
		BufferMemory() {}

		VkBuffer m_buffer = NULL;
		VkDeviceMemory m_mem = NULL;
		VkDeviceSize m_allocationSize = 0;

		void Destroy(VkDevice device);
		void Update(VkDevice Device, const void* pData, size_t Size);
	};

	class VulkanCore {
	public:
		VulkanCore();
		~VulkanCore();
		void Init(const char* pAppName, GLFWwindow* pWindow);
		void CreateCommandBuffer(uint32_t count, VkCommandBuffer* cmdBufs);
		void FreeCommandBuffers(uint32_t count, const VkCommandBuffer* pCmdBufs);
		int GetNumImages() const { return (int)m_images.size(); }
		const VkImage& GetImage(int Index) const;
		VulkanQueue* GetQueue() { return &m_queue; }
		VkRenderPass CreateSimpleRenderPass();
		VkDevice& GetDevice() { return m_device; }
		std::vector<VkFramebuffer> CreateFrameBuffers(VkRenderPass RenderPass);
		BufferMemory CreateVertexBuffer(const void* pVertices, size_t Size);
		//BufferMemory CreateSimpleVertexBuffer(const void* pVertices, size_t Size);
		std::vector<BufferMemory> CreateUniformBuffers(size_t Size);

	private:
		void CreateInstance(const char* pAppName);
		void CreateDebugCallback();
		void CreateSurface(GLFWwindow* pWindow);
		void CreateDevice();
		void CreateSwapChain();
		void CreateCommandBufferPool();
		void CopyBufferToBuffer(VkBuffer Dst, VkBuffer Src, VkDeviceSize Size);
		BufferMemory CreateBuffer(VkDeviceSize Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties);
		uint32_t GetMemoryTypeIndex(uint32_t MemTypeBitsMask, VkMemoryPropertyFlags ReqMemPropFlags);

		BufferMemory CreateUniformBuffer(size_t Size);

		VkInstance m_instance = NULL;
		VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
		GLFWwindow* m_pWindow = NULL;
		VkSurfaceKHR m_surface = VK_NULL_HANDLE;
		VulkanPhysicalDevices m_physDevices;
		uint32_t m_queueFamily = 0;
		VkDevice m_device = VK_NULL_HANDLE;
		VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
		VkSurfaceFormatKHR m_swapChainSurfaceFormat;
		//ImageView -> acceso
		//Image -> Imagen real
		std::vector<VkImage> m_images;
		std::vector<VkImageView> m_imageViews;
	
		VkCommandPool m_cmdBufPool = VK_NULL_HANDLE;
		VulkanQueue m_queue;
		std::vector<VkFramebuffer> m_frameBuffers;
		VkCommandBuffer m_copyCmdBuf;
	
	};


}