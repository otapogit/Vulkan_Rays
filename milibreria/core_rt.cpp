#include "core_rt.h"

namespace core {
	//--------------------------------------------------------------------------------------------------
	// Initialize Vulkan ray tracing
	// #VKRay

	Raytracer::Raytracer() {
	}
	Raytracer::~Raytracer(){}

	void Raytracer::initRayTracing(core::PhysicalDevice physdev)
	{
		// Requesting ray tracing properties
		VkPhysicalDeviceProperties2 prop2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		prop2.pNext = &m_rtProperties;
		vkGetPhysicalDeviceProperties2(physdev.m_physDevice, &prop2);
	}
}