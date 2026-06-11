#shader vertex
#version 330 core 

layout(location=0) in vec3 position; 
layout(location=1) in vec2 aTexcoords;

out vec2 Texcoords;

void main() 
{ 
	gl_Position = vec4(position,1.0f);
	Texcoords=aTexcoords;
}


#shader fragment
#version 330 core 

in vec2 Texcoords;

out vec4 color; 
uniform sampler2D screenTexture;

void main() 
{
	vec3 texColor=texture(screenTexture,Texcoords).xyz;
	if(texColor.x+texColor.y+texColor.z<2.9f)
	texColor=vec3(0.0f,0.0f,0.0f);
	else
	texColor=vec3(1.0f,1.0f,1.0f);
	color =vec4(texColor,1.0f);
}