#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include <random>
#include <iomanip>

#include "ErrorCheck.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image/stb_image.h"
#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "Model.h"
#include "TextureBuffer.h"
#include <ctime>
#include "FrameBuffer.h"
#include "Camera.h"
#include "Texture.h"



#define PI 3.1415926
#define INF 1000000

enum class MouseMode { Disable = 0, Enable = 1 };

MouseMode mousemode = MouseMode::Disable;

//control
//void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouseClick_callback(GLFWwindow* window, int button, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

glm::mat4 LastViewMatrix;
glm::mat4 LastProjectionMatrix;

//window size
int WinWidth = 256;/////////////////////////窗口分辨率
int WinHeight = 256;

//camera
Camera camera;

//为了修改窗口尺寸
FrameBuffer* fblast;
FrameBuffer* fbnew;
FrameBuffer* fbDepth;
FrameBuffer* fbLastDepth;
FrameBuffer* fbNormal;

// 颜色
const glm::vec3 RED(1, 0.5, 0.5);
const glm::vec3 GREEN(0.5, 1, 0.5);
const glm::vec3 BLUE(0.5, 0.5, 1);
const glm::vec3 YELLOW(1.0, 1.0, 0.1);
const glm::vec3 CYAN(0.1, 1.0, 1.0);
const glm::vec3 MAGENTA(1.0, 0.1, 1.0);
const glm::vec3 GRAY(0.5, 0.5, 0.5);
const glm::vec3 WHITE(1, 1, 1);




float fps = 30;
unsigned int frameCounter = 0;
unsigned int lastFrameCounter = 0;
unsigned int realLastFrameCounter = 0;
unsigned int lastFrameWeight = 5;

float deltaTime = 0;//每次循环耗时
float lastTime = 0;

// 物体表面材质定义
struct Material {
	glm::vec3 emissive = glm::vec3(0, 0, 0);  // 作为光源时的发光颜色
	glm::vec3 baseColor = glm::vec3(1, 1, 1);
	float subsurface = 0.0;
	float metallic = 0.0;
	float specular = 0.0;
	float specularTint = 0.0;
	float roughness = 0.0;
	float anisotropic = 0.0;
	float sheen = 0.0;
	float sheenTint = 0.0;
	float clearcoat = 0.0;
	float clearcoatGloss = 0.0;
	float IOR = 1.0;
	float transmission = 0.0;
};

// 三角形定义
struct Triangle {
	glm::vec3 p1, p2, p3;    // 顶点坐标
	glm::vec3 n1, n2, n3;    // 顶点法线
	Material material;  // 材质
};

// BVH 树节点
struct BVHArrayNode {
	int left, right;    // 左右子树索引
	int n, index;       // 叶子节点信息               
	glm::vec3 AA, BB;        // 碰撞盒
};

#ifndef T
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
#endif

#ifndef BVH
struct BVHArrayNode_encoded {
	glm::vec3 childs;        // (left, right, 保留)
	glm::vec3 leafInfo;      // (n, index, 保留)
	glm::vec3 AA, BB;
};
#endif






// 按照三角形中心排序 -- 比较函数
bool cmpx(const Triangle& t1, const Triangle& t2) {
	glm::vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / glm::vec3(3, 3, 3);
	glm::vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / glm::vec3(3, 3, 3);
	return center1.x < center2.x;
}
bool cmpy(const Triangle& t1, const Triangle& t2) {
	glm::vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / glm::vec3(3, 3, 3);
	glm::vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / glm::vec3(3, 3, 3);
	return center1.y < center2.y;
}
bool cmpz(const Triangle& t1, const Triangle& t2) {
	glm::vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / glm::vec3(3, 3, 3);
	glm::vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / glm::vec3(3, 3, 3);
	return center1.z < center2.z;
}


// 构建 BVH
int buildBVH(std::vector<Triangle>& triangles, std::vector<BVHArrayNode>& nodes, int l, int r, int n) {
	if (l > r) return 0;

	// 注：
	// 此处不可通过指针，引用等方式操作，必须用 nodes[id] 来操作
	// 因为 std::vector<> 扩容时会拷贝到更大的内存，那么地址就改变了
	// 而指针，引用均指向原来的内存，所以会发生错误
	nodes.push_back(BVHArrayNode());
	int id = nodes.size() - 1;   // 注意： 先保存索引
	nodes[id].left = nodes[id].right = nodes[id].n = nodes[id].index = 0;
	nodes[id].AA = glm::vec3(1145141919, 1145141919, 1145141919);
	nodes[id].BB = glm::vec3(-1145141919, -1145141919, -1145141919);

	// 计算 AABB
	for (int i = l; i <= r; i++) {
		// 最小点 AA
		float minx = glm::min(triangles[i].p1.x, glm::min(triangles[i].p2.x, triangles[i].p3.x));
		float miny = glm::min(triangles[i].p1.y, glm::min(triangles[i].p2.y, triangles[i].p3.y));
		float minz = glm::min(triangles[i].p1.z, glm::min(triangles[i].p2.z, triangles[i].p3.z));
		nodes[id].AA.x = glm::min(nodes[id].AA.x, minx);
		nodes[id].AA.y = glm::min(nodes[id].AA.y, miny);
		nodes[id].AA.z = glm::min(nodes[id].AA.z, minz);
		// 最大点 BB
		float maxx = glm::max(triangles[i].p1.x, glm::max(triangles[i].p2.x, triangles[i].p3.x));
		float maxy = glm::max(triangles[i].p1.y, glm::max(triangles[i].p2.y, triangles[i].p3.y));
		float maxz = glm::max(triangles[i].p1.z, glm::max(triangles[i].p2.z, triangles[i].p3.z));
		nodes[id].BB.x = glm::max(nodes[id].BB.x, maxx);
		nodes[id].BB.y = glm::max(nodes[id].BB.y, maxy);
		nodes[id].BB.z = glm::max(nodes[id].BB.z, maxz);
	}

	// 不多于 n 个三角形 返回叶子节点
	if ((r - l + 1) <= n) {
		nodes[id].n = r - l + 1;
		nodes[id].index = l;
		return id;
	}

	// 否则递归建树
	float lenx = nodes[id].BB.x - nodes[id].AA.x;
	float leny = nodes[id].BB.y - nodes[id].AA.y;
	float lenz = nodes[id].BB.z - nodes[id].AA.z;
	// 按 x 划分
	if (lenx >= leny && lenx >= lenz)
		std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpx);
	// 按 y 划分
	if (leny >= lenx && leny >= lenz)
		std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpy);
	// 按 z 划分
	if (lenz >= lenx && lenz >= leny)
		std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpz);
	// 递归
	int mid = (l + r) / 2;
	int left = buildBVH(triangles, nodes, l, mid, n);
	int right = buildBVH(triangles, nodes, mid + 1, r, n);

	nodes[id].left = left;
	nodes[id].right = right;

	return id;
}

// SAH 优化构建 BVH
int buildBVHwithSAH(std::vector<Triangle>& triangles, std::vector<BVHArrayNode>& nodes, int l, int r, int n) {
	if (l > r) return 0;

	nodes.push_back(BVHArrayNode());
	int id = nodes.size() - 1;
	nodes[id].left = nodes[id].right = nodes[id].n = nodes[id].index = 0;
	nodes[id].AA = glm::vec3(1145141919, 1145141919, 1145141919);
	nodes[id].BB = glm::vec3(-1145141919, -1145141919, -1145141919);

	// 计算 AABB
	for (int i = l; i <= r; i++) {
		// 最小点 AA
		float minx = glm::min(triangles[i].p1.x, glm::min(triangles[i].p2.x, triangles[i].p3.x));
		float miny = glm::min(triangles[i].p1.y, glm::min(triangles[i].p2.y, triangles[i].p3.y));
		float minz = glm::min(triangles[i].p1.z, glm::min(triangles[i].p2.z, triangles[i].p3.z));
		nodes[id].AA.x = glm::min(nodes[id].AA.x, minx);
		nodes[id].AA.y = glm::min(nodes[id].AA.y, miny);
		nodes[id].AA.z = glm::min(nodes[id].AA.z, minz);
		// 最大点 BB
		float maxx = glm::max(triangles[i].p1.x, glm::max(triangles[i].p2.x, triangles[i].p3.x));
		float maxy = glm::max(triangles[i].p1.y, glm::max(triangles[i].p2.y, triangles[i].p3.y));
		float maxz = glm::max(triangles[i].p1.z, glm::max(triangles[i].p2.z, triangles[i].p3.z));
		nodes[id].BB.x = glm::max(nodes[id].BB.x, maxx);
		nodes[id].BB.y = glm::max(nodes[id].BB.y, maxy);
		nodes[id].BB.z = glm::max(nodes[id].BB.z, maxz);
	}

	// 不多于 n 个三角形 返回叶子节点
	if ((r - l + 1) <= n) {
		nodes[id].n = r - l + 1;
		nodes[id].index = l;
		return id;
	}

	// 否则递归建树
	float Cost = INF;
	int Axis = 0;
	int Split = (l + r) / 2;
	for (int axis = 0; axis < 3; axis++) {
		// 分别按 x，y，z 轴排序
		if (axis == 0) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpx);
		if (axis == 1) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpy);
		if (axis == 2) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpz);

		// leftMax[i]: [l, i] 中最大的 xyz 值
		// leftMin[i]: [l, i] 中最小的 xyz 值
		std::vector<glm::vec3> leftMax(r - l + 1, glm::vec3(-INF, -INF, -INF));
		std::vector<glm::vec3> leftMin(r - l + 1, glm::vec3(INF, INF, INF));
		// 计算前缀 注意 i-l 以对齐到下标 0
		for (int i = l; i <= r; i++) {
			Triangle& t = triangles[i];
			int bias = (i == l) ? 0 : 1;  // 第一个元素特殊处理

			leftMax[i - l].x = glm::max(leftMax[i - l - bias].x, glm::max(t.p1.x, glm::max(t.p2.x, t.p3.x)));
			leftMax[i - l].y = glm::max(leftMax[i - l - bias].y, glm::max(t.p1.y, glm::max(t.p2.y, t.p3.y)));
			leftMax[i - l].z = glm::max(leftMax[i - l - bias].z, glm::max(t.p1.z, glm::max(t.p2.z, t.p3.z)));
			
			leftMin[i - l].x = glm::min(leftMin[i - l - bias].x, glm::min(t.p1.x, glm::min(t.p2.x, t.p3.x)));
			leftMin[i - l].y = glm::min(leftMin[i - l - bias].y, glm::min(t.p1.y, glm::min(t.p2.y, t.p3.y)));
			leftMin[i - l].z = glm::min(leftMin[i - l - bias].z, glm::min(t.p1.z, glm::min(t.p2.z, t.p3.z)));
		}

		// rightMax[i]: [i, r] 中最大的 xyz 值
		// rightMin[i]: [i, r] 中最小的 xyz 值
		std::vector<glm::vec3> rightMax(r - l + 1, glm::vec3(-INF, -INF, -INF));
		std::vector<glm::vec3> rightMin(r - l + 1, glm::vec3(INF, INF, INF));
		// 计算后缀 注意 i-l 以对齐到下标 0
		for (int i = r; i >= l; i--) {
			Triangle& t = triangles[i];
			int bias = (i == r) ? 0 : 1;  // 第一个元素特殊处理

			rightMax[i - l].x = glm::max(rightMax[i - l + bias].x, glm::max(t.p1.x, glm::max(t.p2.x, t.p3.x)));
			rightMax[i - l].y = glm::max(rightMax[i - l + bias].y, glm::max(t.p1.y, glm::max(t.p2.y, t.p3.y)));
			rightMax[i - l].z = glm::max(rightMax[i - l + bias].z, glm::max(t.p1.z, glm::max(t.p2.z, t.p3.z)));
			
			rightMin[i - l].x = glm::min(rightMin[i - l + bias].x, glm::min(t.p1.x, glm::min(t.p2.x, t.p3.x)));
			rightMin[i - l].y = glm::min(rightMin[i - l + bias].y, glm::min(t.p1.y, glm::min(t.p2.y, t.p3.y)));
			rightMin[i - l].z = glm::min(rightMin[i - l + bias].z, glm::min(t.p1.z, glm::min(t.p2.z, t.p3.z)));
		}

		// 遍历寻找分割
		float cost = INF;
		int split = l;
		for (int i = l; i <= r - 1; i++) {
			float lenx, leny, lenz;
			// 左侧 [l, i]
			glm::vec3 leftAA = leftMin[i - l];
			glm::vec3 leftBB = leftMax[i - l];
			lenx = leftBB.x - leftAA.x;
			leny = leftBB.y - leftAA.y;
			lenz = leftBB.z - leftAA.z;
			float leftS = 2.0 * ((lenx * leny) + (lenx * lenz) + (leny * lenz));
			float leftCost = leftS * (i - l + 1);

			// 右侧 [i+1, r]
			glm::vec3 rightAA = rightMin[i + 1 - l];
			glm::vec3 rightBB = rightMax[i + 1 - l];
			lenx = rightBB.x - rightAA.x;
			leny = rightBB.y - rightAA.y;
			lenz = rightBB.z - rightAA.z;
			float rightS = 2.0 * ((lenx * leny) + (lenx * lenz) + (leny * lenz));
			float rightCost = rightS * (r - i);

			// 记录每个分割的最小答案
			float totalCost = leftCost + rightCost;
			if (totalCost < cost) {
				cost = totalCost;
				split = i;
			}
		}
		// 记录每个轴的最佳答案
		if (cost < Cost) {
			Cost = cost;
			Axis = axis;
			Split = split;
		}
	}

	// 按最佳轴分割
	if (Axis == 0) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpx);
	if (Axis == 1) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpy);
	if (Axis == 2) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpz);

	// 递归
	int left = buildBVHwithSAH(triangles, nodes, l, Split, n);
	int right = buildBVHwithSAH(triangles, nodes, Split + 1, r, n);

	nodes[id].left = left;
	nodes[id].right = right;

	return id;
}



unsigned int GenQuad()
{
	std::vector<SimpleVertex> quadVertices = {
		// positions // texCoords
		{glm::vec3(-1.0f,  1.0f,  0.0f),glm::vec2(0.0f, 1.0f)},
		{glm::vec3(-1.0f, -1.0f,  0.0f),glm::vec2(0.0f, 0.0f)},
		{glm::vec3(1.0f, -1.0f,  0.0f),glm::vec2(1.0f, 0.0f)},

		{glm::vec3(-1.0f,  1.0f,  0.0f),glm::vec2(0.0f, 1.0f)},
		{glm::vec3(1.0f, -1.0f,  0.0f),glm::vec2(1.0f, 0.0f)},
		{glm::vec3(1.0f,  1.0f,  0.0f),glm::vec2(1.0f, 1.0f)}
	};
	unsigned int vaID;//VertexArray
	unsigned int vbID;

	VertexArray* va = new VertexArray(vaID);
	VertexBuffer* vb = new VertexBuffer(vbID, quadVertices.data(), (unsigned int)quadVertices.size() * sizeof(SimpleVertex));

	VertexAttribLayout layout;//创建顶点属性布局实例
	layout.Push<GL_FLOAT>(3);//填入第一个属性布局，类型为float，每个点为3维向量
	layout.Push<GL_FLOAT>(2);//填入第三个属性布局，类型为float，每个点为2维向量

	va->AddBuffer(vbID, layout);//将所有属性布局应用于顶点缓冲区vb，并绑定在顶点数组对象va上

	va->Unbind();
	vb->Unbind();
	return vaID;
}







int main(void)
{

	



	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
	{
		std::cout << "glfwInit FAIL!" << std::endl;
		return -1;
	}



	









	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WinWidth, WinHeight, "PathTracing GPU", NULL, NULL);////////////////创建窗口，名字为Heart
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//control
	//glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetMouseButtonCallback(window, mouseClick_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);//每当检测到窗口尺寸改变就回调framebuffer_size_callback函数

	glfwSwapInterval(0);//1为开启垂直同步

	if (glewInit())
	{
		std::cout << "glewInit FAIL!" << std::endl;
	}









	/*兔子*/
	Model bunny("res/models/bunnyOnPlane.obj", glm::vec3(0.0f, 0.0f, 0.0f));//读取模型，目录从当前项目根目录开始，或者生成的exe根目录。需将noise.jpg复制到每一个模型旁边。
	
	/*房间*/
	Model room("res/models/plane.obj", glm::vec3(0.0f, 0.0f, 0.0f));//读取模型，目录从当前项目根目录开始，或者生成的exe根目录。需将noise.jpg复制到每一个模型旁边。
	// 读取三角形
	std::vector<Triangle> triangles;


	//glm::vec3 robotColor[7] = { glm::vec3(229.0f,127.0f,88.0f) / 255.0f,glm::vec3(229.0f,127.0f,88.0f) / 255.0f,glm::vec3(229.0f,127.0f,88.0f) / 255.0f,glm::vec3(229.0f,127.0f,88.0f) / 255.0f,glm::vec3(229.0f,127.0f,88.0f) / 255.0f,glm::vec3(229.0f,127.0f,88.0f) / 255.0f,glm::vec3(65.0f,65.0f,65.0f) / 255.0f };
	//int j = 0;
	for (Mesh& thismesh : bunny.meshes)
	{
		for (int i = 0; i < thismesh.indices.size(); i = i + 3)
		{
			Triangle triangle;
			triangle.p1 = thismesh.vertices[thismesh.indices[i]].Position;
			triangle.p2 = thismesh.vertices[thismesh.indices[i + 1]].Position;
			triangle.p3 = thismesh.vertices[thismesh.indices[i + 2]].Position;

			triangle.n1 = thismesh.vertices[thismesh.indices[i]].Normal;
			triangle.n2 = thismesh.vertices[thismesh.indices[i + 1]].Normal;
			triangle.n3 = thismesh.vertices[thismesh.indices[i + 2]].Normal;

			Material material;
			//material.baseColor = robotColor[j];
			material.baseColor = RED;
			material.specular = 1.0;
			material.specularTint = 1.0;
			material.roughness = 0.2;
			material.subsurface = 1.0;
			material.metallic = 0.0;
			material.clearcoat = 1.0;
			material.clearcoatGloss = 0.0;

			//glm::vec3 emissive = glm::vec3(0, 0, 0);  // 作为光源时的发光颜色
			//glm::vec3 baseColor = glm::vec3(1, 1, 1);
			//float subsurface = 0.0;
			//float metallic = 0.0;
			//float specular = 0.0;
			//float specularTint = 0.0;
			//float roughness = 0.0;
			//float anisotropic = 0.0;
			//float sheen = 0.0;
			//float sheenTint = 0.0;
			//float clearcoat = 0.0;
			//float clearcoatGloss = 0.0;
			//float IOR = 1.0;
			//float transmission = 0.0;

			triangle.material = material;

			triangles.push_back(triangle);
		}
		//j++;
	}
	
	
	for(Mesh &thismesh : room.meshes )
	{
		for (int i = 0; i < thismesh.indices.size(); i = i + 3)
		{
			Triangle triangle;
			triangle.p1 = thismesh.vertices[thismesh.indices[i]].Position;
			triangle.p2 = thismesh.vertices[thismesh.indices[i + 1]].Position;
			triangle.p3 = thismesh.vertices[thismesh.indices[i + 2]].Position;

			triangle.n1 = thismesh.vertices[thismesh.indices[i]].Normal;
			triangle.n2 = thismesh.vertices[thismesh.indices[i + 1]].Normal;
			triangle.n3 = thismesh.vertices[thismesh.indices[i + 2]].Normal;

			Material material;
			material.baseColor = WHITE;
			material.specular = 1.0;
			material.roughness = 0.01;
			material.subsurface = 0.0;
			material.metallic = 0.0;
			material.clearcoat = 1.0;
			material.clearcoatGloss = 0.0;
			triangle.material = material;

			triangles.push_back(triangle);
		}
	}
	

	
	int nTriangles = triangles.size();
	std::cout << "模型读取完成: 共 " << nTriangles << " 个三角形" << std::endl;

	


	BVHArrayNode FirstUselessNode;
	FirstUselessNode.left = 255;
	FirstUselessNode.right = 128;
	FirstUselessNode.n = 30;
	FirstUselessNode.AA = glm::vec3(1, 1, 0);
	FirstUselessNode.BB = glm::vec3(0, 1, 0);

	std::vector<BVHArrayNode> nodes{ FirstUselessNode };

	buildBVHwithSAH(triangles, nodes, 0, triangles.size() - 1, 8);
	int nNodes = nodes.size();
	std::cout << "BVH 建立完成: 共 " << nNodes << " 个节点" << std::endl;


	// 编码 三角形, 材质
	std::vector<Triangle_encoded> triangles_encoded(nTriangles);
	for (int i = 0; i < nTriangles; ++i)
	{
		Triangle& t = triangles[i];
		Material& m = t.material;
		// 顶点位置
		triangles_encoded[i].p1 = t.p1;
		triangles_encoded[i].p2 = t.p2;
		triangles_encoded[i].p3 = t.p3;
		// 顶点法线
		triangles_encoded[i].n1 = t.n1;
		triangles_encoded[i].n2 = t.n2;
		triangles_encoded[i].n3 = t.n3;
		// 材质
		triangles_encoded[i].emissive = m.emissive;
		triangles_encoded[i].baseColor = m.baseColor;
		triangles_encoded[i].param1 = glm::vec3(m.subsurface, m.metallic, m.specular);
		triangles_encoded[i].param2 = glm::vec3(m.specularTint, m.roughness, m.anisotropic);
		triangles_encoded[i].param3 = glm::vec3(m.sheen, m.sheenTint, m.clearcoat);
		triangles_encoded[i].param4 = glm::vec3(m.clearcoatGloss, m.IOR, m.transmission);
	}

	TextureBuffer tboTriangles(&triangles_encoded[0], triangles_encoded.size() * sizeof(Triangle_encoded));


	// 编码 BVHNode, aabb
	std::vector<BVHArrayNode_encoded> nodes_encoded(nNodes);
	for (int i = 0; i < nNodes; i++) {
		nodes_encoded[i].childs = glm::vec3(nodes[i].left, nodes[i].right, 0);
		nodes_encoded[i].leafInfo = glm::vec3(nodes[i].n, nodes[i].index, 0);
		nodes_encoded[i].AA = nodes[i].AA;
		nodes_encoded[i].BB = nodes[i].BB;
	}


	TextureBuffer tboBVH(&nodes_encoded[0], nodes_encoded.size() * sizeof(BVHArrayNode_encoded));






	FrameBuffer newframebuffer(WinWidth,WinHeight);
	fbnew = &newframebuffer;
	newframebuffer.GenTexture2D();


	FrameBuffer lastframebuffer(WinWidth, WinHeight);
	fblast = &lastframebuffer;
	lastframebuffer.GenTexture2D();
	unsigned int FboQuadID = lastframebuffer.GenQuad();//用于绘制贴图的四边形

	FrameBuffer framebufferDepth(WinWidth, WinWidth);
	fbDepth = &framebufferDepth;
	framebufferDepth.GenTexture2DShadowMap();//深度越远是1，越近是0

	FrameBuffer framebufferLastDepth(WinWidth, WinWidth);
	fbLastDepth = &framebufferLastDepth;
	framebufferLastDepth.GenTexture2DShadowMap();//深度越远是1，越近是0

	FrameBuffer framebufferNormal(WinWidth, WinHeight);
	fbNormal = &framebufferNormal;
	framebufferNormal.GenTexture2D();



// 	RenderPass pass1;
// 	RenderPass pass2;



	unsigned int QuadID = GenQuad();

	Shader PathTracingShader("res/Shaders/PathTracing.shader");
	PathTracingShader.Bind();
	PathTracingShader.Unbind();

	Shader FrameMixShader("res/Shaders/FrameMix.shader");
	FrameMixShader.Bind();
	FrameMixShader.Unbind();

	Shader ScreenBasicShader("res/Shaders/PathTracingScreenBasic.shader");
	ScreenBasicShader.Bind();
	ScreenBasicShader.Unbind();

	Shader DepthShader("res/Shaders/ShadowMap.shader");
	DepthShader.Bind();
	DepthShader.Unbind();

	Shader NormalShader("res/Shaders/NormalMap.shader");
	NormalShader.Bind();
	NormalShader.Unbind();


	glm::mat4 ViewMatrix = camera.SetView();
	glm::mat4 ProjectionMatrix = camera.SetProjection((float)WinWidth / WinHeight);
	

	Texture hdrMap("res/textures/skybox/je_gray_02_4k.hdr","HDRMap");
	Texture hdrCache("res/textures/skybox/je_gray_02_4k.hdr", "Cache");
	int hdrResolution = hdrCache.GetWidth();

	//循环显示
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{


		GLClearError();//清除错误信息

		lastFrameWeight = 2;//unsigned int(fps / 6.0);//越大拖影越重
		if (frameCounter == lastFrameWeight)
		{
			realLastFrameCounter = frameCounter;
		}

		//记录每帧的时间
		deltaTime = (float)glfwGetTime() - lastTime;
		lastTime = (float)glfwGetTime();

		LastViewMatrix = camera.SetView();
		LastProjectionMatrix = camera.SetProjection((float)WinWidth / WinHeight);

		//相机键盘输入控制
		if (camera.KeyControl(window, deltaTime))
		{
			lastFrameCounter = frameCounter;
			frameCounter = 0;
		}

		//相机键盘输入控制
		if(mousemode == MouseMode::Disable)
		{
			if (camera.MouseControl(window))
			{
				lastFrameCounter = frameCounter;
				frameCounter = 0;
			}
		}

		
		
		ViewMatrix = camera.SetView();
		ProjectionMatrix = camera.SetProjection((float)WinWidth / WinHeight);


		
		framebufferLastDepth.GetDepthBuffer(framebufferDepth.GetID());
		
		//pass0   获取深度
		DepthShader.Bind();
		DepthShader.SetUniformMat4("u_view", ViewMatrix);
		DepthShader.SetUniformMat4("u_projection", ProjectionMatrix);
		framebufferDepth.Bind();
		//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT);

		glClear(GL_DEPTH_BUFFER_BIT);
		room.Draw(DepthShader);
		bunny.Draw(DepthShader);
		
		framebufferDepth.Unbind();
		DepthShader.Unbind();

		//pass0   获取法线
		NormalShader.Bind();
		NormalShader.SetUniformMat4("u_view", ViewMatrix);
		NormalShader.SetUniformMat4("u_projection", ProjectionMatrix);
		framebufferNormal.Bind();
		glEnable(GL_DEPTH_TEST);
		glClear(GL_DEPTH_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		room.Draw(NormalShader);
		bunny.Draw(NormalShader);

		framebufferNormal.Unbind();
		NormalShader.Unbind();

		
		//pass1  记录新的帧缓冲
		PathTracingShader.Bind();
		newframebuffer.Bind();

		glBindVertexArray(QuadID);

		glDisable(GL_DEPTH_TEST);

		tboTriangles.Bind(0);
		PathTracingShader.SetUniform1i("triangles", 0);
		tboBVH.Bind(1);
		PathTracingShader.SetUniform1i("nodes", 1);

		PathTracingShader.SetUniform1i("nTriangles", nTriangles);
		PathTracingShader.SetUniform1i("nNodes", nNodes);
		PathTracingShader.SetUniform1ui("frameCounter", frameCounter);
		PathTracingShader.SetUniform1f("WinWidth", (float)WinWidth);
		PathTracingShader.SetUniform1f("WinHeight", (float)WinHeight);
		PathTracingShader.SetUniformMat4("u_view", ViewMatrix);
		PathTracingShader.SetUniformMat4("u_projection", ProjectionMatrix);
		PathTracingShader.SetUniform3f("CameraPos", camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
		hdrMap.Bind(6);
		PathTracingShader.SetUniform1i("hdrMap",6);
		hdrCache.Bind(7);
		PathTracingShader.SetUniform1i("hdrCache", 7);
		PathTracingShader.SetUniform1i("hdrResolution ", hdrResolution);
		
		glDrawArrays(GL_TRIANGLES, 0, 6);//绘制图像
		//pass1.draw();
		//pass2.draw(pass1.colorAttachments);

		glBindVertexArray(0);

		PathTracingShader.Unbind();

		newframebuffer.Unbind();
		

		//pass2 新的帧缓冲与旧的帧缓冲混合,再存入旧的帧缓冲，再供下一帧使用
		lastframebuffer.Bind();

		FrameMixShader.Bind();
		FrameMixShader.SetUniform1f("frameCounter", (float)frameCounter);
		FrameMixShader.SetUniform1f("lastFrameCounter", (float)realLastFrameCounter);
		FrameMixShader.SetUniform1f("lastFrameWeight", (float)lastFrameWeight);
		FrameMixShader.SetUniformMat4("u_view", ViewMatrix);
		FrameMixShader.SetUniformMat4("u_projection", ProjectionMatrix);
		FrameMixShader.SetUniformMat4("u_lastview", LastViewMatrix);
		FrameMixShader.SetUniformMat4("u_lastprojection", LastProjectionMatrix);
		

		glBindVertexArray(FboQuadID);
		glDisable(GL_DEPTH_TEST);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, newframebuffer.tbID);
		FrameMixShader.SetUniform1i("screenTexture", 4);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, lastframebuffer.tbID);
		FrameMixShader.SetUniform1i("screenTexture2", 5);

		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, framebufferDepth.tbID);
		FrameMixShader.SetUniform1i("depthTexture", 8);

		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, framebufferLastDepth.tbID);
		FrameMixShader.SetUniform1i("lastDepthTexture", 9);

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, framebufferNormal.tbID);
		FrameMixShader.SetUniform1i("NormalTexture", 10);

		

		glDrawArrays(GL_TRIANGLES, 0, 6);

		FrameMixShader.Unbind();
		glBindVertexArray(0);

		
		//pass3 绘制混合后的画面
		ScreenBasicShader.Bind();
		ScreenBasicShader.SetUniform1f("WinWidth", (float)WinWidth);
		ScreenBasicShader.SetUniform1f("WinHeight", (float)WinHeight);

		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, framebufferDepth.tbID);
		ScreenBasicShader.SetUniform1i("depthTexture", 8);

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, framebufferNormal.tbID);
		ScreenBasicShader.SetUniform1i("NormalTexture", 10);


		lastframebuffer.Unbind();
		
		lastframebuffer.Draw(ScreenBasicShader, FboQuadID);



		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
		



		
		
		fps = 1.0 / deltaTime;
		std::cout << "\r";
		std::cout << "当前分辨率: " << WinWidth << "x" << WinHeight<<std::fixed << std::setprecision(2) << "    平均FPS : " << fps << "    迭代次数: " << frameCounter << "次          ";
		std::cout << "\r";
		frameCounter++;
		//lastFrameCounter++;

		GLCheckError();//获取错误信息
			
	}




	glfwTerminate();
	return 0;
}

/*
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	if(mousemode == MouseMode::Disable)
	{
		
		LastViewMatrix = camera.SetView();
		LastProjectionMatrix = camera.SetProjection((float)WinWidth / WinHeight);
		camera.MouseControl(xposIn, yposIn);
		lastFrameCounter = frameCounter;
		frameCounter = 0;
	}
}
*/

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (mousemode == MouseMode::Disable)
	{
		
		LastViewMatrix = camera.SetView();
		LastProjectionMatrix = camera.SetProjection((float)WinWidth / WinHeight);
		camera.ScrollControl(xoffset, yoffset);
		lastFrameCounter = frameCounter;
		frameCounter = 0;
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	lastFrameCounter = 0;
	frameCounter = 0;
	WinWidth = width;
	WinHeight = height;
	glViewport(0, 0, WinWidth, WinHeight);//视口变换
	fblast->ResetWindow(WinWidth, WinHeight);
	fbnew->ResetWindow(WinWidth, WinHeight);
	fbDepth->ResetWindowCameraMap(WinWidth, WinHeight);
	fbLastDepth->ResetWindowCameraMap(WinWidth, WinHeight);
	fbNormal->ResetWindow(WinWidth, WinHeight);

}

void mouseClick_callback(GLFWwindow* window, int button, int action, int mods) {
	if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_RIGHT)) 
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		mousemode = MouseMode::Enable;
	}
	else if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_LEFT))
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		mousemode = MouseMode::Disable;
		camera.firstMouse = true;
	}
}