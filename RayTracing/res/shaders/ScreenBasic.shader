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
	vec4 texColor=texture(screenTexture,Texcoords);
	//if(texColor.w<0.1)
	//discard;
	color = texColor;
	//float gamma=2.2;
    //color =vec4(pow(texColor,vec3(1.0/gamma)),1.0f);
}