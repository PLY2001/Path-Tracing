#pragma once
#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>

class Light
{
private:
	
public:
	glm::vec3 Pos;
	Light(glm::vec3 pos);
	std::vector<glm::mat4> GetShadowTransformsPoint(glm::mat4 shadowProj);
};


