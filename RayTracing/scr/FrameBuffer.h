#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GL/glew.h"
#include <iostream>
#include <vector>
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "Shader.h"

class FrameBuffer
{
private:
	unsigned int fbID;//FrameBuffer
	
	unsigned int rbID;//RenderBuffer
	unsigned int WinWidth;
	unsigned int WinHeight;
	unsigned int mSamples;

public:
	unsigned int tbID;//TextureBuffer
	FrameBuffer(unsigned int WinWidth,unsigned int WinHeight);
	void GenTexture2D();
	void GenTexture2DMultiSample(unsigned int samples);
	void GenTexture2DShadowMap();
	void GenTexture2DShadowCubeMap();
	~FrameBuffer();
	unsigned int GenQuad();
	void Bind() const;
	void Unbind() const;
	void Draw(Shader& ScreenShader,unsigned int vaID);
	void GetColorAfterMSAA(unsigned int framebufferMSAA);
	void GetDepthBuffer(unsigned int framebufferDepth);
	void ShowColorAfterMSAA(unsigned int framebufferMSAA);
	inline unsigned int GetID() { return fbID; }
	inline unsigned int GetTexID() { return tbID; }
	void ResetWindow(unsigned int Width, unsigned int Height);
	void ResetWindowMultiSample(unsigned int Width, unsigned int Height);
	void ResetWindowCameraMap(unsigned int Width, unsigned int Height);
};