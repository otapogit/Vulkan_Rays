#include "core_graphics_pipeline.h"

#include "core_utils.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
namespace core {

	GraphicsPipeline::GraphicsPipeline(VkDevice Device, GLFWwindow* pWindow, VkRenderPass RenderPass,
		const SimpleMesh* pMesh,int NumImages, 
		std::vector<BufferMemory>& UniformBuffers, int UniformDataSize,
		VkShaderModule vs, VkShaderModule fs) {


		printf("Creating graphics pipeline \n");
		printf("Creating graphics pipeline \n");
		m_device = Device;

		if (pMesh) {
			CreateDescriptorSets(NumImages, pMesh, UniformBuffers,UniformDataSize);
		}

		VkPipelineShaderStageCreateInfo ShaderStageCreateInfo[2] = {};

		ShaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ShaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		ShaderStageCreateInfo[0].module = vs;
		ShaderStageCreateInfo[0].pName = "main";

		ShaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		ShaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		ShaderStageCreateInfo[1].module = fs;
		ShaderStageCreateInfo[1].pName = "main";


		VkPipelineVertexInputStateCreateInfo VertexInputInfo = {};
		VertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		VkPipelineInputAssemblyStateCreateInfo PipelineIACreateInfo = {};
		PipelineIACreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		//line strip, triangles, etc
		PipelineIACreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		PipelineIACreateInfo.primitiveRestartEnable = VK_FALSE;

		int WindowWidth, WindowHeight;
		WindowWidth = 1920;
		WindowHeight = 1055;
		//por alguna razon esto me daba error????
		//glfwGetWindowSize(pWindow, &WindowWidth, &WindowHeight);
		VkViewport VP = {};
		VP.x = 0.0f;
		VP.y = 0.0f,
		VP.width = (float)WindowWidth;
		VP.height = (float)WindowHeight;
		VP.minDepth = 0.0f;
		VP.maxDepth = 1.0f;

		VkRect2D Scissor = {};
		Scissor.offset = { 0,0 };
		Scissor.extent = { (uint32_t)WindowWidth, (uint32_t)WindowHeight };

		VkPipelineViewportStateCreateInfo VPCreateInfo = {};
		VPCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		VPCreateInfo.viewportCount = 1;
		VPCreateInfo.pViewports = &VP;
		VPCreateInfo.scissorCount = 1;
		VPCreateInfo.pScissors = &Scissor;

		VkPipelineRasterizationStateCreateInfo RastCreateInfo = {};
		RastCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		//fill, lines, point
		RastCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		RastCreateInfo.cullMode = VK_CULL_MODE_NONE;
		RastCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		RastCreateInfo.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo PipelineMSCreateInfo = {};
		PipelineMSCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		PipelineMSCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		PipelineMSCreateInfo.sampleShadingEnable = VK_FALSE;
		PipelineMSCreateInfo.minSampleShading = 1.0f;

		VkPipelineDepthStencilStateCreateInfo DepthStencilState = {};
		DepthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		DepthStencilState.depthTestEnable = VK_TRUE;
		DepthStencilState.depthWriteEnable = VK_TRUE;
		DepthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
		DepthStencilState.depthBoundsTestEnable = VK_FALSE;
		DepthStencilState.stencilTestEnable = VK_FALSE;
		DepthStencilState.front = {};
		DepthStencilState.back = {};
		DepthStencilState.minDepthBounds = 0.0f;
		DepthStencilState.maxDepthBounds = 1.0f;

		VkPipelineColorBlendAttachmentState BlendAttachState = {};
		BlendAttachState.blendEnable = VK_FALSE;
		BlendAttachState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo BlendCreateInfo = {};
		BlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		BlendCreateInfo.logicOpEnable = VK_FALSE;
		BlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		BlendCreateInfo.attachmentCount = 1;
		BlendCreateInfo.pAttachments = &BlendAttachState;

		VkPipelineLayoutCreateInfo LayoutInfo = {};
		LayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

		if (pMesh && pMesh->m_vb.m_buffer) {
			LayoutInfo.setLayoutCount = 1;
			LayoutInfo.pSetLayouts = &m_descriptorSetLayout;
		}
		else {
			LayoutInfo.setLayoutCount = 0;
			LayoutInfo.pSetLayouts = VK_NULL_HANDLE;
		}

		VkResult res = vkCreatePipelineLayout(m_device, &LayoutInfo, NULL, &m_pipelineLayout);
		CHECK_VK_RESULT(res, "vkCreatePipelineLayout\n");

		printf("created pipeline layout\n");

		VkGraphicsPipelineCreateInfo PipelineInfo = {};
		PipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		PipelineInfo.stageCount = sizeof(ShaderStageCreateInfo) / sizeof(ShaderStageCreateInfo[0]);
		PipelineInfo.pStages = &ShaderStageCreateInfo[0];
		PipelineInfo.pVertexInputState = &VertexInputInfo;
		PipelineInfo.pInputAssemblyState = &PipelineIACreateInfo;
		PipelineInfo.pViewportState = &VPCreateInfo;
		PipelineInfo.pRasterizationState = &RastCreateInfo;
		PipelineInfo.pMultisampleState = &PipelineMSCreateInfo;

		PipelineInfo.pColorBlendState = &BlendCreateInfo;
		PipelineInfo.layout = m_pipelineLayout;
		PipelineInfo.renderPass = RenderPass;
		PipelineInfo.subpass = 0;
		PipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		PipelineInfo.basePipelineIndex = -1;


		res = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &PipelineInfo, NULL, &m_pipeline);
		CHECK_VK_RESULT(res, "vkCreateGraphicsPipelines\n");

		printf("Graphics pipeline created\n");
	}

	GraphicsPipeline::~GraphicsPipeline()
	{
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, NULL);
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, NULL);
		vkDestroyDescriptorPool(m_device, m_descriptorPool, NULL);
		vkDestroyPipeline(m_device, m_pipeline, NULL);
	}

	void GraphicsPipeline::Bind(VkCommandBuffer CmdBuf, int ImageIndex) {
		vkCmdBindPipeline(CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

		if (m_descriptorSets.size() > 0) {
			vkCmdBindDescriptorSets(CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
				0,//firstSet
				1,//DescriptorSetCount
				&m_descriptorSets[ImageIndex],
				0,//dynamicOffsetCount
				NULL);//pDynamicOffsets
		}
	}

	void GraphicsPipeline::CreateDescriptorSets(int NumImages, const SimpleMesh* pMesh, std::vector<BufferMemory>& UniformBuffers, int UniformDataSize) {

		CreateDescriptorPool(NumImages);

		CreateDescriptorSetLayout(UniformBuffers, UniformDataSize);

		AllocateDescriptorSets(NumImages);
		
		UpdateDescriptorSets(NumImages, pMesh, UniformBuffers, UniformDataSize);

	}

	void GraphicsPipeline::CreateDescriptorPool(int NumImages) {

		VkDescriptorPoolCreateInfo PoolInfo = {};
		PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		//Para alocar cosas en el otro metodo cambiar flags
		PoolInfo.flags = 0;
		//Esto puede ser cambiado para pasar mas cosas
		PoolInfo.maxSets = (uint32_t)NumImages;
		//A lo mejor hay que cambiar las pools para poner las cosas en especifico tipo uniforms y tal
		PoolInfo.poolSizeCount = 0;
		PoolInfo.pPoolSizes = NULL;

		VkResult res = vkCreateDescriptorPool(m_device, &PoolInfo, NULL, &m_descriptorPool);
		CHECK_VK_RESULT(res, "vkCreateDescriptorPool");
		printf("Descriptor pool created\n");

	}

	void GraphicsPipeline::CreateDescriptorSetLayout(std::vector<BufferMemory>& UniformBuffers, int UniformDataSize) {

		/*
			AQUI SOLO ESTÁ PASANDO LOS VERTICES PARA PASAR MÁS COSAS EXTENDER ESTO
		*/

		std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;

		VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_VB = {};
		VertexShaderLayoutBinding_VB.binding = 0;
		VertexShaderLayoutBinding_VB.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		VertexShaderLayoutBinding_VB.descriptorCount = 1;
		VertexShaderLayoutBinding_VB.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		LayoutBindings.push_back(VertexShaderLayoutBinding_VB);

		VkDescriptorSetLayoutBinding VertexShaderLayoutBinding_Uniform = {};
		VertexShaderLayoutBinding_Uniform.binding = 1;
		VertexShaderLayoutBinding_Uniform.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		VertexShaderLayoutBinding_Uniform.descriptorCount = 1;
		//Obviamente si es necesario ampliar esto
		VertexShaderLayoutBinding_Uniform.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		if (UniformBuffers.size() > 0) {
			LayoutBindings.push_back(VertexShaderLayoutBinding_Uniform);
		}

		VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
		LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		LayoutInfo.pNext = NULL;
		LayoutInfo.flags = 0;
		LayoutInfo.bindingCount = (uint32_t)LayoutBindings.size();
		LayoutInfo.pBindings = LayoutBindings.data();

		VkResult res = vkCreateDescriptorSetLayout(m_device, &LayoutInfo, NULL, &m_descriptorSetLayout);
		CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout\n");

	}

	void GraphicsPipeline::AllocateDescriptorSets(int NumImages) {

		std::vector<VkDescriptorSetLayout> Layouts(NumImages, m_descriptorSetLayout);
		
		VkDescriptorSetAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		AllocInfo.pNext = NULL;
		AllocInfo.descriptorPool = m_descriptorPool;
		AllocInfo.descriptorSetCount = (uint32_t)NumImages;
		AllocInfo.pSetLayouts = Layouts.data();

		m_descriptorSets.resize(NumImages);

		VkResult res = vkAllocateDescriptorSets(m_device, &AllocInfo, m_descriptorSets.data());
		CHECK_VK_RESULT(res, "vkAllocateDescriptorSets\n");
	}
	void GraphicsPipeline::UpdateDescriptorSets(int NumImages, const SimpleMesh* pMesh, std::vector<BufferMemory>& UniformBuffers, int UniformDataSize) {

		VkDescriptorBufferInfo BufferInfo_VB = {};
		BufferInfo_VB.buffer = pMesh->m_vb.m_buffer;
		BufferInfo_VB.offset = 0;
		BufferInfo_VB.range = pMesh->m_vertexBufferSize; //o VK_WHOLE_SIZE

		std::vector<VkWriteDescriptorSet> WriteDescriptorSet;

		/*
			Para hacer más cosas hay que ver de organizar tanto los descriptor sets pertinentes como los binding points 
		*/

		for (size_t i = 0; i < NumImages; i++) {

			VkWriteDescriptorSet wds = {};
			wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			wds.dstSet = m_descriptorSets[i];
			wds.dstBinding = 0;
			wds.dstArrayElement = 0;
			wds.descriptorCount = 1;
			wds.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			wds.pBufferInfo = &BufferInfo_VB;

			WriteDescriptorSet.push_back(wds);


			if (UniformBuffers.size() > 0) {
				VkDescriptorBufferInfo BufferInfo_Uniform = {};
				BufferInfo_Uniform.buffer = UniformBuffers[i].m_buffer;
				BufferInfo_Uniform.offset = 0;
				BufferInfo_Uniform.range = (VkDeviceSize)UniformDataSize;

				VkWriteDescriptorSet wds_u = {};
				wds_u.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				wds_u.dstSet = m_descriptorSets[i];
				wds_u.dstBinding = 1;
				wds_u.dstArrayElement = 0;
				wds_u.descriptorCount = 1;
				wds_u.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				wds_u.pBufferInfo = &BufferInfo_Uniform;
				 
				WriteDescriptorSet.push_back(wds_u);
			}
		}
		vkUpdateDescriptorSets(m_device, (uint32_t)WriteDescriptorSet.size(), WriteDescriptorSet.data(), 0, NULL);

	}
}