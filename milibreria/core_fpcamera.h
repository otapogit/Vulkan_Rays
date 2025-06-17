#pragma once
#include <iostream>
#include <algorithm>
#include <glm/ext.hpp>
#include <glm/glm.hpp>


struct CameraMovement {
	bool Forward = false;
	bool Backward = false;
	bool StrafeLeft = false;
	bool StrafeRight = false;
	bool Up = false;
	bool Down = false;
	bool FastSpeed = false;
};


struct MouseState {
	glm::vec2 m_pos = glm::vec2(0.0f);
	bool m_buttonPressed = false;
};

class CameraFirstPerson {
public:
	CameraFirstPerson(const glm::vec3& Pos, const glm::vec3& Target,
		const glm::vec3& Up, float FOV, float width, float height, float znear, float zfar);

	const glm::mat4& GetProjMatrix() const { return m_projection;}
	glm::vec3 GetPosition() const { return m_cameraPos; }
	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetVPMatrix() const;

	CameraMovement m_movement;
	float m_acceleration = 15.0f;
	float m_damping = 1.0f;
	float m_maxSpeed = 10.0f;
	float m_fastCoef = 5.0f;

	MouseState m_mouseState;
	float m_mouseSpeed = 1.0f;

	void Update(float dt);

	bool GLFWCameraHandler(CameraMovement& movement, int Key, int Action, int Mods);

private:

	glm::vec3 CalcAcceleration();
	void CalcVelocity(float dt);
	void CalcCameraOrientation();
	void SetUpVector();

	glm::mat4 m_projection = glm::mat4(0.0);
	glm::vec3 m_cameraPos = glm::vec3(0.0);
	glm::quat m_cameraOrientation = glm::quat(glm::vec3(0.0));
	glm::vec3 m_velocity = glm::vec3(0.0f);
	glm::vec2 m_oldMousePos = glm::vec2(0.0f);
	glm::vec3 m_up = glm::vec3(0.0f);

	float m_yaw;
	float m_pitch;
};
