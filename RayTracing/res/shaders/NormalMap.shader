#shader vertex
#version 330 core 

layout(location=0) in vec3 position; 
layout(location=1) in vec3 normal;

out vec3 Normal;

uniform mat4 u_view;
uniform mat4 u_projection;

void main() 
{ 
	Normal = normal*0.5+vec3(0.5);
	gl_Position = u_projection*u_view*vec4(position,1.0f);

}


#shader fragment
#version 330 core 

in vec3 Normal;
out vec4 color;

void main() 
{
	color = vec4(Normal,1.0);
}