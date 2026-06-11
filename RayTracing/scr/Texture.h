#pragma once
#include <string>
#include "GL/glew.h"
#include "stb_image/stb_image.h"
#include "hdrloader/hdrloader.h"
#include <vector>
#include <algorithm>
class Texture
{
private:
	unsigned int RendererID;
	std::string FilePath;
	unsigned char* LocalBuffer;
	int Width, Height, BBP;//bits per pixel

public:
	Texture(const std::string& path);
	Texture(const std::string& path, const std::string texturemode);
	float* calculateHdrCache(float* HDR, int width, int height);
	~Texture();

	void Bind(unsigned int slot = 0) const;//Ä¬ÈÏË÷ÒýÎª0£¨¼´TEXCOORD0£©
	void Unbind() const;

	inline int GetWidth() const { return Width; }
	inline int GetHeight() const { return Height; }
};