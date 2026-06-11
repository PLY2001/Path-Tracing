#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLFW/glfw3.h"

class Camera
{
private:
	glm::vec3 cameraPos;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;
	float cameraFov;


	float lastX;
	float lastY;
	float yaw;
	float pitch;

public:
	bool firstMouse;
	Camera();
	bool MouseControl(GLFWwindow* window);
	void ScrollControl(double xoffset, double yoffset);
	bool KeyControl(GLFWwindow* window,const float deltaTime);
	glm::mat4 SetView();
	glm::mat4 SetProjection(float aspect);
	inline glm::vec3 GetPosition() const { return cameraPos; }
};
