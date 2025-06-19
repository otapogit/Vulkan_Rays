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
	
	struct AccelerationStructureBuildData
	{
		VkAccelerationStructureTypeKHR asType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;  // Mandatory to set

		// Collection of geometries for the acceleration structure.
		std::vector<VkAccelerationStructureGeometryKHR> asGeometry;
		// Build range information corresponding to each geometry.
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfo;
		// Build information required for acceleration structure.
		VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
		// Size information for acceleration structure build resources.
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

		// Adds a geometry with its build range information to the acceleration structure.
		void addGeometry(const VkAccelerationStructureGeometryKHR& asGeom, const VkAccelerationStructureBuildRangeInfoKHR& offset);
		void addGeometry(const AccelerationStructureGeometryInfo& asGeom);
		// Configures the build information and calculates the necessary size information.
		VkAccelerationStructureBuildSizesInfoKHR finalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags)
		{
			assert(asGeometry.size() > 0 && "No geometry added to Build Structure");
			assert(asType != VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR && "Acceleration Structure Type not set");

			buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			buildInfo.type = asType;
			buildInfo.flags = flags;
			buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
			buildInfo.dstAccelerationStructure = VK_NULL_HANDLE;
			buildInfo.geometryCount = static_cast<uint32_t>(asGeometry.size());
			buildInfo.pGeometries = asGeometry.data();
			buildInfo.ppGeometries = nullptr;
			buildInfo.scratchData.deviceAddress = 0;

			std::vector<uint32_t> maxPrimCount(rangeInfo.size());
			for (size_t i = 0; i < rangeInfo.size(); ++i)
			{
				maxPrimCount[i] = rangeInfo[i].primitiveCount;
			}

			vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
				maxPrimCount.data(), &sizeInfo);

			return sizeInfo;
		}
	};
	// Single Geometry information, multiple can be used in a single BLAS
	struct AccelerationStructureGeometryInfo
	{
		VkAccelerationStructureGeometryKHR       geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
	};
	class Raytracer {
	public:
		Raytracer();
		~Raytracer();
		// #VKRay
		void initRayTracing(core::PhysicalDevice physdev);
		void setup(VkDevice dev, VkCommandPool pool);
		void createBottomLevelAS(std::vector<core::SimpleMesh> meshes);
		auto objectToVkGeometryKHR(const core::SimpleMesh& model);
		void buildBlas(const std::vector<core::BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags);
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		core::PhysicalDevice m_physicaldevice;
		VkDevice m_device;
		VkCommandPool m_cmdBufPool;
	};


}
