#pragma once

#include "core.h"


namespace core{
	struct SimpleMesh {
		BufferMemory m_vb;
		size_t m_vertexBufferSize = 0;
		int vertexcount = 0;
		VulkanTexture* m_pTex = NULL;

		void Destroy(VkDevice device) {
			m_vb.Destroy(device);
			if (m_pTex) {
				m_pTex->Destroy(device);
				delete m_pTex;
			}
		}
	};
}