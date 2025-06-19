#pragma once
#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace core {
	struct VertexObj {
		VertexObj(const glm::vec3& p, const glm::vec2& t) {
			Pos = p;
			Tex = t;
		}
		glm::vec3 Pos;
		glm::vec2 Tex;
	};
}