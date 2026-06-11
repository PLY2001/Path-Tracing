#pragma once
#include <vector>
#include <string>
#include "GL/glew.h"
#include "stb_image/stb_image.h"
#include <iostream>
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "Shader.h"
class Skybox
{
private:
	unsigned int loadCubemap(std::vector<std::string> faces);
public:
	Skybox(std::string path);
	unsigned int GenBox();
	unsigned int cubemapTexture;
	void Draw(Shader& SkyShader,unsigned int SkyboxID);
};