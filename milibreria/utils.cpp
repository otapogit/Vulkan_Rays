#pragma once
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan_core.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <string>

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
}