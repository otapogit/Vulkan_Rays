#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "core.h"
#include "core_simple_mesh.h"
#include "core_vertex.h"
#include "utils.h"
#include <cassert>

namespace core {

	struct BlasInput
	{
		// Data used to build acceleration structure geometry
		std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
		VkBuildAccelerationStructureFlagsKHR                  flags{ 0 };
	};
	
	class Raytracer {
	public:
		Raytracer();
		~Raytracer();
		// #VKRay
		void initRayTracing(core::PhysicalDevice physdev);
		void setup(VkDevice dev);
		void createBottomLevelAS(std::vector<core::SimpleMesh> meshes);
		auto objectToVkGeometryKHR(const core::SimpleMesh& model);
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		core::PhysicalDevice m_physicaldevice;
		VkDevice m_device;
	};


}
