#pragma once
#include "GL/glew.h"
#include <iostream>
static void GLClearError()
{
	while (glGetError() != GL_NO_ERROR)
	{

	}
}

static void GLCheckError()
{
	while (GLenum error = glGetError())
	{
		std::cout << "[OpenGL Error](" << error << ")" << std::endl;
	}
}
