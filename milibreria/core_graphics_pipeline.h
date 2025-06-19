#pragma once
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include "core_simple_mesh.h"
namespace core{
	class GraphicsPipeline {

	public:
		GraphicsPipeline(VkDevice Device, GLFWwindow* pWindow, VkRenderPass RenderPass,
			int NumImages,std::vector<BufferMemory>& UniformBuffers, int UniformDataSize,VkShaderModule vs, VkShaderModule fs);
		
		~GraphicsPipeline();


		void Bind(VkCommandBuffer CmdBuf, int ImageIndex);
		void DrawMesh(VkCommandBuffer CmdBuf, const SimpleMesh& mesh);
		void DrawMeshIndexed(VkCommandBuffer CmdBuf, const SimpleMesh& mesh);
		void UpdateTexture(core::VulkanTexture* texture);
	private:
		void CreateDescriptorSets(int NumImages, std::vector<BufferMemory>& UniformBuffers, int UniformDataSize,core::VulkanTexture* texture = nullptr);
		void CreateDescriptorPool(int NumImages);
		void CreateDescriptorSetLayout(std::vector<BufferMemory>& UniformBuffers, int UniformDataSize);
		void AllocateDescriptorSets(int NumImages);
		void UpdateDescriptorSets(int NumImages, std::vector<BufferMemory>& UniformBuffers, int UniformDataSize,core::VulkanTexture* texture = nullptr);


		VkDevice m_device = NULL;
		VkPipeline m_pipeline = NULL;
		VkPipelineLayout m_pipelineLayout = NULL;
		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> m_descriptorSets;
	};
}