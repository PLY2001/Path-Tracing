#include "Texturebuffer.h"
#include <GL/glew.h>


TextureBuffer::TextureBuffer(Triangle_encoded* data, unsigned int size)
{
	glGenBuffers(1, &RendererID);
	glBindBuffer(GL_TEXTURE_BUFFER, RendererID);
	glBufferData(GL_TEXTURE_BUFFER, size, data, GL_STATIC_DRAW);

	glGenTextures(1, &TexID);
	glBindTexture(GL_TEXTURE_BUFFER, TexID);

	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, RendererID);
}

TextureBuffer::TextureBuffer(BVHArrayNode_encoded* data, unsigned int size)
{
	glGenBuffers(1, &RendererID);
	glBindBuffer(GL_TEXTURE_BUFFER, RendererID);
	glBufferData(GL_TEXTURE_BUFFER, size, data, GL_STATIC_DRAW);

	glGenTextures(1, &TexID);
	glBindTexture(GL_TEXTURE_BUFFER, TexID);

	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, RendererID);
}

TextureBuffer::~TextureBuffer()
{
	glDeleteBuffers(1, &RendererID);
	glDeleteTextures(1, &TexID);
}

void TextureBuffer::Bind(unsigned int slot) const
{
	glActiveTexture(GL_TEXTURE0 + slot);//绑定纹理前，要先激活对应的纹理单元，索引为“0+slot”（索引为0的单元OpenGL是会自动激活的）
	glBindTexture(GL_TEXTURE_BUFFER, TexID);
}

void TextureBuffer::Unbind() const
{
	glBindTexture(GL_TEXTURE_BUFFER, 0);

}

