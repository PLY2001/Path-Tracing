#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>

#define T
struct Triangle_encoded {
	glm::vec3 p1, p2, p3;    // 顶点坐标
	glm::vec3 n1, n2, n3;    // 顶点法线
	glm::vec3 emissive;      // 自发光参数
	glm::vec3 baseColor;     // 颜色
	glm::vec3 param1;        // (subsurface, metallic, specular)
	glm::vec3 param2;        // (specularTint, roughness, anisotropic)
	glm::vec3 param3;        // (sheen, sheenTint, clearcoat)
	glm::vec3 param4;        // (clearcoatGloss, IOR, transmission)
};

#define BVH
struct BVHArrayNode_encoded {
	glm::vec3 childs;        // (left, right, 保留)
	glm::vec3 leafInfo;      // (n, index, 保留)
	glm::vec3 AA, BB;
};


class TextureBuffer
{
private:
	unsigned int RendererID;
	unsigned int TexID;
public:
	//template<typename F>
	TextureBuffer(Triangle_encoded* data, unsigned int size);
	TextureBuffer(BVHArrayNode_encoded* data, unsigned int size);
	~TextureBuffer();
	void Bind(unsigned int slot = 0) const;
	void Unbind() const;

};