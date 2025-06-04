#include "core_glfw.h"

namespace core {

	void GLFW_KeyCallback(GLFWwindow* pWindow, int Key, int Scancode, int Action, int Mods) {

		GLFWcallbacks* pGLFWCallbacks = (GLFWcallbacks*)glfwGetWindowUserPointer(pWindow);

		pGLFWCallbacks->Key(pWindow, Key, Scancode, Action, Mods);
	}

	void GLFW_MouseCallback(GLFWwindow* pWindow, double xpos, double ypos) {

		GLFWcallbacks* pGLFWCallbacks = (GLFWcallbacks*)glfwGetWindowUserPointer(pWindow);

		pGLFWCallbacks->MouseMove(pWindow, xpos, ypos);
	}

	void GLFW_MouseButtonCallback(GLFWwindow* pWindow, int Button, int Action, int Mods) {

		GLFWcallbacks* pGLFWCallbacks = (GLFWcallbacks*)glfwGetWindowUserPointer(pWindow);

		pGLFWCallbacks->MouseButton(pWindow, Button, Action, Mods);
	}

	GLFWwindow* glwf_vulkan_init(int Width, int Height, const char* pTitle) {

		if (!glfwInit())
			exit(EXIT_FAILURE);
		if (!glfwVulkanSupported())
			exit(EXIT_FAILURE);


		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, 0);

		GLFWwindow* pWindow = glfwCreateWindow(Width, Height, pTitle, NULL, NULL);
		if (!pWindow) {
			glfwTerminate();
			exit(EXIT_FAILURE);
		}
		return pWindow;
	}

	void glfw_vulkan_set_callbacks(GLFWwindow* pWindow, GLFWcallbacks* pCallbacks) {

		glfwSetWindowUserPointer(pWindow, pCallbacks);

		glfwSetKeyCallback(pWindow, GLFW_KeyCallback);
		glfwSetCursorPosCallback(pWindow, GLFW_MouseCallback);
		glfwSetMouseButtonCallback(pWindow, GLFW_MouseButtonCallback);
	}
}