#shader vertex
#version 330 core 

layout(location=0) in vec3 position; 
layout(location=1) in vec3 normal;
layout(location=3) in mat4 model; //ÌùÍ¼×ø±ê

layout(std140) uniform Matrices
{
	mat4 u_view;
	mat4 u_projection;
};

void main() 
{ 
	vec3 Oposition=normalize(normal)*0.05f+position;
	vec4 Cposition=u_projection*u_view*model*vec4(Oposition,1.0f);
	Cposition.z+=0.0015f;
	gl_Position =  Cposition;

}


#shader fragment
#version 330 core 

out vec4 color; 


void main() 
{
	

	color =vec4(1.0f,1.0f,1.0f,1.0f); 
}