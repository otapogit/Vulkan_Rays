#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "physical_device.h"
#include "core_utils.h"
#include "core.h"
#include <cassert>
#include "utils.h"
#include <string>

namespace core {

	VulkanPhysicalDevices::VulkanPhysicalDevices() {

	}

	VulkanPhysicalDevices::~VulkanPhysicalDevices() {

	}

	static void PrintImageUsageFlags(const VkImageUsageFlags& flags) {

		if (flags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
			printf("Image usage transfer src is supported\n");
		}
		if (flags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
			printf("Image usage transfer dst is supported\n");
		}
		if (flags & VK_IMAGE_USAGE_SAMPLED_BIT) {
			printf("Image usage sampled is supported\n");
		}
		if (flags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
			printf("Image usage color attachment is supported\n");
		}
		if (flags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			printf("Image usage depth stencil attachment is supported\n");
		}
		if (flags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
			printf("Image usage transient attachment is supported\n");
		}
		if (flags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
			printf("Image usage input attachment is supported\n");
		}
	}

	static VkFormat FindDepthFormat(VkPhysicalDevice Device) {

		std::vector<VkFormat> Candidates = { VK_FORMAT_D32_SFLOAT,
											VK_FORMAT_D32_SFLOAT_S8_UINT,
											VK_FORMAT_D24_UNORM_S8_UINT };

		VkFormat DepthFormat = FindSupportedFormat(Device, Candidates,
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

		return DepthFormat;
	}

	void VulkanPhysicalDevices::Init(const VkInstance& Instance, const VkSurfaceKHR& Surface) {
		uint32_t NumDevices = 0;

		VkResult res = vkEnumeratePhysicalDevices(Instance, &NumDevices, NULL);
		CHECK_VK_RESULT(res, "vkEnumeratePhysicalDevices Error (1)\n");

		printf("Num phyisical devices %d\n\n", NumDevices);

		m_devices.resize(NumDevices);

		std::vector<VkPhysicalDevice> Devices;
		Devices.resize(NumDevices);

		res = vkEnumeratePhysicalDevices(Instance, &NumDevices, Devices.data());
		CHECK_VK_RESULT(res, "vkEnumeratePhysicalDevices Error (2)\n");

		for (uint32_t i = 0; i < NumDevices; i++) {
			VkPhysicalDevice PhysDev = Devices[i];
			m_devices[i].m_physDevice = PhysDev;

			vkGetPhysicalDeviceProperties(PhysDev, &m_devices[i].m_devProps);
			
			printf("Device name: %s\n", m_devices[i].m_devProps.deviceName);
			uint32_t apiVer = m_devices[i].m_devProps.apiVersion;
			printf("	API version: %d.%d.%d.%d\n",
				VK_API_VERSION_VARIANT(apiVer),
				VK_API_VERSION_MAJOR(apiVer),
				VK_API_VERSION_MINOR(apiVer),
				VK_API_VERSION_PATCH(apiVer)
				);

			uint32_t NumQFamilies = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &NumQFamilies, NULL);
			printf("	Num of family queues: %d\n", NumQFamilies);

			m_devices[i].m_qFamilyProps.resize(NumQFamilies);
			m_devices[i].m_qSupportsPresent.resize(NumQFamilies);

			vkGetPhysicalDeviceQueueFamilyProperties(PhysDev, &NumQFamilies, m_devices[i].m_qFamilyProps.data());

			for (uint32_t q = 0; q < NumQFamilies; q++) {
				VkQueueFamilyProperties& QFamilyProp = m_devices[i].m_qFamilyProps[q];
				VkQueueFlags flags = QFamilyProp.queueFlags;
				printf("	GFX %s, Compute %s, Transfer %s, Sparse binding %s\n",
					(flags & VK_QUEUE_GRAPHICS_BIT) ? "Yes" : "No",
					(flags & VK_QUEUE_COMPUTE_BIT) ? "Yes" : "No",
					(flags & VK_QUEUE_TRANSFER_BIT) ? "Yes" : "No",
					(flags & VK_QUEUE_SPARSE_BINDING_BIT) ? "Yes" : "No"
				);
				uint32_t qcount = QFamilyProp.queueCount;
				printf("		Queue Count: ");
				printf(std::to_string(qcount).c_str());
				printf("\n");
				res = vkGetPhysicalDeviceSurfaceSupportKHR(PhysDev, q, Surface, &(m_devices[i].m_qSupportsPresent[q]));
				CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceSupportKHR error\n");
			}

			uint32_t NumFormats = 0;
			res = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, Surface, &NumFormats, NULL);
			CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceFormatsKHR error (1)\n");
			assert(NumFormats > 0);
			
			m_devices[i].m_surfaceFormats.resize(NumFormats);

			res = vkGetPhysicalDeviceSurfaceFormatsKHR(PhysDev, Surface, &NumFormats, m_devices[i].m_surfaceFormats.data());
			CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceFormatsKHR error (2)\n");

			for (uint32_t j = 0; j < NumFormats; j++) {
				const VkSurfaceFormatKHR& SurfaceFormat = m_devices[i].m_surfaceFormats[j];
				printf("	Format %x color space %x\n", SurfaceFormat.format, SurfaceFormat.colorSpace);
			}

			res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(PhysDev, Surface, &(m_devices[i].m_surfaceCaps));
			CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR error\n");

			PrintImageUsageFlags(m_devices[i].m_surfaceCaps.supportedUsageFlags);

			uint32_t NumPresentModes = 0;

			res = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, Surface, &NumPresentModes, NULL);
			CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR error (1)\n");
			
			assert(NumPresentModes != 0);
			m_devices[i].m_presentModes.resize(NumPresentModes);

			res = vkGetPhysicalDeviceSurfacePresentModesKHR(PhysDev, Surface, &NumPresentModes, m_devices[i].m_presentModes.data());
			CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR error (2)\n");

			printf("Number of presentation modes %d\n", NumPresentModes);
			vkGetPhysicalDeviceMemoryProperties(PhysDev, &(m_devices[i].m_memProps));

			printf("Num memory types %d\n", m_devices[i].m_memProps.memoryTypeCount);
			for (uint32_t j = 0; j < m_devices[i].m_memProps.memoryTypeCount;j++) {
				printf("%d: flags %x heap %d\t", j,
					m_devices[i].m_memProps.memoryTypes[j].propertyFlags,
					m_devices[i].m_memProps.memoryTypes[j].heapIndex);

			}
			//Falta aqui un metodo para mostrar los queeu types soportados
			printf("\n");
			printf("Num heap types %d\n", m_devices[i].m_memProps.memoryHeapCount);
			printf("\n");

			vkGetPhysicalDeviceFeatures(m_devices[i].m_physDevice, &m_devices[i].m_features);

			m_devices[i].m_depthFormat = FindDepthFormat(PhysDev);
		}
	}

	uint32_t VulkanPhysicalDevices::SelectDevice(VkQueueFlags RequiredQueueType, bool SupportsPresent) {
		for (uint32_t i = m_devices.size()-1; i >= 0 ; i--) {
			for (uint32_t j = 0; j < m_devices[i].m_qFamilyProps.size();j++) {
				const VkQueueFamilyProperties& QFamilyProps = m_devices[i].m_qFamilyProps[j];
				if ((QFamilyProps.queueFlags & RequiredQueueType) && ((bool)m_devices[i].m_qSupportsPresent[j] == SupportsPresent)) {
					m_devIndex = i;
					int QueueFamily = j;
					printf("Using GFX device %d and queue family %d\n", m_devIndex, QueueFamily);
					return QueueFamily;
				}
			}
		}

		fprintf(stderr, "Reqwuired queue type %x and supports present %d not found\n", RequiredQueueType, SupportsPresent);
		return 0;
	}

	const PhysicalDevice& VulkanPhysicalDevices::Selected() const {
		if (m_devIndex < 0){
			fprintf(stderr, "Physical device not selected \n");
		}
		return m_devices[m_devIndex];
	}


}