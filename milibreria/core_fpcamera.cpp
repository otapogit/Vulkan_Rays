#include "core_fpcamera.h"
#include <GLFW/glfw3.h>

bool CameraFirstPerson::GLFWCameraHandler(CameraMovement& movement, int Key, int Action, int Mods) {

	bool Press = Action != GLFW_RELEASE;

	bool Handled = true;

	switch (Key) {
	case GLFW_KEY_W:
		movement.Forward = Press;
		break;
	case GLFW_KEY_S:
		movement.Backward = Press;
		break;
	case GLFW_KEY_A:
		movement.StrafeLeft = Press;
		break;
	case GLFW_KEY_D:
		movement.StrafeRight = Press;
		break;
	case GLFW_KEY_O:
		movement.Up = Press;
		break;
	case GLFW_KEY_L:
		movement.Down = Press;
		break;
	case GLFW_KEY_P:
		movement.FastSpeed = Press;
		break;
	}

	return Handled;
}


CameraFirstPerson::CameraFirstPerson(const glm::vec3& Pos, const glm::vec3& Target,
	const glm::vec3& Up, float FOV, float width, float height, float znear, float zfar) {

	m_cameraPos = Pos;
	m_up = Up;

	float ar = width / height;

	m_cameraOrientation = glm::lookAt(Pos, Target, Up);
	m_projection = glm::perspective(FOV, ar, znear, zfar);

}

glm::mat4 CameraFirstPerson::GetViewMatrix() const {
	glm::mat4 t = glm::translate(glm::mat4(1.0), -m_cameraPos);

	glm::mat4 r = glm::mat4_cast(m_cameraOrientation);

	glm::mat4 res = r * t;
	return res;
}

glm::mat4 CameraFirstPerson::GetVPMatrix() const {
	glm::mat4 View = GetViewMatrix();
	return m_projection * View;
}

void CameraFirstPerson::Update(float dt) {

	if (m_mouseState.m_buttonPressed && (m_mouseState.m_pos != m_oldMousePos)) {
		CalcCameraOrientation();
	}
	m_oldMousePos = m_mouseState.m_pos;


	CalcVelocity(dt);
	m_cameraPos += m_velocity * dt;
}

void CameraFirstPerson::CalcVelocity(float dt) {
	glm::vec3 Acceleration = CalcAcceleration();

	if (Acceleration == glm::vec3(0.0f)) {
		m_velocity -= m_velocity * std::min(dt * m_damping, 1.0f);
	} else {
		m_velocity += Acceleration * m_acceleration * dt;
		float MaxSpeed = m_movement.FastSpeed ? m_maxSpeed * m_fastCoef : m_maxSpeed;

		if (glm::length(m_velocity) > m_maxSpeed) {
			m_velocity = glm::normalize(m_velocity) * m_maxSpeed;
		}
	}
}

glm::vec3 CameraFirstPerson::CalcAcceleration() {

	glm::mat4 v = glm::mat4_cast(m_cameraOrientation);
	glm::vec3 Forward, Up;

	glm::vec3 right = glm::vec3(v[0][0], v[1][0], v[2][0]);
	Forward = glm::vec3(v[0][2], v[1][2], v[2][2]);
	Up = glm::cross(Forward, right);

	glm::vec3 Acceleration = glm::vec3(0.0f);

	if (m_movement.Forward) {
		Acceleration -= Forward;
	}
	if (m_movement.Backward) {
		Acceleration += Forward;
	}
	if (m_movement.StrafeLeft) {
		Acceleration -= right;
	}
	if (m_movement.StrafeRight) {
		Acceleration += right;
	}
	if (m_movement.Up) {
		Acceleration -= Up;
	}
	if (m_movement.Down) {
		Acceleration += Up;
	}
	if (m_movement.FastSpeed) {
		Acceleration *= m_fastCoef;
	}
	return Acceleration;

}

void CameraFirstPerson::CalcCameraOrientation() {
	glm::vec2 DeltaMouse = m_mouseState.m_pos - m_oldMousePos;
	glm::quat DeltaQuat = glm::quat(glm::vec3(m_mouseSpeed * DeltaMouse.y, m_mouseSpeed * DeltaMouse.x, 0.0f));

	m_cameraOrientation = glm::normalize(DeltaQuat * m_cameraOrientation);

	SetUpVector();
}

void CameraFirstPerson::SetUpVector() {

	glm::mat4 View = GetViewMatrix();

	glm::vec3 Forward = glm::vec3(View[0][2], View[1][2], View[2][2]);
	glm::vec3 right = glm::vec3(View[0][0], View[1][0], View[2][0]);
	glm::vec3 Up = glm::cross(Forward, right);
	m_cameraOrientation = glm::lookAt(m_cameraPos, m_cameraPos + Forward, Up);

}


