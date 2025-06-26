#include "core_rt.h"
#include "utils.h"

namespace core {
	//--------------------------------------------------------------------------------------------------
	// Initialize Vulkan ray tracing
	// #VKRay

	Raytracer::Raytracer() {}

	Raytracer::~Raytracer(){
    }

	/*
	* Metodo para pasar todos los componentes necesarios del core aqui
	* Quiero mantener encapsulado de cierta manera el raytracing
	*/
	void Raytracer::setup( VkCommandPool pool, core::VulkanCore* core) {

		m_cmdBufPool = pool;
        m_vkcore = core;
        loadRayTracingFunctions();
	}

	void Raytracer::initRayTracing(core::PhysicalDevice physdev, VkDevice* dev)
	{
        m_device = dev;
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
		VkDeviceAddress vertexAddress = GetBufferDeviceAddress(*m_device, model.m_vb.m_buffer);
		VkDeviceAddress indexAddress = GetBufferDeviceAddress(*m_device, model.m_indexbuffer.m_buffer);

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

        printf("BLAS created for %d triangles\n", maxPrimitiveCount);

		return input;
	}

	// También necesitarás actualizar tu método createBottomLevelAS:
	void Raytracer::createBottomLevelAS(std::vector<core::SimpleMesh> meshes) {
		// BLAS - Storing each primitive in a geometry
		std::vector<core::BlasInput> allBlas;
		allBlas.reserve(meshes.size());

        printf("\n");

		for (const core::SimpleMesh& obj : meshes) {
            //Da error aqui
			auto blas = objectToVkGeometryKHR(obj);
			allBlas.emplace_back(blas);
		}


		// Ahora puedes llamar a tu implementación
		buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	}

    void Raytracer::buildBlas(std::vector<core::BlasInput>& input, VkBuildAccelerationStructureFlagsKHR flags) {
        uint32_t nbBlas = static_cast<uint32_t>(input.size());
        VkDeviceSize maxScratchSize{ 0 };

        // Preparar la información para los comandos de construcción de acceleration structures
        std::vector<core::AccelerationStructureBuildData> buildAs(nbBlas);
        std::vector<VkAccelerationStructureBuildSizesInfoKHR> buildSizes(nbBlas);

        // 1. Preparar información de construcción para cada BLAS
        for (uint32_t idx = 0; idx < nbBlas; idx++) {
            buildAs[idx].asType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildAs[idx].asGeometry = input[idx].asGeometry;
            buildAs[idx].rangeInfo = input[idx].asBuildOffsetInfo;

            // Finalizar geometría y obtener información de tamaños
            buildSizes[idx] = buildAs[idx].finalizeGeometry(*m_device, input[idx].flags | flags, vkGetAccelerationStructureBuildSizesKHR);
            maxScratchSize = std::max(maxScratchSize, buildSizes[idx].buildScratchSize);
        }

        // 2. Crear buffer de scratch
 

        core::BufferMemory blasScratchBuffer = m_vkcore[0].CreateBufferBlas(maxScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

        // Obtener la dirección del device del buffer de scratch
        VkDeviceAddress scratchAddress = GetBufferDeviceAddress(*m_device, blasScratchBuffer.m_buffer);

        // 3. Crear y construir cada BLAS
        m_blas.resize(nbBlas);

        for (uint32_t idx = 0; idx < nbBlas; idx++) {
            // Crear buffer para almacenar la acceleration structure


            core::BufferMemory asBuffer = m_vkcore[0].CreateBufferBlas(buildSizes[idx].accelerationStructureSize,
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // Crear la acceleration structure
            VkAccelerationStructureCreateInfoKHR createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            createInfo.buffer = asBuffer.m_buffer;
            createInfo.size = buildSizes[idx].accelerationStructureSize;
            createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;


            VkAccelerationStructureKHR accelerationStructure;
            VkResult result = vkCreateAccelerationStructureKHR(*m_device, &createInfo, nullptr, &accelerationStructure);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("Failed to create acceleration structure");
            }

            // Guardar en tu estructura de datos (asumiendo que tienes una estructura para almacenar BLAS)
            core::AccelerationStructure blas;
            blas.handle = accelerationStructure;
            blas.buffer = asBuffer;

            // Obtener la dirección de la acceleration structure
            VkAccelerationStructureDeviceAddressInfoKHR addressInfo = {};
            addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            addressInfo.accelerationStructure = accelerationStructure;
            blas.address = vkGetAccelerationStructureDeviceAddressKHR(*m_device, &addressInfo);

            m_blas.push_back(blas);

            // 4. Construir la acceleration structure
            VkCommandBufferAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = m_cmdBufPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(*m_device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            // Configurar la información de construcción
            buildAs[idx].buildInfo.dstAccelerationStructure = accelerationStructure;
            buildAs[idx].buildInfo.scratchData.deviceAddress = scratchAddress;

            // Preparar punteros a la información de rangos
            std::vector<VkAccelerationStructureBuildRangeInfoKHR*> buildRangeInfos(buildAs[idx].rangeInfo.size());
            for (size_t i = 0; i < buildAs[idx].rangeInfo.size(); i++) {
                buildRangeInfos[i] = &buildAs[idx].rangeInfo[i];
            }

            // Construir la acceleration structure
            vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildAs[idx].buildInfo, buildRangeInfos.data());

            // Añadir barrier para asegurar que la construcción termine antes de la siguiente
            VkMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
            barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
                0, 1, &barrier, 0, nullptr, 0, nullptr);

            vkEndCommandBuffer(commandBuffer);

            // Submit y esperar
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            // Asumiendo que tienes acceso a la queue - podrías necesitar pasarla como parámetro
            core::VulkanQueue* m_pQueue = m_vkcore[0].GetQueue();

            m_pQueue->SubmitSync(commandBuffer);
            m_pQueue->WaitIdle();

            // Liberar command buffer
            vkFreeCommandBuffers(*m_device, m_cmdBufPool, 1, &commandBuffer);
        }

        // 5. Limpiar buffer de scratch
        vkDestroyBuffer(*m_device, blasScratchBuffer.m_buffer, NULL);
    }

    static VkTransformMatrixKHR toTransformMatrixKHR(glm::mat4 matrix)
    {
        // VkTransformMatrixKHR uses a row-major memory layout, while glm::mat4
        // uses a column-major memory layout. We transpose the matrix so we can
        // memcpy the matrix's data directly.
        glm::mat4            temp = glm::transpose(matrix);
        VkTransformMatrixKHR out_matrix;
        memcpy(&out_matrix, &temp, sizeof(VkTransformMatrixKHR));
        return out_matrix;
    }

    void Raytracer::createTopLevelAS()
    {
        std::vector<VkAccelerationStructureInstanceKHR> tlas;
        //tlas.reserve(m_instances.size());
        tlas.reserve(2);
        //for (const HelloVulkan::ObjInstance& inst : m_instances)
        //{
            VkAccelerationStructureInstanceKHR rayInst{};
            rayInst.transform = toTransformMatrixKHR(glm::mat4(1.0f));  // Position of the instance
            rayInst.instanceCustomIndex = 0;                               // gl_InstanceCustomIndexEXT  NI IDEA DE COMO VA
            rayInst.accelerationStructureReference = m_blas[0].address;
            rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            rayInst.mask = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
            rayInst.instanceShaderBindingTableRecordOffset = 0;  // We will use the same hit group for all objects
            tlas.emplace_back(rayInst);
        //}
        //m_rtBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
    } 

    VkAccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildData::finalizeGeometry(VkDevice device, VkBuildAccelerationStructureFlagsKHR flags, PFN_vkGetAccelerationStructureBuildSizesKHR pfnGetBuildSizes)
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

        pfnGetBuildSizes(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo,
            maxPrimCount.data(), &sizeInfo);

        return sizeInfo;
    }

    // En tu archivo .cpp, implementa el método:
    void Raytracer::loadRayTracingFunctions() {
        // Cargar funciones de acceleration structure
        vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
            vkGetDeviceProcAddr(*m_device, "vkCreateAccelerationStructureKHR"));

        vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(
            vkGetDeviceProcAddr(*m_device, "vkDestroyAccelerationStructureKHR"));

        vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
            vkGetDeviceProcAddr(*m_device, "vkGetAccelerationStructureBuildSizesKHR"));

        vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
            vkGetDeviceProcAddr(*m_device, "vkGetAccelerationStructureDeviceAddressKHR"));

        vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
            vkGetDeviceProcAddr(*m_device, "vkCmdBuildAccelerationStructuresKHR"));

        vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(
            vkGetDeviceProcAddr(*m_device, "vkBuildAccelerationStructuresKHR"));

        // Cargar funciones de ray tracing pipeline
        vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
            vkGetDeviceProcAddr(*m_device, "vkCmdTraceRaysKHR"));

        vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
            vkGetDeviceProcAddr(*m_device, "vkGetRayTracingShaderGroupHandlesKHR"));

        vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(
            vkGetDeviceProcAddr(*m_device, "vkCreateRayTracingPipelinesKHR"));

        // Verificar que todas las funciones se cargaron correctamente
        if (!vkCreateAccelerationStructureKHR || !vkDestroyAccelerationStructureKHR ||
            !vkGetAccelerationStructureBuildSizesKHR || !vkGetAccelerationStructureDeviceAddressKHR ||
            !vkCmdBuildAccelerationStructuresKHR || !vkBuildAccelerationStructuresKHR ||
            !vkCmdTraceRaysKHR || !vkGetRayTracingShaderGroupHandlesKHR ||
            !vkCreateRayTracingPipelinesKHR) {
            throw std::runtime_error("Failed to load ray tracing functions!");
        }
    }
}