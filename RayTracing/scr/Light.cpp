#include "Light.h"

Light::Light(glm::vec3 pos):Pos(pos)
{

}

std::vector<glm::mat4> Light::GetShadowTransformsPoint(glm::mat4 shadowProj)
{
	std::vector<glm::mat4> shadowTransforms;//视角变换要有6个对应方向的变换矩阵
	shadowTransforms.push_back(shadowProj * glm::lookAt(Pos, Pos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(Pos, Pos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(Pos, Pos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(Pos, Pos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(Pos, Pos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
	shadowTransforms.push_back(shadowProj * glm::lookAt(Pos, Pos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));
	return shadowTransforms;
}
