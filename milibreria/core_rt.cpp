#include "core_rt.h"
#include "utils.h"
#include "core_shader.h"
#include <array>

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
    * 
    * Componentes necesarios por ahora: Core, MEshes, tamaño textura de salida
	*/
	void Raytracer::setup( VkCommandPool pool, core::VulkanCore* core) {

		m_cmdBufPool = pool;
        m_vkcore = core;
        loadRayTracingFunctions();
        m_outTexture = new core::VulkanTexture();
        createOutImage(800,800);
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
                                            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
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
        std::vector<VkAccelerationStructureInstanceKHR> instances;

        // Crear instancia para cada BLAS
        for (size_t i = 0; i < m_blas.size(); i++) {
            VkAccelerationStructureInstanceKHR instance{};
            
            instance.transform = toTransformMatrixKHR(glm::mat4(1.0f));// Matriz de transformación (identidad por defecto)
            instance.instanceCustomIndex = static_cast<uint32_t>(i);// Índice personalizado de la instancia (accessible en shaders como gl_InstanceCustomIndexEXT)
            instance.accelerationStructureReference = m_blas[i].address;// Referencia a la BLAS
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR; // Flags de la instancia
            instance.mask = 0xFF;// Máscara para ray culling (0xFF significa que todos los rays pueden intersectar)
            instance.instanceShaderBindingTableRecordOffset = 0; // Offset en la shader binding table para hit shaders
            instances.push_back(instance);
        }
        printf("\n%d instances pre build\n", instances.size());

        buildTlas(instances, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
    } 

    void Raytracer::buildTlas(const std::vector<VkAccelerationStructureInstanceKHR>& instances,
        VkBuildAccelerationStructureFlagsKHR flags)
    {
        // 1. Crear buffer para las instancias
        VkDeviceSize instanceBufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);

        m_instBuffer = m_vkcore[0].CreateBufferBlas(
            instanceBufferSize,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
            VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
        );

        // 2. Copiar datos de instancias al buffer
        void* data;
        vkMapMemory(*m_device, m_instBuffer.m_mem, 0, instanceBufferSize, 0, &data);
        memcpy(data, instances.data(), instanceBufferSize);
        vkUnmapMemory(*m_device, m_instBuffer.m_mem);

        // 3. Configurar la geometría de instancias
        VkAccelerationStructureGeometryInstancesDataKHR instancesVk{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR
        };
        instancesVk.arrayOfPointers = VK_FALSE;
        instancesVk.data.deviceAddress = GetBufferDeviceAddress(*m_device, m_instBuffer.m_buffer);

        // 4. Configurar la geometría
        VkAccelerationStructureGeometryKHR topASGeometry{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR
        };
        topASGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        topASGeometry.geometry.instances = instancesVk;

        // 5. Preparar información de construcción
        core::AccelerationStructureBuildData buildData;
        buildData.asType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildData.asGeometry.push_back(topASGeometry);

        VkAccelerationStructureBuildRangeInfoKHR buildOffsetInfo{};
        buildOffsetInfo.primitiveCount = static_cast<uint32_t>(instances.size());
        buildOffsetInfo.primitiveOffset = 0;
        buildOffsetInfo.firstVertex = 0;
        buildOffsetInfo.transformOffset = 0;
        buildData.rangeInfo.push_back(buildOffsetInfo);

        // 6. Obtener información de tamaños
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo = buildData.finalizeGeometry(
            *m_device, flags, vkGetAccelerationStructureBuildSizesKHR
        );

        // 7. Crear buffer de scratch
        core::BufferMemory scratchBuffer = m_vkcore[0].CreateBufferBlas(
            sizeInfo.buildScratchSize,
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
        );
        VkDeviceAddress scratchAddress = GetBufferDeviceAddress(*m_device, scratchBuffer.m_buffer);

        // 8. Crear buffer para la TLAS
        core::BufferMemory tlasBuffer = m_vkcore[0].CreateBufferBlas(
            sizeInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        // 9. Crear la acceleration structure
        VkAccelerationStructureCreateInfoKHR createInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR
        };
        createInfo.buffer = tlasBuffer.m_buffer;
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

        VkResult result = vkCreateAccelerationStructureKHR(*m_device, &createInfo, nullptr, &m_tlas.handle);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create top level acceleration structure");
        }

        m_tlas.buffer = tlasBuffer;

        // 10. Obtener la dirección de la TLAS
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR
        };
        addressInfo.accelerationStructure = m_tlas.handle;
        m_tlas.address = vkGetAccelerationStructureDeviceAddressKHR(*m_device, &addressInfo);

        // 11. Construir la TLAS

        VkCommandBuffer commandBuffer;
        m_vkcore->CreateCommandBuffer(1, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{
            VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
        };
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        // Configurar la información de construcción
        buildData.buildInfo.dstAccelerationStructure = m_tlas.handle;
        buildData.buildInfo.scratchData.deviceAddress = scratchAddress;

        // Preparar punteros a la información de rangos
        VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &buildData.rangeInfo[0];

        // Construir la acceleration structure
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildData.buildInfo, &pBuildRangeInfo);

        // Añadir barrier
        VkMemoryBarrier barrier{ VK_STRUCTURE_TYPE_MEMORY_BARRIER };
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            0, 1, &barrier, 0, nullptr, 0, nullptr);

        vkEndCommandBuffer(commandBuffer);

        // Submit y esperar
        core::VulkanQueue* pQueue = m_vkcore[0].GetQueue();
        pQueue->SubmitSync(commandBuffer);
        pQueue->WaitIdle();

        // Limpiar
        vkFreeCommandBuffers(*m_device, m_cmdBufPool, 1, &commandBuffer);
        vkDestroyBuffer(*m_device, scratchBuffer.m_buffer, nullptr);
        vkFreeMemory(*m_device, scratchBuffer.m_mem, nullptr);

        printf("TLAS created with %zu instances\n", instances.size());
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

    void Raytracer::createRtDescriptorSet() {
        CreateRtDescriptorPool(1);
        printf("Creating RT descriptor set layout\n");
        CreateRtDescriptorSetLayout();
        //IMPORTANTE, A ESTE DESCRIPTOR SET SE LE DEBERÁ AÑADIR EL OTRO DESCRIPTOR SET DE INFO GENERAL DE LA ESCENA PARA QUE VAYA OK :)
        printf("Allocating RT Descriptor set\n");
        AllocateRtDescriptorSet();
        printf("Writing RT Descriptor set\n");
        WriteAccStructure();
    }

    void Raytracer::AllocateRtDescriptorSet() {
        VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocateInfo.descriptorPool = m_rtDescPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &m_rtDescSetLayout;
        vkAllocateDescriptorSets(*m_device, &allocateInfo, &m_rtDescSet);
    }


    void Raytracer::CreateRtDescriptorPool(int NumImages) {

        std::vector<VkDescriptorPoolSize> poolSizes;

        // Pool para uniform buffers
        VkDescriptorPoolSize uniformPoolSize = {};
        uniformPoolSize.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        uniformPoolSize.descriptorCount = (uint32_t)NumImages;
        poolSizes.push_back(uniformPoolSize);

        // Pool para texturas
        VkDescriptorPoolSize samplerPoolSize = {};
        samplerPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        samplerPoolSize.descriptorCount = (uint32_t)NumImages;
        poolSizes.push_back(samplerPoolSize);

        VkDescriptorPoolCreateInfo PoolInfo = {};
        PoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        //Para alocar cosas en el otro metodo cambiar flags
        PoolInfo.flags = 0;
        //Esto puede ser cambiado para pasar mas cosas
        PoolInfo.maxSets = (uint32_t)NumImages;
        //A lo mejor hay que cambiar las pools para poner las cosas en especifico tipo uniforms y tal
        PoolInfo.poolSizeCount = (uint32_t)poolSizes.size();
        PoolInfo.pPoolSizes = poolSizes.data();

        VkResult res = vkCreateDescriptorPool(*m_device, &PoolInfo, NULL, &m_rtDescPool);
        CHECK_VK_RESULT(res, "vkCreateDescriptorPool");
        printf("Descriptor pool created\n");

    }

    void Raytracer::CreateRtDescriptorSetLayout() {

        std::vector<VkDescriptorSetLayoutBinding> LayoutBindings;

        VkDescriptorSetLayoutBinding AccStructureLayoutBinding_Uniform = {};
        AccStructureLayoutBinding_Uniform.binding = 1;
        AccStructureLayoutBinding_Uniform.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        AccStructureLayoutBinding_Uniform.descriptorCount = 1;
        //Obviamente si es necesario ampliar esto
        AccStructureLayoutBinding_Uniform.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;


        LayoutBindings.push_back(AccStructureLayoutBinding_Uniform);

        // CORREGIDO: Usar la variable correcta
        VkDescriptorSetLayoutBinding FragmentShaderLayoutBinding = {};
        FragmentShaderLayoutBinding.binding = 2;
        FragmentShaderLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        FragmentShaderLayoutBinding.descriptorCount = 1;
        FragmentShaderLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        LayoutBindings.push_back(FragmentShaderLayoutBinding);


        VkDescriptorSetLayoutCreateInfo LayoutInfo = {};
        LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        LayoutInfo.pNext = NULL;
        LayoutInfo.flags = 0;
        LayoutInfo.bindingCount = (uint32_t)LayoutBindings.size();
        LayoutInfo.pBindings = LayoutBindings.data();

        VkResult res = vkCreateDescriptorSetLayout(*m_device, &LayoutInfo, NULL, &m_rtDescSetLayout);
        CHECK_VK_RESULT(res, "vkCreateDescriptorSetLayout\n");

    }

    void Raytracer::WriteAccStructure() {
        std::vector<VkWriteDescriptorSet> WriteDescriptorSet;
        //solo hay un m_rtDescSet
        VkAccelerationStructureKHR tlas = m_tlas.handle;
        VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
        descASInfo.accelerationStructureCount = 1;
        descASInfo.pAccelerationStructures = &tlas;
        VkWriteDescriptorSet wds_t = {};
        wds_t.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds_t.dstSet = m_rtDescSet;
        wds_t.dstBinding = 1;
        wds_t.dstArrayElement = 0;
        wds_t.descriptorCount = 1;
        wds_t.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        wds_t.pNext = &descASInfo;
        WriteDescriptorSet.push_back(wds_t);

        //Esto a lo mejor hay que cambiarlo xq no voy a samplear nada, es de output
        VkDescriptorImageInfo ImageInfo = {};
        ImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        ImageInfo.imageView = m_outTexture->m_view;
        ImageInfo.sampler = NULL;
        VkWriteDescriptorSet wds_i = {};
        wds_i.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        wds_i.dstSet = m_rtDescSet;
        wds_i.dstBinding = 2;
        wds_i.dstArrayElement = 0;
        wds_i.descriptorCount = 1;
        wds_i.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        wds_i.pImageInfo = &ImageInfo;
        WriteDescriptorSet.push_back(wds_i);
       
        vkUpdateDescriptorSets(*m_device, (uint32_t)WriteDescriptorSet.size(), WriteDescriptorSet.data(), 0, NULL);
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

    void Raytracer::createOutImage(int windowwidth, int windowheight) {
        VkFormat Format = VK_FORMAT_R8G8B8A8_UNORM;
        m_vkcore->CreateTextureImage(*m_outTexture, (uint32_t)windowwidth, (uint32_t)windowheight, Format);
    }

    void Raytracer::createRtPipeline(VkShaderModule rgenModule, VkShaderModule rmissModule, VkShaderModule rchitModule) {
        // 1. Cargar shaders
        enum StageIndices {
            eRaygen,
            eMiss,
            eClosestHit,
            eShaderGroupCount
        };

        std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages;

        //A lo mejor es interesante pasar los shaders desde el ejecutable

        // Raygen shader
        VkShaderModule raygenModule = rgenModule;
        stages[eRaygen].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[eRaygen].stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        stages[eRaygen].module = raygenModule;
        stages[eRaygen].pName = "main";
        stages[eRaygen].pNext = nullptr;
        stages[eRaygen].flags = 0;

        // Miss shader
        VkShaderModule missModule = rmissModule;
        stages[eMiss].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[eMiss].stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        stages[eMiss].module = missModule;
        stages[eMiss].pName = "main";
        stages[eMiss].pNext = nullptr;
        stages[eMiss].flags = 0;

        // Closest hit shader
        VkShaderModule chitModule = rchitModule;
        stages[eClosestHit].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[eClosestHit].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        stages[eClosestHit].module = chitModule;
        stages[eClosestHit].pName = "main";
        stages[eClosestHit].pNext = nullptr;
        stages[eClosestHit].flags = 0;

        // 2. Crear shader groups
        m_rtShaderGroups.resize(eShaderGroupCount);

        // Raygen group
        m_rtShaderGroups[eRaygen].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        m_rtShaderGroups[eRaygen].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        m_rtShaderGroups[eRaygen].generalShader = eRaygen;
        m_rtShaderGroups[eRaygen].closestHitShader = VK_SHADER_UNUSED_KHR;
        m_rtShaderGroups[eRaygen].anyHitShader = VK_SHADER_UNUSED_KHR;
        m_rtShaderGroups[eRaygen].intersectionShader = VK_SHADER_UNUSED_KHR;

        // Miss group
        m_rtShaderGroups[eMiss].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        m_rtShaderGroups[eMiss].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        m_rtShaderGroups[eMiss].generalShader = eMiss;
        m_rtShaderGroups[eMiss].closestHitShader = VK_SHADER_UNUSED_KHR;
        m_rtShaderGroups[eMiss].anyHitShader = VK_SHADER_UNUSED_KHR;
        m_rtShaderGroups[eMiss].intersectionShader = VK_SHADER_UNUSED_KHR;

        // Hit group
        m_rtShaderGroups[eClosestHit].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
        m_rtShaderGroups[eClosestHit].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        m_rtShaderGroups[eClosestHit].generalShader = VK_SHADER_UNUSED_KHR;
        m_rtShaderGroups[eClosestHit].closestHitShader = eClosestHit;
        m_rtShaderGroups[eClosestHit].anyHitShader = VK_SHADER_UNUSED_KHR;
        m_rtShaderGroups[eClosestHit].intersectionShader = VK_SHADER_UNUSED_KHR;

        // 3. Crear pipeline layout
        //Incluir aqui más sets si necesario
        std::vector<VkDescriptorSetLayout> rtDescSetLayouts = { m_rtDescSetLayout };

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

        printf("Creating rt pipeline layout\n");

        VkResult result = vkCreatePipelineLayout(*m_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ray tracing pipeline layout");
        }

        // 4. Crear el pipeline de ray tracing
        VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{};
        rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
        rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
        rayPipelineInfo.pStages = stages.data();
        rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
        rayPipelineInfo.pGroups = m_rtShaderGroups.data();
        rayPipelineInfo.maxPipelineRayRecursionDepth = 2; // Ajustar según necesidades
        rayPipelineInfo.layout = m_rtPipelineLayout;

        printf("Preparing to create RT pipeline\n");

        result = vkCreateRayTracingPipelinesKHR(*m_device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &m_rtPipeline);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ray tracing pipeline");
        }

        // 5. Limpiar módulos de shader
        //vkDestroyShaderModule(*m_device, raygenModule, nullptr);
        //vkDestroyShaderModule(*m_device, missModule, nullptr);
        //vkDestroyShaderModule(*m_device, chitModule, nullptr);

        printf("Ray tracing pipeline created successfully\n");
    }


    void Raytracer::createRtShaderBindingTable() {
        // 1. Obtener el tamaño de handle de shader group
        uint32_t groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
        uint32_t groupHandleSize = m_rtProperties.shaderGroupHandleSize;
        uint32_t groupSizeAligned = (groupHandleSize + m_rtProperties.shaderGroupBaseAlignment - 1) &
            ~(m_rtProperties.shaderGroupBaseAlignment - 1);

        // 2. Obtener handles de shader groups
        uint32_t sbtDataSize = groupCount * groupHandleSize;
        std::vector<uint8_t> shaderHandleStorage(sbtDataSize);

        VkResult result = vkGetRayTracingShaderGroupHandlesKHR(*m_device, m_rtPipeline, 0, groupCount,
            sbtDataSize, shaderHandleStorage.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to get ray tracing shader group handles");
        }

        // 3. Calcular tamaños de regiones
        VkDeviceSize sbtSize = groupCount * groupSizeAligned;

        // 4. Crear buffer SBT
        m_rtSBTBuffer = m_vkcore[0].CreateBufferBlas(
            sbtSize,
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
        );

        // 5. Mapear y copiar datos al buffer
        void* data;
        vkMapMemory(*m_device, m_rtSBTBuffer.m_mem, 0, sbtSize, 0, &data);

        auto* pSBTBuffer = reinterpret_cast<uint8_t*>(data);
        for (uint32_t g = 0; g < groupCount; g++) {
            memcpy(pSBTBuffer, shaderHandleStorage.data() + g * groupHandleSize, groupHandleSize);
            pSBTBuffer += groupSizeAligned;
        }

        vkUnmapMemory(*m_device, m_rtSBTBuffer.m_mem);

        // 6. Configurar regiones de SBT
        VkDeviceAddress sbtAddress = GetBufferDeviceAddress(*m_device, m_rtSBTBuffer.m_buffer);

        m_rgenRegion.deviceAddress = sbtAddress;
        m_rgenRegion.stride = groupSizeAligned;
        m_rgenRegion.size = groupSizeAligned;

        m_missRegion.deviceAddress = sbtAddress + groupSizeAligned;
        m_missRegion.stride = groupSizeAligned;
        m_missRegion.size = groupSizeAligned;

        m_hitRegion.deviceAddress = sbtAddress + 2 * groupSizeAligned;
        m_hitRegion.stride = groupSizeAligned;
        m_hitRegion.size = groupSizeAligned;

        m_callRegion = {}; // No se usa en este ejemplo

        printf("Shader binding table created successfully\n");
    }

    void Raytracer::raytrace(VkCommandBuffer cmdBuf, int width, int height) {
        // 1. Transición de imagen a layout correcto
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        imageMemoryBarrier.image = m_outTexture->m_image;
        imageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        // 2. Bind pipeline y descriptor sets
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout,
            0, 1, &m_rtDescSet, 0, nullptr);

        // 3. Ejecutar ray tracing
        vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, width, height, 1);

        // 4. Barrier para asegurar que el ray tracing termine
        VkMemoryBarrier memoryBarrier{};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
    }

    void Raytracer::render(int width, int height) {
        VkCommandBuffer cmdBuf;
        m_vkcore->CreateCommandBuffer(1, &cmdBuf);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmdBuf, &beginInfo);

        // Ejecutar ray tracing
        raytrace(cmdBuf, width, height);

        vkEndCommandBuffer(cmdBuf);

        // Submit y esperar
        core::VulkanQueue* pQueue = m_vkcore->GetQueue();
        uint32_t ImageIndex = pQueue->AcquireNextImage();

        pQueue->SubmitSync(cmdBuf);
        pQueue->Present(ImageIndex);
        pQueue->WaitIdle();

        // Limpiar command buffer
        vkFreeCommandBuffers(m_vkcore->GetDevice(), m_cmdBufPool, 1, &cmdBuf);
    }
}