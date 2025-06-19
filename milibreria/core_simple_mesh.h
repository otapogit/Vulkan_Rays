#pragma once

#include "core.h"


namespace core{
	struct SimpleMesh {
		BufferMemory m_vb;
		BufferMemory m_indexbuffer;
		size_t m_vertexBufferSize = 0;
		size_t m_indexBufferSize = 0;
		VkIndexType m_indexType = VK_INDEX_TYPE_UINT32;
		int vertexcount = 0;
		VulkanTexture* m_pTex = NULL;

		void Destroy(VkDevice device) {
			m_vb.Destroy(device);
			if (m_pTex) {
				m_pTex->Destroy(device);
				delete m_pTex;
			}
			if (m_indexbuffer.m_buffer)
				m_indexbuffer.Destroy(device);

		}
	};
}