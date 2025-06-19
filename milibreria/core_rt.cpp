#include "core_rt.h"
#include "utils.h"

namespace core {
	//--------------------------------------------------------------------------------------------------
	// Initialize Vulkan ray tracing
	// #VKRay

	Raytracer::Raytracer() {
	}
	Raytracer::~Raytracer(){}

	void Raytracer::setup(VkDevice dev) {
		m_device = dev;
	}

	void Raytracer::initRayTracing(core::PhysicalDevice physdev)
	{
		// Requesting ray tracing properties
		VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		prop2.pNext = &m_rtProperties;
		vkGetPhysicalDeviceProperties2(physdev.m_physDevice, &prop2);
	}

	//--------------------------------------------------------------------------------------------------
// Convert an OBJ model into the ray tracing geometry used to build the BLAS
//
	auto Raytracer::objectToVkGeometryKHR(const core::SimpleMesh& model)
	{
		// BLAS builder requires raw device addresses.
		VkDeviceAddress vertexAddress = GetBufferDeviceAddress(m_device, model.m_vb.m_buffer);
		VkDeviceAddress indexAddress = GetBufferDeviceAddress(m_device, model.m_indexbuffer.m_buffer);

		uint32_t maxPrimitiveCount = model.m_vertexBufferSize / 3;

		// Describe buffer as array of VertexObj.
		VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
		triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
		triangles.vertexData.deviceAddress = vertexAddress;
		triangles.vertexStride = sizeof(core::VertexObj);
		// Describe index data (32-bit unsigned int)
		triangles.indexType = VK_INDEX_TYPE_UINT32;
		triangles.indexData.deviceAddress = indexAddress;
		// Indicate identity transform by setting transformData to null device pointer.
		//triangles.transformData = {};
		triangles.maxVertex = model.m_vertexBufferSize - 1;

		// Identify the above data as containing opaque triangles.
		VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
		asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
		asGeom.geometry.triangles = triangles;

		// The entire array will be used to build the BLAS.
		VkAccelerationStructureBuildRangeInfoKHR offset;
		offset.firstVertex = 0;
		offset.primitiveCount = maxPrimitiveCount;
		offset.primitiveOffset = 0;
		offset.transformOffset = 0;

		// Our blas is made from only one geometry, but could be made of many geometries
		core::BlasInput input;
		input.asGeometry.emplace_back(asGeom);
		input.asBuildOffsetInfo.emplace_back(offset);

		return input;
	}

	void Raytracer::createBottomLevelAS(std::vector<core::SimpleMesh> meshes)
	{
		// BLAS - Storing each primitive in a geometry
		std::vector<core::BlasInput> allBlas;
		allBlas.reserve(meshes.size());
		for (const auto& obj : meshes)
		{
			auto blas = objectToVkGeometryKHR(obj);

			// We could add more geometry in each BLAS, but we add only one for now
			allBlas.emplace_back(blas);
		}
		//m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}
}