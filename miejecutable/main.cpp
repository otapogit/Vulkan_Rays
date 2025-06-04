

#include <core.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include "VulkanApp.cpp"

#define GLFW_INCLUDE_VULKAN
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define APP_NAME "VulkanApp"

//VulkanApp App;

void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) {

		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}





int main(int argc, char* argv[]) {

	VulkanApp App(WINDOW_WIDTH, WINDOW_HEIGHT);

	App.Init(APP_NAME);

	App.Execute();


	return 0;
}