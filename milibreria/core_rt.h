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
#include "3rdParty/stb_image_write.h"

#include <cassert>

namespace core {

	struct BlasInput
	{
		// Data used to build acceleration structure geometry
		std::vector<VkAccelerationStructureGeometryKHR>       asGeometry;
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> asBuildOffsetInfo;
		VkBuildAccelerationStructureFlagsKHR                  flags{ 0 };
	};
	
	struct TlasInput {
		std::vector<VkAccelerationStructureInstanceKHR> instances;
		VkBuildAccelerationStructureFlagsKHR flags = 0;
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
		//void addGeometry(const VkAccelerationStructureGeometryKHR& asGeom, const VkAccelerationStructureBuildRangeInfoKHR& offset);
		//void addGeometry(const AccelerationStructureGeometryInfo& asGeom);
		// Configures the build information and calculates the necessary size information.
		VkAccelerationStructureBuildSizesInfoKHR finalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags, PFN_vkGetAccelerationStructureBuildSizesKHR pfnGetBuildSizes);
	};
	struct AccelerationStructure {
		VkAccelerationStructureKHR handle = VK_NULL_HANDLE;
		core::BufferMemory buffer;
		VkDeviceAddress address = 0;
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
		void initRayTracing(core::PhysicalDevice physdev, VkDevice* dev);
		void setup( VkCommandPool pool, core::VulkanCore* core);
		void createBottomLevelAS(std::vector<core::SimpleMesh> meshes);
		void createTopLevelAS();




		// Método helper para limpiar recursos
		void cleanup() {
			if (m_tlas.handle != VK_NULL_HANDLE) {
				vkDestroyAccelerationStructureKHR(*m_device, m_tlas.handle, nullptr);
				vkDestroyBuffer(*m_device, m_tlas.buffer.m_buffer, nullptr);
				vkFreeMemory(*m_device, m_tlas.buffer.m_mem, nullptr);
			}

			if (m_instBuffer.m_buffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(*m_device, m_instBuffer.m_buffer, nullptr);
				vkFreeMemory(*m_device, m_instBuffer.m_mem, nullptr);
			}
			for (auto& blas : m_blas) {
				if (blas.handle != VK_NULL_HANDLE) {
					vkDestroyAccelerationStructureKHR(m_device[0], blas.handle, nullptr);
				}
				//implementar destroybuffer o simplemente
				vkDestroyBuffer(m_device[0], blas.buffer.m_buffer, NULL);
			}
			m_blas.clear();
			vkDestroyDescriptorPool(*m_device, m_rtDescPool, nullptr);
			vkDestroyDescriptorSetLayout(*m_device, m_rtDescSetLayout, nullptr);
			m_outTexture->Destroy(*m_device);
		}
		void createRtDescriptorSet();

		void createRtPipeline(VkShaderModule rgenModule, VkShaderModule rmissModule, VkShaderModule rchitModule);
		void createRtShaderBindingTable();
		void raytrace(VkCommandBuffer cmdBuf, int width, int height);
		void render(int width, int height);
	private:

		void createOutImage(int windowwidth, int windowheight);

		void loadRayTracingFunctions();
		auto objectToVkGeometryKHR(const core::SimpleMesh& model);
		void buildBlas(std::vector<core::BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags);
		void buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
			VkBuildAccelerationStructureFlagsKHR flags);
		
		
		void CreateRtDescriptorPool(int NumImages);
		void CreateRtDescriptorSetLayout();
		void AllocateRtDescriptorSet();
		void WriteAccStructure();

		void saveImageToPNG(const std::string& filename, int width, int height);
		void createStagingBuffer(VkDeviceSize size, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		core::PhysicalDevice m_physicaldevice;
		VkDevice* m_device;
		VkCommandPool m_cmdBufPool;
		// Nuevos miembros para almacenar las BLAS creadas
		std::vector<AccelerationStructure> m_blas;

		core::AccelerationStructure m_tlas;
		core::BufferMemory m_instBuffer; // Buffer para las instancias

		VulkanCore* m_vkcore;

		core::VulkanTexture* m_outTexture;

		int windowwidth, windowheight;

		// Ray tracing function pointers
		PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR;
		PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR;
		PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR;
		PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR;
		PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR;
		PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR;
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR;
		PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR;

		//Descriptor Sets
		//nvvk::DescriptorSetBindings                     m_rtDescSetLayoutBind;
		VkDescriptorPool                                m_rtDescPool;
		VkDescriptorSetLayout                           m_rtDescSetLayout;
		VkDescriptorSet                                 m_rtDescSet;

		VkPipeline m_rtPipeline = VK_NULL_HANDLE;
		VkPipelineLayout m_rtPipelineLayout = VK_NULL_HANDLE;
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;

		// Shader Binding Table
		core::BufferMemory m_rtSBTBuffer;
		VkStridedDeviceAddressRegionKHR m_rgenRegion{};
		VkStridedDeviceAddressRegionKHR m_missRegion{};
		VkStridedDeviceAddressRegionKHR m_hitRegion{};
		VkStridedDeviceAddressRegionKHR m_callRegion{};

	};


}
