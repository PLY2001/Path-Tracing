#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

#include "Shader.h"
#include "Renderer.h"
#include "Texture.h"
#include "Camera.h"
#include "Model.h"

#include "FrameBuffer.h"
#include "Skybox.h"

#include "UniformBuffer.h"
#include "InstanceBuffer.h"

#include "Light.h"


//window size
unsigned int WinWidth = 1280;
unsigned int WinHeight = 720;
unsigned int ShadowMapWidth = 1024;
unsigned int ShadowMapHeight = 1024;

//camera
Camera camera;

//control
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
float deltaTime = 0;//每次循环耗时
float lastTime = 0;

unsigned int ModelCount = 1;//模型的数量



//为了修改窗口尺寸
FrameBuffer* fbMSAA;
FrameBuffer* fb3;


//防止连按
unsigned int FrameCount = 0;

int main(void)
{
	
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
	{
		std::cout << "glfwInit FAIL!" << std::endl;
		return -1;
	}
		

	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	//开启多重采样抗锯齿4x
	glfwWindowHint(GLFW_SAMPLES, 4);
	glEnable(GL_MULTISAMPLE);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(WinWidth, WinHeight, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//control
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);//每当检测到窗口尺寸改变就回调framebuffer_size_callback函数
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	glfwSwapInterval(1);

	if(glewInit())
	{
		std::cout << "glewInit FAIL!" << std::endl;
	}

	std::cout << glGetString(GL_VERSION) << std::endl;
	
	

	/*球*/
	Model sphere("res/models/Sphere.obj",glm::vec3(0.0f, 1.0f, 0.0f));//读取模型，目录从当前项目根目录开始，或者生成的exe根目录。需将noise.jpg复制到每一个模型旁边。

	//创建变换矩阵
	std::vector<glm::mat4> ModelMatrices;//生成模型的model变换矩阵数组
	ModelMatrices.push_back(sphere.mModelMatrix);
	glm::mat4 ViewMatrix;
	glm::mat4 ProjectionMatrix;

	//创建实例化数组
	InstanceBuffer insbosphere(sizeof(glm::mat4), &sphere.mModelMatrix);//创建实例化数组
	insbosphere.AddInstanceBuffermat4(sphere.meshes[0].vaID, 3);
	

	/*平面*/
	Model plane("res/models/plane.obj", glm::vec3(0.0f, 0.0f, 0.0f));
	InstanceBuffer insboplane(sizeof(glm::mat4), &plane.mModelMatrix);//创建实例化数组
	insboplane.AddInstanceBuffermat4(plane.meshes[0].vaID, 3);
	
														

	//读取shader，目录从当前项目根目录开始，或者生成的exe根目录
	Shader shader("res/shaders/Basic.shader");
	shader.Bind();
	shader.Unbind();

	Shader PlaneShader("res/shaders/Plane.shader");
	shader.Bind();
	shader.Unbind();
	
	Shader SkyShader("res/shaders/Skybox.shader");
	SkyShader.Bind();
	SkyShader.Unbind();

	Shader ScreenBasicShader("res/shaders/ScreenBasic.shader");
	ScreenBasicShader.Bind();
	ScreenBasicShader.Unbind();

	
	
	

	
	
	//渲染
	Renderer renderer;

	

	//创建帧缓冲MSAA
	FrameBuffer framebufferMSAA(WinWidth, WinHeight);
	fbMSAA = &framebufferMSAA;
	framebufferMSAA.GenTexture2DMultiSample(4);
	//创建帧缓冲3
	FrameBuffer framebuffer3(WinWidth, WinHeight);
	fb3 = &framebuffer3;
	framebuffer3.GenTexture2D();
	unsigned int QuadID = framebuffer3.GenQuad();//用于绘制贴图的四边形
	
	

	//创建天空盒
	Skybox skybox("Church");
	unsigned int SkyboxID = skybox.GenBox();//用于绘制天空盒的立方体

	//因为天空盒需要，设置深度函数
	glDepthFunc(GL_LEQUAL);

	//创建Uniform缓冲对象
	UniformBuffer ubo(2 * sizeof(glm::mat4),0);
	std::vector<int> shaderIDs;
	shaderIDs.push_back(shader.RendererID);
	ubo.Bind(shaderIDs, "Matrices");
	
	
	//增加模型
	for (int i = 0; i < 2; ++i)
	{
		ModelMatrices.push_back(glm::translate(sphere.mModelMatrix, glm::vec3(ModelCount * 3.0f, 0.0f, 0.0f)));
		ModelCount++;
	}
	

	

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		GLClearError();//清除错误信息

		//相机键盘输入控制
		camera.KeyControl(window, deltaTime);


		
		
		
		

		//记录每帧的时间
		deltaTime = (float)glfwGetTime() - lastTime;
		lastTime = (float)glfwGetTime();

		//设置变换矩阵
		
		ViewMatrix = camera.SetView();
		ProjectionMatrix = camera.SetProjection((float)WinWidth / WinHeight);

		//将model矩阵数组填入实例化数组
		insbosphere.SetDatamat4(sizeof(glm::mat4) * ModelCount, ModelMatrices.data());

		//向uniform缓冲对象填入view、projection矩阵数据
		ubo.SetDatamat4(0,sizeof(glm::mat4), &ViewMatrix);
		ubo.SetDatamat4(sizeof(glm::mat4),sizeof(glm::mat4), &ProjectionMatrix);
		
		

		/* Render here */
		
	

		
			
		
		


		//first pass
		framebufferMSAA.Bind();//使用带抗锯齿的帧缓冲
		
		renderer.ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		renderer.ClearDepth();
		//glEnable(GL_DEPTH_TEST);//zbuffer		
		
		
		
		
		//second pass
		//render stuff
		renderer.CullFace("BACK");

		shader.Bind();

		//向shader发送灯光位置和相机位置
		
		shader.SetUniform4f("u_CameraPosition", camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z, 1.0f);
		//向shader发送贴图单元索引
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemapTexture);
		shader.SetUniform1i("skybox", 5);

		sphere.DrawInstanced(shader,ModelCount);
		
		
		
		
		shader.Unbind();
		
		PlaneShader.Bind();

		//向PlaneShader发送灯光位置和相机位置

		PlaneShader.SetUniform4f("u_CameraPosition", camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z, 1.0f);
		//向PlaneShader发送贴图单元索引
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skybox.cubemapTexture);
		PlaneShader.SetUniform1i("skybox", 5);

		

		plane.Draw(PlaneShader);


		PlaneShader.Unbind();
		
		
		
			

			//天空盒
			//third pass
			SkyShader.Bind();
			
			glm::mat4 SkyboxViewMatrix = glm::mat4(glm::mat3(camera.SetView()));//有位移前看出来是个很小的方块，去除位移后，方块就始终套在相机上，所以满眼都是天空盒了
			SkyShader.SetUniformMat4("view", SkyboxViewMatrix);
			SkyShader.SetUniformMat4("projection", ProjectionMatrix);
			skybox.Draw(SkyShader,SkyboxID);
			
			//framebufferMSAA.ShowColorAfterMSAA(framebufferMSAA.GetID());//从抗锯齿帧缓冲获取颜色到默认帧缓冲0，可直接显示
			framebuffer3.GetColorAfterMSAA(framebufferMSAA.GetID());//从抗锯齿帧缓冲获取颜色到其他缓冲
			framebuffer3.Unbind();
			framebuffer3.Draw(ScreenBasicShader, QuadID);


			
			//forth pass
			
			
			
			
			

			
			
			
		

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
		GLCheckError();//获取错误信息	
	}

	

	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	WinWidth = width;
	WinHeight = height;
	glViewport(0, 0, WinWidth, WinHeight);//视口变换
	fbMSAA->ResetWindowMultiSample(WinWidth, WinHeight);
	fb3->ResetWindow(WinWidth, WinHeight);
	

}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	camera.MouseControl(xposIn, yposIn);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ScrollControl(xoffset, yoffset);
}

