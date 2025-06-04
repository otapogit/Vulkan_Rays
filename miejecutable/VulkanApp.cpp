#include <core.h>
#include <core_shader.h>
#include <core_graphics_pipeline.h>
#include <core_simple_mesh.h>
#include <core_glfw.h>
#include <iostream>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include "core_fpcamera.h"

#define WINDOW_HEIGHT 1080
#define WINDOW_WIDTH 1920


class VulkanApp : public core::GLFWcallbacks {
public:

	VulkanApp(int WindowWidth, int WindowHeight) {
		m_windowWidth = WindowWidth;
		m_windowHeight = WindowHeight;
	}

	~VulkanApp() {
		m_vkcore.FreeCommandBuffers((uint32_t)m_cmdBufs.size(), m_cmdBufs.data());

		vkDestroyShaderModule(m_vkcore.GetDevice(), m_vs, NULL);
		vkDestroyShaderModule(m_vkcore.GetDevice(), m_fs, NULL);
		vkDestroyRenderPass(m_vkcore.GetDevice(), m_renderPass, NULL);
		delete m_pipeline;
		m_mesh.Destroy(m_device);
		m_mesh1.Destroy(m_device);
		for (int i = 0; i < m_uniformBuffers.size(); i++) {
			m_uniformBuffers[i].Destroy(m_device);
		}
		
	}

	void Init(const char* pAppName) {
		m_pWindow = core::glwf_vulkan_init(WINDOW_WIDTH, WINDOW_HEIGHT, pAppName);
		m_vkcore.Init(pAppName, m_pWindow);
		
		m_device = m_vkcore.GetDevice();
		m_numImages = m_vkcore.GetNumImages();



		m_pQueue = m_vkcore.GetQueue();

		m_renderPass = m_vkcore.CreateSimpleRenderPass();
		m_frameBuffers = m_vkcore.CreateFrameBuffers(m_renderPass);

		CreateShaders();

		CreateVertexBuffer();
		CreateVertexBuffer2();

		CreateUniformBuffers();

		CreatePipeline();

		CreateCamera();

		CreateCommandBuffers();
		RecordCommandBuffers();

		core::glfw_vulkan_set_callbacks(m_pWindow, this);
	}

	void RenderScene() {
		uint32_t ImageIndex = m_pQueue->AcquireNextImage();
		UpdateUniformBuffers(ImageIndex);
		m_pQueue->SubmitAsync(m_cmdBufs[ImageIndex]);
		
		m_pQueue->Present(ImageIndex);
	}

	void Execute() {
		float CurTime = (float)glfwGetTime();

		while (!glfwWindowShouldClose(m_pWindow)) {
			float Time = (float)glfwGetTime();
			float dt = Time - CurTime;
			m_pCamera->Update(dt);
			RenderScene();
			CurTime = Time;
			glfwPollEvents();
		}
		glfwTerminate();
	}

	//Callback
	void Key(GLFWwindow* pWindow, int Key, int Scancode, int Action, int Mods) {
		if ((Key == GLFW_KEY_ESCAPE) && (Action == GLFW_PRESS)) {

			glfwSetWindowShouldClose(pWindow, GLFW_TRUE);
		}

		bool handled = m_pCamera->GLFWCameraHandler(m_pCamera->m_movement, Key, Action, Mods);


	}
	void MouseMove(GLFWwindow* pWindow, double xpos, double ypos) {
		
		m_pCamera->m_mouseState.m_pos.x = (float)xpos / (float)WINDOW_WIDTH;
		m_pCamera->m_mouseState.m_pos.y = (float)ypos / (float)WINDOW_HEIGHT;
	}
	void MouseButton(GLFWwindow* pWindow, int Button, int Action, int Mods) {
	
		if (Button == GLFW_MOUSE_BUTTON_LEFT) {
			m_pCamera->m_mouseState.m_buttonPressed = (Action == GLFW_PRESS);
		}
	}

	//igual esto que en ele shader
	struct UniformData {
		glm::mat4 MVP;
	};


private:
	void CreateUniformBuffers() {
		m_uniformBuffers = m_vkcore.CreateUniformBuffers(sizeof(UniformData));
	}

	void CreateCommandBuffers() {
		m_cmdBufs.resize(m_numImages);
		m_vkcore.CreateCommandBuffer(m_numImages, m_cmdBufs.data());

	}



	void CreatePipeline() {

		m_pipeline = new core::GraphicsPipeline(m_vkcore.GetDevice(), m_pWindow, m_renderPass,&m_mesh,m_numImages,m_uniformBuffers,sizeof(UniformData), m_vs, m_fs);
	}

	void CreateShaders() {
		m_vs = core::CreateShaderModuleFromText(m_vkcore.GetDevice(), "test.vert");
		m_fs = core::CreateShaderModuleFromText(m_vkcore.GetDevice(), "test.frag");
	}


	void CreateVertexBuffer() {
		struct Vertex {
			Vertex(const glm::vec3& p, const glm::vec2& t) {
				Pos = p;
				Tex = t;
			}
			glm::vec3 Pos;
			glm::vec2 Tex;
		};

		std::vector<Vertex> vertices = {
			Vertex({-1.0f,-1.0f,0.0f},{0.0f,0.0f}),
			Vertex({1.0f,-1.0f,0.0f},{0.0f,1.0f}),
			Vertex({0.0f,0.0f,0.0f},{1.0f,1.0f}),
			Vertex({1.0f,1.0f,0.0f},{0.0f,0.0f}),
			Vertex({-1.0f,1.0f,0.0f},{0.0f,1.0f}),
			Vertex({0.0f,0.0f,0.0f},{1.0f,1.0f}),
		};

		m_mesh.m_vertexBufferSize = sizeof(vertices[0]) * vertices.size();
		m_mesh.m_vb = m_vkcore.CreateVertexBuffer(vertices.data(), m_mesh.m_vertexBufferSize);
		m_mesh.vertexcount = vertices.size();
	}

	void CreateVertexBuffer2() {
		struct Vertex {
			Vertex(const glm::vec3& p, const glm::vec2& t) {
				Pos = p;
				Tex = t;
			}
			glm::vec3 Pos;
			glm::vec2 Tex;
		};

		std::vector<Vertex> vertices = {
			Vertex({-4.0f,-2.0f,0.0f},{0.0f,0.0f}),
			Vertex({-2.0f,-2.0f,0.0f},{0.0f,1.0f}),
			Vertex({-3.0f,-3.0f,0.0f},{1.0f,1.0f}),
			Vertex({-2.0f,-2.0f,0.0f},{0.0f,0.0f}),
			Vertex({-4.0f,-2.0f,0.0f},{0.0f,1.0f}),
			Vertex({-3.0f,-3.0f,0.0f},{1.0f,1.0f}),
		};

		m_mesh1.m_vertexBufferSize = sizeof(vertices[0]) * vertices.size();
		m_mesh1.m_vb = m_vkcore.CreateVertexBuffer(vertices.data(), m_mesh1.m_vertexBufferSize);
		m_mesh1.vertexcount = vertices.size();
	}

	void UpdateUniformBuffers(uint32_t ImageIndex) {
		/*static float foo = 0.0f;
		glm::mat4 Rotate = glm::mat4(1.0);
		Rotate = glm::rotate(Rotate, glm::radians(foo), glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f)));
		foo += 0.005f;
		*/
		glm::mat4 WVP = m_pCamera->GetVPMatrix();

		//glm::mat4 WVP = Rotate;
		m_uniformBuffers[ImageIndex].Update(m_device, &WVP, sizeof(WVP));
	}

	void RecordCommandBuffers() {
		VkClearColorValue ClearColor = { 1.0f,0.6f,0.6f,0.0f };

		/*
		VkImageSubresourceRange ImageRange = {};

		VkImageMemoryBarrier PresentToClearBarrier = {};

		VkImageMemoryBarrier ClearToPresentBarrier = {};

		*/

		VkClearValue ClearValue;
		ClearValue.color = ClearColor;

		VkRenderPassBeginInfo RenderPassBeginInfo = {};
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.pNext = NULL;
		RenderPassBeginInfo.renderPass = m_renderPass;
		VkRect2D rect;
		rect.offset = { 0,0 };
		rect.extent = { WINDOW_WIDTH,WINDOW_HEIGHT-25 };
		RenderPassBeginInfo.renderArea = rect;
		RenderPassBeginInfo.clearValueCount = 1;
		RenderPassBeginInfo.pClearValues = &ClearValue;



		for (uint i = 0; i < m_cmdBufs.size(); i++) {

			//PresentToClearBarrier.image = m_vkcore.GetImage(i);
			//ClearToPresentBarrier.image = m_vkcore.GetImage(i);

			core::BeginCommandBuffer(m_cmdBufs[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
			/*
			vkCmdPipelineBarrier(m_cmdBufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, //dependency flags
				0, NULL, // memory barriers
				0, NULL, //BufferMemory barriers
				1, &PresentToClearBarrier); // Image memory barriers

			vkCmdClearColorImage(m_cmdBufs[i], m_vkcore.GetImage(i), VK_IMAGE_LAYOUT_GENERAL, &ClearColor, 1, &ImageRange);

			vkCmdPipelineBarrier(m_cmdBufs[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, NULL, 0, NULL, 1, &ClearToPresentBarrier);
			*/

			RenderPassBeginInfo.framebuffer = m_frameBuffers[i];

			vkCmdBeginRenderPass(m_cmdBufs[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			m_pipeline->Bind(m_cmdBufs[i],(int)i);

			uint32_t VertexCount = m_mesh.vertexcount;
			uint32_t InstanceCount = 1;
			uint32_t FirstVertex = 0;
			uint32_t FirstInstance = 0;

			vkCmdDraw(m_cmdBufs[i], VertexCount, InstanceCount, FirstVertex, FirstInstance);

			vkCmdEndRenderPass(m_cmdBufs[i]);

			VkResult res = vkEndCommandBuffer(m_cmdBufs[i]);
			CHECK_VK_RESULT(res, "vkEndCommandBuffer\n");
		}
		
		printf("Command buffers recorded\n");
	}

	void CreateCamera() {
		
		float FOV = 45.0f;
		float znear = 0.1f;
		float zfar = 1000.0f;
		CreateCamera(FOV, znear, zfar);
	}

	void CreateCamera(float FOV, float znear, float zfar) {
		if ((m_windowWidth == 0) || (m_windowHeight == 0)) {
			printf("Invalid window dims");
			exit(1);
		}
		glm::vec3 Pos(1.0f, 1.0f, 1.0f);
		glm::vec3 Target(0.0f, 0.0f, 0.0f);
		glm::vec3 Up(0.0f, 1.0f, 0.0f);

		m_pCamera = new CameraFirstPerson(Pos, Target, Up, FOV, m_windowWidth, m_windowHeight, znear, zfar);
	}

	float m_windowWidth, m_windowHeight;

	core::VulkanQueue* m_pQueue;
	core::VulkanCore m_vkcore;
	VkDevice m_device = NULL;
	int m_numImages = 0;
	std::vector<VkCommandBuffer> m_cmdBufs;
	GLFWwindow* m_pWindow;
	VkRenderPass m_renderPass;
	std::vector<VkFramebuffer> m_frameBuffers;
	VkShaderModule m_vs;
	VkShaderModule m_fs;
	core::GraphicsPipeline* m_pipeline = NULL;
	core::SimpleMesh m_mesh, m_mesh1;
	CameraFirstPerson* m_pCamera = NULL;

	std::vector<core::BufferMemory> m_uniformBuffers;
};