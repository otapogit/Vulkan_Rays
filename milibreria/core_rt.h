#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "core.h"
#include "utils.h"
#include <cassert>

namespace core {
	class Raytracer {
	public:
		Raytracer();
		~Raytracer();
		// #VKRay
		void                                            initRayTracing(core::PhysicalDevice physdev);
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		core::PhysicalDevice m_physicaldevice;

	};
}
