#pragma once
#include "utils.h"

namespace core {

	const char* Get_DebugSeverityString(VkDebugUtilsMessageTypeFlagBitsEXT severity) {

		switch (severity) {
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
				return "info";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
				return "warning";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
				return "error";
			case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
				return "verbose";
			default:
				exit(1);
		}

		return "No such severity";
	}

	const char* Get_DebugType(VkDebugUtilsMessageTypeFlagsEXT severity) {

		switch (severity) {
			case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
				return "General";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
				return "Validation";
			case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
				return "Performance";

			default:
				exit(1);
		}

		return "No such type";
	}

	int GetBytesPerTexFormat(VkFormat Format)
	{
		switch (Format)
		{
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8_UNORM:
			return 1;
		case VK_FORMAT_R16_SFLOAT:
			return 2;
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_B8G8R8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_R8G8B8A8_SRGB:
			return 4;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			return 4 * sizeof(uint16_t);
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			return 4 * sizeof(float);
		default:
			printf("Unknown format %d\n", Format);
			exit(1);
		}

		return 0;
	}

	bool HasStencilComponent(VkFormat Format)
	{
		return ((Format == VK_FORMAT_D32_SFLOAT_S8_UINT) ||
			(Format == VK_FORMAT_D24_UNORM_S8_UINT));
	}

	VkFormat FindSupportedFormat(VkPhysicalDevice Device, const std::vector<VkFormat>& Candidates,
		VkImageTiling Tiling, VkFormatFeatureFlags Features)
	{
		for (int i = 0; i < Candidates.size(); i++) {
			VkFormat Format = Candidates[i];
			VkFormatProperties Props;
			vkGetPhysicalDeviceFormatProperties(Device, Format, &Props);

			if ((Tiling == VK_IMAGE_TILING_LINEAR) &&
				(Props.linearTilingFeatures & Features) == Features) {
				return Format;
			}
			else if (Tiling == VK_IMAGE_TILING_OPTIMAL &&
				(Props.optimalTilingFeatures & Features) == Features) {
				return Format;
			}
		}

		printf("Failed to find supported format!\n");
		exit(1);
	}
}