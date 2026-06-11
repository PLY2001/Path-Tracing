#include "Mesh.h"

#define INF 1000000

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<myTexture> textures):
	vertices(vertices),indices(indices),textures(textures)
{
	SetMesh();
}

void Mesh::SetMesh()
{
	VertexArray* va=new VertexArray(vaID);
	VertexBuffer* vb=new VertexBuffer(vbID, vertices.data(), (unsigned int)vertices.size() * sizeof(Vertex));

	VertexAttribLayout layout;//创建顶点属性布局实例
	layout.Push<GL_FLOAT>(3);//填入第一个属性布局，类型为float，每个点为3维向量
	layout.Push<GL_FLOAT>(3);//填入第二个属性布局，类型为float，每个点为2维向量
	//layout.Push<GL_FLOAT>(3);//填入第二个属性布局，类型为float，每个点为2维向量
	layout.Push<GL_FLOAT>(2);//填入第三个属性布局，类型为float，每个点为2维向量

	va->AddBuffer(vbID, layout);//将所有属性布局应用于顶点缓冲区vb，并绑定在顶点数组对象va上
	IndexBuffer* ib=new IndexBuffer(ibID,indices.data(), (unsigned int)indices.size());

	va->Unbind();
	vb->Unbind();
	ib->Unbind();
}

void Mesh::ProcessTriangle()
{
	for (int i = 0; i < (indices.size()-3); i = i + 3)
	{
		triangles.push_back(myTriangle(vertices[indices[i]].Position, vertices[indices[i + 1]].Position, vertices[indices[i + 2]].Position));
	}
	
}

void Mesh::Draw(Shader& shader)
{
	shader.Bind();
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	//unsigned int normalNr = 1;
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i); //在绑定之前激活相应的纹理单元
		//获取纹理序号,diffuse_textureN 中的 N
		std::string number;
		std::string name = textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++);
		//else if (name == "texture_normal")
		//number = std::to_string(normalNr++);
		
		shader.SetUniform1i(("material." + name + number).c_str(), i);
	    

		glBindTexture(GL_TEXTURE_2D, textures[i].slot);
	}
	glActiveTexture(GL_TEXTURE0);

	
	glBindVertexArray(vaID);
	glDrawElements(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void Mesh::DrawInstanced(Shader& shader,int amount)
{
	shader.Bind();
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	//unsigned int normalNr = 1;
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i); //在绑定之前激活相应的纹理单元
		//获取纹理序号,diffuse_textureN 中的 N
		std::string number;
		std::string name = textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++);
		//else if (name == "texture_normal")
		//	number = std::to_string(normalNr++);


		shader.SetUniform1i(("material." + name + number).c_str(), i);


		glBindTexture(GL_TEXTURE_2D, textures[i].slot);
	}
	glActiveTexture(GL_TEXTURE0);


	glBindVertexArray(vaID);
	glDrawElementsInstanced(GL_TRIANGLES, (unsigned int)indices.size(), GL_UNSIGNED_INT, nullptr,amount);
	glBindVertexArray(0);

}



// 构建 BVH
BVHTree Mesh::buildBVH(std::vector<myTriangle>& triangles, int l, int r, int n) 
{
	if (l > r) return 0;

	BVHNode* node = new BVHNode();
	node->AABBMinPos = glm::vec3(1145141919, 1145141919, 1145141919);
	node->AABBMaxPos = glm::vec3(-1145141919, -1145141919, -1145141919);

	// 计算 AABB
	for (int i = l; i <= r; i++) {
		// 最小点 AA
		float minx = glm::min(triangles[i].p1.x, glm::min(triangles[i].p2.x, triangles[i].p3.x));
		float miny = glm::min(triangles[i].p1.y, glm::min(triangles[i].p2.y, triangles[i].p3.y));
		float minz = glm::min(triangles[i].p1.z, glm::min(triangles[i].p2.z, triangles[i].p3.z));
		node->AABBMinPos.x = glm::min(node->AABBMinPos.x, minx);
		node->AABBMinPos.y = glm::min(node->AABBMinPos.y, miny);
		node->AABBMinPos.z = glm::min(node->AABBMinPos.z, minz);
		// 最大点 BB
		float maxx = glm::max(triangles[i].p1.x, glm::max(triangles[i].p2.x, triangles[i].p3.x));
		float maxy = glm::max(triangles[i].p1.y, glm::max(triangles[i].p2.y, triangles[i].p3.y));
		float maxz = glm::max(triangles[i].p1.z, glm::max(triangles[i].p2.z, triangles[i].p3.z));
		node->AABBMaxPos.x = glm::max(node->AABBMaxPos.x, maxx);
		node->AABBMaxPos.y = glm::max(node->AABBMaxPos.y, maxy);
		node->AABBMaxPos.z = glm::max(node->AABBMaxPos.z, maxz);
	}

	// 不多于 n 个三角形 返回叶子节点
	if ((r - l + 1) <= n) {
		node->n = r - l + 1;
		node->index = l;
		return node;
	}

	// 否则递归建树
	float lenx = node->AABBMaxPos.x - node->AABBMinPos.x;
	float leny = node->AABBMaxPos.y - node->AABBMinPos.y;
	float lenz = node->AABBMaxPos.z - node->AABBMinPos.z;
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
	node->left = buildBVH(triangles, l, mid, n);
	node->right = buildBVH(triangles, mid + 1, r, n);

	return node;
}


// SAH 优化构建 BVH
BVHTree Mesh::buildBVHwithSAH(std::vector<myTriangle>& triangles, int l, int r, int n) 
{
	if (l > r) return 0;

	BVHNode* node = new BVHNode();
	node->AABBMinPos = glm::vec3(1145141919, 1145141919, 1145141919);
	node->AABBMaxPos = glm::vec3(-1145141919, -1145141919, -1145141919);

	// 计算 AABB
	for (int i = l; i <= r; i++) {
		// 最小点 AA
		float minx = glm::min(triangles[i].p1.x, glm::min(triangles[i].p2.x, triangles[i].p3.x));
		float miny = glm::min(triangles[i].p1.y, glm::min(triangles[i].p2.y, triangles[i].p3.y));
		float minz = glm::min(triangles[i].p1.z, glm::min(triangles[i].p2.z, triangles[i].p3.z));
		node->AABBMinPos.x = glm::min(node->AABBMinPos.x, minx);
		node->AABBMinPos.y = glm::min(node->AABBMinPos.y, miny);
		node->AABBMinPos.z = glm::min(node->AABBMinPos.z, minz);
		// 最大点 BB
		float maxx = glm::max(triangles[i].p1.x, glm::max(triangles[i].p2.x, triangles[i].p3.x));
		float maxy = glm::max(triangles[i].p1.y, glm::max(triangles[i].p2.y, triangles[i].p3.y));
		float maxz = glm::max(triangles[i].p1.z, glm::max(triangles[i].p2.z, triangles[i].p3.z));
		node->AABBMaxPos.x = glm::max(node->AABBMaxPos.x, maxx);
		node->AABBMaxPos.y = glm::max(node->AABBMaxPos.y, maxy);
		node->AABBMaxPos.z = glm::max(node->AABBMaxPos.z, maxz);
	}

	// 不多于 n 个三角形 返回叶子节点
	if ((r - l + 1) <= n) {
		node->n = r - l + 1;
		node->index = l;
		return node;
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
			myTriangle& t = triangles[i];
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
			myTriangle& t = triangles[i];
			int bias = (i == r) ? 0 : 1;  // 第一个元素特殊处理

			rightMax[i - l].x = glm::max(rightMax[i - l + bias].x,glm::max(t.p1.x,glm::max(t.p2.x, t.p3.x)));
			rightMax[i - l].y = glm::max(rightMax[i - l + bias].y,glm::max(t.p1.y,glm::max(t.p2.y, t.p3.y)));
			rightMax[i - l].z = glm::max(rightMax[i - l + bias].z,glm::max(t.p1.z,glm::max(t.p2.z, t.p3.z)));

			rightMin[i - l].x = glm::min(rightMin[i - l + bias].x,glm::min(t.p1.x,glm::min(t.p2.x, t.p3.x)));
			rightMin[i - l].y = glm::min(rightMin[i - l + bias].y,glm::min(t.p1.y,glm::min(t.p2.y, t.p3.y)));
			rightMin[i - l].z = glm::min(rightMin[i - l + bias].z,glm::min(t.p1.z,glm::min(t.p2.z, t.p3.z)));
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
	node->left = buildBVHwithSAH(triangles, l, Split, n);
	node->right = buildBVHwithSAH(triangles, Split + 1, r, n);

	return node;
}