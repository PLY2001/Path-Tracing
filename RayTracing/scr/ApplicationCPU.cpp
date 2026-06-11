#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include <random>

#include "ErrorCheck.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image/stb_image.h"
#include "Shader.h"
#include "VertexArray.h"
#include "VertexBuffer.h"
#include "Model.h"

#include <omp.h>    // openmp多线程加速

#define PI 3.1415926
#define INF 1000000

// 颜色
const glm::vec3 RED(1, 0.5, 0.5);
const glm::vec3 GREEN(0.5, 1, 0.5);
const glm::vec3 BLUE(0.5, 0.5, 1);
const glm::vec3 YELLOW(1.0, 1.0, 0.1);
const glm::vec3 CYAN(0.1, 1.0, 1.0);
const glm::vec3 MAGENTA(1.0, 0.1, 1.0);
const glm::vec3 GRAY(0.5, 0.5, 0.5);
const glm::vec3 WHITE(1, 1, 1);

// 采样次数
const int SAMPLE = 128;
// 每次采样的亮度
const double BRIGHTNESS = (2.0f * 3.1415926f) * (1.0f / double(SAMPLE));


//光线
typedef struct Ray
{
	glm::vec3 startPoint = glm::vec3(0, 0, 0);   //起点
	glm::vec3 direction = glm::vec3(0, 0, 0);   //方向
}Ray;

//材质
typedef struct Material
{
	bool isEmissive = false;        // 是否发光
	glm::vec3 normal = glm::vec3(0, 0, 0);    // 法向量
	glm::vec3 color = glm::vec3(0, 0, 0);     // 颜色
	double specularRate = 0.0f;      // 镜面度
	double roughness = 1.0f;        // 粗糙度
}Material;

// 光线求交结果
typedef struct HitResult
{
	bool isHit = false;             // 是否命中
	double distance = INF;         // 光线起点与命中点的距离
	glm::vec3 hitPoint = glm::vec3(0, 0, 0);  // 光线命中点
	Material material;              // 命中点的表面材质
}HitResult;


//形状基类
class Shape
{
public:
	Shape() {}
	virtual HitResult intersect(Ray ray) { return HitResult(); }
};

// 三角形
class Triangle : public Shape
{
public:
	Triangle() {}
	Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, glm::vec3 C, double s, double rough)//三角形参数构造
	{
		p1 = P1, p2 = P2, p3 = P3;
		material.normal = glm::normalize(glm::cross(p2 - p1, p3 - p1)); material.color = C;
		material.specularRate = s;
		material.roughness = rough;
		
	}
	Triangle(glm::vec3 P1, glm::vec3 P2, glm::vec3 P3, glm::vec3 C)
	{
		p1 = P1, p2 = P2, p3 = P3;
		material.normal = glm::normalize(glm::cross(p2 - p1, p3 - p1)); material.color = C;

	}
	glm::vec3 p1, p2, p3;    // 三顶点
	Material material;  // 材质

	// 三角与光线求交函数
	HitResult intersect(Ray ray)
	{
		HitResult res;

		glm::vec3 O = ray.startPoint;        // 射线起点
		glm::vec3 D = ray.direction;         // 射线方向
		glm::vec3 N = material.normal;       // 法向量
		if (glm::dot(N, D) > 0.0f) N = -N;   // 获取正确的法向量

		// 如果视线和三角形平行
		if (fabs(glm::dot(N, D)) < 0.00001f) return res;

		//求交计算
		glm::vec3 E1 = p2 - p1;
		glm::vec3 E2 = p3 - p1;
		glm::vec3 S = O - p1;
		glm::vec3 S1 = glm::cross(D,E2);
		glm::vec3 S2 = glm::cross(S, E1);

		// 距离
		float t = glm::dot(S2, E2) / glm::dot(S1, E1);
		if (t < 0.0005f) return res;    // 如果三角形在相机背面
		
		float b1 = glm::dot(S1, S) / glm::dot(S1, E1);
		float b2 = glm::dot(S2, D) / glm::dot(S1, E1);
		
		// 判断交点是否在三角形中
		if (b1 < 0 || b2 < 0 || (1 - b1 - b2) < 0) return res;



		// 得到交点
		glm::vec3 P = O + D * t;

		// 装填返回结果
		res.isHit = true;
		res.distance = t;
		res.hitPoint = P;
		res.material = material;
		res.material.normal = N;    // 要返回正确的法向
		return res;
	};
};

// 球
class Sphere : public Shape
{
public:
	Sphere() {}
	Sphere(glm::vec3 o, double r, glm::vec3 c, double s, double rough) { O = o; R = r; material.color = c;  material.specularRate = s;  material.roughness = rough; }//球的参数构造
	Sphere(glm::vec3 o, double r, glm::vec3 c) { O = o; R = r; material.color = c; }
	glm::vec3 O;             // 圆心
	double R;           // 半径
	Material material;  // 材质

	// 球与光线求交函数
	HitResult intersect(Ray ray)
	{
		HitResult res;

		glm::vec3 S = ray.startPoint;        // 射线起点
		glm::vec3 d = ray.direction;         // 射线方向

		float t1 = glm::dot(d, S - O);
		float t2 = glm::dot(S - O, S - O);
		float t3 = R*R;
		if ((t1 * t1 - t2 + t3) < 0) return res;
		float t4 = (-t1 + glm::sqrt(t1 * t1 - t2 + t3))/glm::length(d);
		float t5 = (-t1 - glm::sqrt(t1 * t1 - t2 + t3)) / glm::length(d);
		if (t4 < 0 || t5 < 0) return res;

		float t = (t4 < t5) ? (t4) : (t5);   // 最近距离

		
		glm::vec3 P = S + t * d;     // 交点

		// 防止自己交自己
		if (fabs(t4) < 0.0005f || fabs(t5) < 0.0005f) return res;

		// 装填返回结果
		res.isHit = true;
		res.distance = t;
		res.hitPoint = P;
		res.material = material;
		res.material.normal = normalize(P - O); // 要返回正确的法向
		return res;
	}
};

// 返回距离光线起点最近命中点的结果
HitResult shoot(std::vector<Shape*>& shapes, Ray ray)
{
	HitResult res, r;
	res.distance = INF;

	// 遍历所有图形，求最近交点
	for (auto& shape : shapes)
	{
		r = shape->intersect(ray);
		if (r.isHit && r.distance < res.distance) res = r;  // 记录距离最近的求交结果
	}

	return res;
}


// 网格三角形
class MeshTriangle : public Shape
{
public:
	MeshTriangle(Mesh mesh, glm::vec3 C, double s, double rough)//三角形参数构造
	{
		mmesh = mesh;
		for (myTriangle meshtriangle : mesh.triangles)
		{
			triangles.push_back(Triangle(meshtriangle.p1, meshtriangle.p2, meshtriangle.p3, C,s,rough));
		}
		
	}
	MeshTriangle(Mesh mesh, glm::vec3 C)
	{
		mmesh = mesh;
		for (myTriangle meshtriangle : mesh.triangles)
		{
			triangles.push_back(Triangle(meshtriangle.p1, meshtriangle.p2, meshtriangle.p3, C));
		}

	}
	Mesh mmesh;
	std::vector<Triangle> triangles;
	
	// 和 aabb 盒子求交，没有交点则返回 -1
	float hitAABB(Ray r, glm::vec3 AABBMinPos, glm::vec3 AABBMaxPos) {
		// 1.0 / direction
		glm::vec3 invdir = glm::vec3(1.0 / r.direction.x, 1.0 / r.direction.y, 1.0 / r.direction.z);

		glm::vec3 in = (AABBMaxPos - r.startPoint) * invdir;
		glm::vec3 out = (AABBMinPos - r.startPoint) * invdir;

		glm::vec3 tmax = glm::max(in, out);
		glm::vec3 tmin = glm::min(in, out);

		float t1 = glm::min(tmax.x, glm::min(tmax.y, tmax.z));
		float t0 = glm::max(tmin.x, glm::max(tmin.y, tmin.z));

		return (t1 >= t0) ? ((t0 > 0.0) ? (t0) : (t1)) : (-1);
	}

	// 在 BVH 上遍历求交
	HitResult hitBVH(Ray ray, BVHTree root) 
	{
		if (root == NULL) return HitResult();

		// 是叶子 暴力查
		if (root->n > 0) {
			return hitTriangleArray(ray, root->n, root->n + root->index - 1);
		}

		// 和左右子树 AABB 求交
		float d1 = INF, d2 = INF;
		if (root->left) d1 = hitAABB(ray, root->left->AABBMinPos, root->left->AABBMaxPos);
		if (root->right) d2 = hitAABB(ray, root->right->AABBMinPos, root->right->AABBMaxPos);

		// 递归结果
		HitResult r1, r2;
		if (d1 > 0) r1 = hitBVH(ray,  root->left);
		if (d2 > 0) r2 = hitBVH(ray,  root->right);

		return r1.distance < r2.distance ? r1 : r2;
	}

	// 暴力查数组
	HitResult hitTriangleArray(Ray ray, int l, int r) 
	{
		HitResult res;
		for (int i = l; i <= r; i++) {
			HitResult thisres = triangles[i].intersect(ray);
			if (thisres.distance < INF && thisres.distance < res.distance) {
				res = thisres;
			}
		}
		return res;
	}

	// 三角与光线求交函数
	HitResult intersect(Ray ray)
	{
		
		return hitBVH(ray, mmesh.bvhtree);
	}
};

// 0-1 随机数生成
std::uniform_real_distribution<> dis(0.0, 1.0);
std::random_device rd;
std::mt19937 gen(rd());
double randf()
{
	return dis(gen);
}

// 生成单位球内的随机向量（坐标范围-1到1）
glm::vec3 randomVec3()
{
	glm::vec3 d;
	do
	{
		d = 2.0f * glm::vec3(randf(), randf(), randf()) - glm::vec3(1, 1, 1);
	} while (dot(d, d) > 1.0);
	return normalize(d);

}

// 通过将单位球内的随机向量偏移到单位法向量终点，从而使随机向量分布于该法向的半球内（太妙了！）
glm::vec3 randomDirection(glm::vec3 n)
{
	// 法向球
	return normalize(randomVec3() + n);
}



// 路径追踪
glm::vec3 pathTracing(std::vector<Shape*>& shapes, Ray ray, int depth)//depth为间接反射次数
{
	if (depth > 8) return glm::vec3(0);  //间接反射最多8次
	HitResult res = shoot(shapes, ray);  //对所有形状发射光线，返回最近的命中点结果

	if (!res.isHit) return glm::vec3(0); // 未命中，中止递归

	if (res.material.isEmissive) return res.material.color;// 如果命中发光体则返回发光颜色，中止递归

	// 有 P 的概率终止递归
	double r = randf();
	float P = 0.8;
	if (r > P) return glm::vec3(0);

	// 递归继续
	Ray randomRay;//创建新的反射光线
	randomRay.startPoint = res.hitPoint;
	randomRay.direction = randomDirection(res.material.normal);//反射方向为法线半球内随机单位方向

	glm::vec3 color = glm::vec3(0);
	float cosine = fabs(dot(-ray.direction, res.material.normal));

	// 根据反射率决定光线最终的方向
	r = randf();//随机判断是否需要镜面反射，镜面度specularRate越大，概率越大
	if (r < res.material.specularRate)  // 镜面反射
	{
		glm::vec3 ref = glm::normalize(glm::reflect(ray.direction, res.material.normal));
		randomRay.direction = glm::mix(ref, randomRay.direction, res.material.roughness);//用mix混合函数给镜面反射方向来点随机偏移，模拟粗糙度
		color = pathTracing(shapes, randomRay, depth + 1) * cosine; //路径追踪反射方程
	}
	else    // 漫反射
	{
		glm::vec3 srcColor = res.material.color;
		glm::vec3 ptColor = pathTracing(shapes, randomRay, depth + 1) * cosine; //路径追踪反射方程
		color = ptColor * srcColor;    // 和材质颜色混合
	}

	return color / P; //除以P保证概率期望不变

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

unsigned int ReadPhoto(int* WinWidth, int* WinHeight, unsigned char* LocalBuffer)
{
	unsigned int tbID;
	glGenTextures(1, &tbID);
	glBindTexture(GL_TEXTURE_2D, tbID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);//设置纹理过滤方式（必须设置）
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);//设置纹理过滤方式（必须设置）
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);//设置纹理环绕方式（必须设置）
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);//设置纹理环绕方式（必须设置）

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, *WinWidth, *WinHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, LocalBuffer);//将读取的图片存入贴图
	glBindTexture(GL_TEXTURE_2D, 0);

	//if (LocalBuffer)//存入贴图成功后，可以清除读取的图片缓存
	//	stbi_image_free(LocalBuffer);
	return tbID;
}




int main(void)
{

	std::vector<Shape*> shapes;  // 几何物体的集合
	// 球
	//shapes.push_back(new Sphere(glm::vec3(-0.6, -0.8, 0.6), 0.2, WHITE, 1.0, 0.0));
	//shapes.push_back(new Sphere(glm::vec3(-0.1, -0.7, 0.2), 0.3, WHITE, 0.9, 0.2));
	//shapes.push_back(new Sphere(glm::vec3(0.5, -0.6, -0.5), 0.4, WHITE, 0.9, 0.6));
	
	// 光源
	Triangle l1 = Triangle(glm::vec3(1.0, 0.99, 0.8), glm::vec3(-1.0, 0.99, 0.6), glm::vec3(-1.0, 0.99, 0.8), GRAY);
	Triangle l2 = Triangle(glm::vec3(1.0, 0.99, 0.8), glm::vec3(1.0, 0.99, 0.6), glm::vec3(-1.0, 0.99, 0.6), GRAY);
	Triangle l3 = Triangle(glm::vec3(1.0, 0.99, 0.4), glm::vec3(-1.0, 0.99, 0.2), glm::vec3(-1.0, 0.99, 0.4), GRAY);
	Triangle l4 = Triangle(glm::vec3(1.0, 0.99, 0.4), glm::vec3(1.0, 0.99, 0.2), glm::vec3(-1.0, 0.99, 0.2), GRAY);
	Triangle l5 = Triangle(glm::vec3(1.0, 0.99, 0.0), glm::vec3(-1.0, 0.99, -0.2), glm::vec3(-1.0, 0.99, 0.0), GRAY);
	Triangle l6 = Triangle(glm::vec3(1.0, 0.99, 0.0), glm::vec3(1.0, 0.99, -0.2), glm::vec3(-1.0, 0.99, -0.2), GRAY);
	Triangle l7 = Triangle(glm::vec3(1.0, 0.99, -0.4), glm::vec3(-1.0, 0.99, -0.6), glm::vec3(-1.0, 0.99, -0.4), GRAY);
	Triangle l8 = Triangle(glm::vec3(1.0, 0.99, -0.4), glm::vec3(1.0, 0.99, -0.6), glm::vec3(-1.0, 0.99, -0.6), GRAY);
	Triangle l9 = Triangle(glm::vec3(1.0, 0.99, -0.8), glm::vec3(-1.0, 0.99, -1.0), glm::vec3(-1.0, 0.99, -0.8), GRAY);
	Triangle l10 = Triangle(glm::vec3(1.0, 0.99, -0.8), glm::vec3(1.0, 0.99, -1.0), glm::vec3(-1.0, 0.99, -1.0), GRAY);
	l1.material.isEmissive = true;
	l2.material.isEmissive = true;
	l3.material.isEmissive = true;
	l4.material.isEmissive = true;
	l5.material.isEmissive = true;
	l6.material.isEmissive = true;
	l7.material.isEmissive = true;
	l8.material.isEmissive = true;
	l9.material.isEmissive = true;
	l10.material.isEmissive = true;
	shapes.push_back(&l1);
	shapes.push_back(&l2);
	shapes.push_back(&l3);
	shapes.push_back(&l4);
	shapes.push_back(&l5);
	shapes.push_back(&l6);
	shapes.push_back(&l7);
	shapes.push_back(&l8);
	shapes.push_back(&l9);
	shapes.push_back(&l10);

	// 背景盒子
	// bottom
	shapes.push_back(new Triangle(glm::vec3(1, -1, 1), glm::vec3(-1, -1, -1),glm::vec3(-1, -1, 1), WHITE));
	shapes.push_back(new Triangle(glm::vec3(1, -1, 1), glm::vec3(1, -1, -1),glm::vec3(-1, -1, -1), WHITE));
	// top
	shapes.push_back(new Triangle(glm::vec3(1, 1, 1), glm::vec3(-1, 1, 1),glm::vec3(-1, 1, -1), WHITE));
	shapes.push_back(new Triangle(glm::vec3(1, 1, 1), glm::vec3(-1, 1, -1),glm::vec3(1, 1, -1), WHITE));
	// back
	shapes.push_back(new Triangle(glm::vec3(1, -1, -1),glm::vec3(-1, 1, -1),glm::vec3(-1, -1, -1), CYAN));
	shapes.push_back(new Triangle(glm::vec3(1, -1, -1),glm::vec3(1, 1, -1),glm::vec3(-1, 1, -1), CYAN));
	// left
	shapes.push_back(new Triangle(glm::vec3(-1, -1, -1),glm::vec3(-1, 1, 1),glm::vec3(-1, -1, 1), RED));
	shapes.push_back(new Triangle(glm::vec3(-1, -1, -1),glm::vec3(-1, 1, -1),glm::vec3(-1, 1, 1), RED));
	// right
	shapes.push_back(new Triangle(glm::vec3(1, 1, 1),glm::vec3(1, -1, -1),glm::vec3(1, -1, 1), YELLOW));
	shapes.push_back(new Triangle(glm::vec3(1, -1, -1),glm::vec3(1, 1, 1),glm::vec3(1, 1, -1), YELLOW));


	


	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
	{
		std::cout << "glfwInit FAIL!" << std::endl;
		return -1;
	}



	//window size
	int WinWidth = 1000;/////////////////////////窗口分辨率
	int WinHeight = 1000;




	




	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WinWidth, WinHeight, "PathTracing CPU", NULL, NULL);////////////////创建窗口，名字为Heart
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	glfwSwapInterval(1);

	if (glewInit())
	{
		std::cout << "glewInit FAIL!" << std::endl;
	}


	/*球*/
	Model bunny("res/models/scene.obj", glm::vec3(0.0f, 0.0f, 0.0f));//读取模型，目录从当前项目根目录开始，或者生成的exe根目录。需将noise.jpg复制到每一个模型旁边。
	std::cout << "start process triangle" << std::endl;
	bunny.meshes[0].ProcessTriangle();
	std::cout << "start buildBVH" << std::endl;
	bunny.meshes[0].bvhtree = bunny.meshes[0].buildBVHwithSAH(bunny.meshes[0].triangles, 0, bunny.meshes[0].triangles.size() - 1, 8);
	std::cout << "finish buildBVH" << std::endl;
	shapes.push_back(new MeshTriangle(bunny.meshes[0], RED,0.9,0.2));

	

	unsigned int QuadID = GenQuad();

	Shader ScreenShader("res/Shaders/ScreenBasic.shader");
	ScreenShader.Bind();
	ScreenShader.Unbind();



	int BlackWidth, BlackHeight, BlackBBP;
	stbi_set_flip_vertically_on_load(1);//垂直翻转纹理
	unsigned char* BlackLocalBuffer = stbi_load("res/Photos/black512.jpg", &BlackWidth, &BlackHeight, &BlackBBP, 4);//////////读取图片数据存到BlackLocalBuffer数组中，用它作为绘制的载体，我称之画布
	unsigned int BlacktbID = ReadPhoto(&BlackWidth, &BlackHeight, BlackLocalBuffer);

	int size = BlackWidth * BlackHeight * 4;
	float* FinalColor = new float[size]; //暂时存储最终输出颜色

	omp_set_num_threads(50); // 线程个数。要想使用多线程，每个循环需要相互独立。需要开启项目属性-c/c++-语言-openMP支持，关闭项目属性-c/c++-语言-符合模式，最后#include <omp.h>
	#pragma omp parallel for //开启线程
	for (int k = 0; k < SAMPLE; k++)//总渲染采样次数
	{
		for (int i = 0; i < BlackWidth; i++)
		{
			for (int j = 0; j < BlackHeight; j++) //光线总数（屏幕分辨率）
			{
				// 像素平面坐标转世界坐标
				double x = 2.0 * double(i) / double(BlackWidth) - 1.0;
				double y = 2.0 * double(j) / double(BlackHeight) - 1.0;

				glm::vec3 coord = glm::vec3(x, y, 1.1f);          // 像素平面对应的世界坐标，作为光线起点
				glm::vec3 direction = glm::normalize(coord - glm::vec3(0.0f, 0.0f, 4.0f));    // 从摄像机向像素平面对应的世界坐标射出，作为光线方向

				// 生成光线
				Ray ray;
				ray.startPoint = coord;
				ray.direction = direction;

				// 找交点
				HitResult res = shoot(shapes, ray);
				glm::vec3 color = glm::vec3(0.0f);//预设输出颜色

				if (res.isHit)//以下内容就是pathTracing函数的内容，只是第一步还未进入递归，要单独写出来
				{
					// 命中光源直接返回光源颜色
					if (res.material.isEmissive)
					{
						color = res.material.color;
					}
					// 命中实体则选择一个随机方向重新发射光线并且进行路径追踪
					else
					{
						// 根据交点处法向量生成交点处反射的随机半球向量
						Ray randomRay;
						randomRay.startPoint = res.hitPoint;
						randomRay.direction = randomDirection(res.material.normal);

						// 根据反射率决定光线最终的方向
						double r = randf();
						if (r < res.material.specularRate)  // 镜面反射
						{
							glm::vec3 ref = glm::normalize(glm::reflect(ray.direction, res.material.normal));
							randomRay.direction = glm::mix(ref, randomRay.direction, res.material.roughness);
							color = pathTracing(shapes, randomRay, 0);
						}
						else    // 漫反射
						{
							glm::vec3 srcColor = res.material.color;
							glm::vec3 ptColor = pathTracing(shapes, randomRay, 0);
							color = ptColor * srcColor;    // 和原颜色混合
						}

						color *= BRIGHTNESS;//调整伽马值
					}
				}
				
				FinalColor[4 * (i + BlackWidth * j)] += color.x;//r通道
				FinalColor[4 * (i + BlackWidth * j) + 1] += color.y;//g通道
				FinalColor[4 * (i + BlackWidth * j) + 2] += color.z;//b通道

			}
		}
	}

	for (int i = 0; i < BlackWidth; i++)//设置画布颜色
	{
		for (int j = 0; j < BlackHeight; j++)
		{
			BlackLocalBuffer[4 * (i + BlackWidth * j)] = glm::clamp(FinalColor[4 * (i + BlackWidth * j)],0.0f,1.0f)*255.0f;//r通道
			BlackLocalBuffer[4 * (i + BlackWidth * j) + 1] = glm::clamp(FinalColor[4 * (i + BlackWidth * j) + 1],0.0f,1.0f)*255.0f;//g通道
			BlackLocalBuffer[4 * (i + BlackWidth * j) + 2] = glm::clamp(FinalColor[4 * (i + BlackWidth * j) + 2],0.0f,1.0f)*255.0f;//b通道
		}
	}

	std::cout << "Show!" << std::endl;

	//循环显示
	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		

		GLClearError();//清除错误信息

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ScreenShader.Bind();

		glBindVertexArray(QuadID);

		glDisable(GL_DEPTH_TEST);


		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, BlacktbID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, BlackWidth, BlackHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, BlackLocalBuffer);
		ScreenShader.SetUniform1i("screenTexture", 1);



		glDrawArrays(GL_TRIANGLES, 0, 6);//绘制图像

		glBindVertexArray(0);

		ScreenShader.Unbind();




		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
		GLCheckError();//获取错误信息	
	}



	glfwTerminate();
	return 0;
}

