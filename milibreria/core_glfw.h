#pragma once

#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace core {
	class GLFWcallbacks {
	public:
		virtual void Key(GLFWwindow* pWindow, int Key, int Scancode, int Action, int Mods) = 0;
		virtual void MouseMove(GLFWwindow* pWindow, double xpos, double ypos) = 0;
		virtual void MouseButton(GLFWwindow* pWindow, int Button, int Action, int Mods) = 0;
	};

	GLFWwindow* glwf_vulkan_init(int Width, int Height, const char* pTitle);

	void glfw_vulkan_set_callbacks(GLFWwindow* pWindow, GLFWcallbacks* pCallbacks);
}