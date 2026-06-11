#shader vertex
#version 330 core 

layout(location=0) in vec3 position; 
layout(location=3) in mat4 model; 

void main() 
{ 
	gl_Position = model*vec4(position,1.0f);//先只M变换
}

#shader geometry
#version 330 core 
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 shadowMatrices[6];
out vec4 FragPos; // 由几何着色器导出该点的世界坐标

void main()
{
    for(int face = 0; face < 6; ++face)//有6个方向的点
    {
        gl_Layer = face; //通过对内建变量gl_Layer赋值来选择当前方向
        for(int i = 0; i < 3; ++i) //当前方向的三角形的3点
        {
            FragPos = gl_in[i].gl_Position;//该方向每个点的世界坐标
            gl_Position = shadowMatrices[face] * FragPos;//完成该方向每个点的VP变换
            EmitVertex();
        }    
        EndPrimitive();
    }
}

#shader fragment
#version 330 core

in vec4 FragPos;
uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    float lightDistance = length(FragPos.xyz - lightPos);//计算世界坐标系中光源到该点的距离
    lightDistance = lightDistance / far_plane;//将距离标准化为范围0到1
    gl_FragDepth = lightDistance;//对内建变量gl_FragDepth赋值，将上文算的标准化距离作为该点深度
}
