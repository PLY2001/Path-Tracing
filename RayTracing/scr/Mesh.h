#pragma once
#include <string>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <vector>
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Shader.h"

#include "ErrorCheck.h"

#ifndef VERTEX
struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	//glm::vec3 Tangent;
	glm::vec2 Texcoord;
};
#endif

struct myTexture
{
	unsigned int slot;
	std::string type;
	aiString path;
};

//BVH新增

//三角形结构体
typedef struct myTriangle {
	glm::vec3 p1, p2, p3;   // 三点
	glm::vec3 center;       // 中心
	myTriangle() {};
	myTriangle(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
		p1 = a, p2 = b, p3 = c;
		center = (p1 + p2 + p3) / glm::vec3(3, 3, 3);
	}
} myTriangle;

// BVH 树节点
typedef struct BVHNode {
	BVHNode* left = NULL;       // 左右子树索引
	BVHNode* right = NULL;
	int n, index;               // 节点存储信息               
	glm::vec3 AABBMinPos, AABBMaxPos;                // 碰撞盒
}BVHNode,*BVHTree;

//

class Mesh
{
public:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<myTexture> textures;

	//BVH新增
	std::vector<myTriangle> triangles;
	BVHTree bvhtree;
	//
	Mesh(){}
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<myTexture> textures);
	void Draw(Shader& shader);
	void DrawInstanced(Shader& shader, int amount);
	unsigned int vaID, vbID, ibID;

	//BVH新增
	void ProcessTriangle();
	BVHTree buildBVH(std::vector<myTriangle>& triangles, int l, int r, int n);//构建BVH树
	BVHTree buildBVHwithSAH(std::vector<myTriangle>& triangles, int l, int r, int n);
	//
private:
	void SetMesh();

	//BVH新增

	

	// 按照三角形中心排序 -- 比较函数
	static bool cmpx(const myTriangle& t1, const myTriangle& t2) {
		return t1.center.x < t2.center.x;
	}
	static bool cmpy(const myTriangle& t1, const myTriangle& t2) {
		return t1.center.y < t2.center.y;
	}
	static bool cmpz(const myTriangle& t1, const myTriangle& t2) {
		return t1.center.z < t2.center.z;
	}

	
	//
	
	
};