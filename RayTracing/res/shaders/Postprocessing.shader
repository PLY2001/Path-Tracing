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

float kernel[3]=float[](1,-8,1);
float width=360;
float height=240;
float WOff = 1.0f/width;
float HOff = 1.0f/height;
float offsetsW[3] = float[](-WOff,0.0f,WOff);
float offsetsH[3] = float[](-HOff,0.0f,HOff);

void main() 
{
	vec3 texColor=vec3(0.0f);
	for(int i=0;i<3;i++)
	{
		texColor+=kernel[i]*texture(screenTexture,vec2(Texcoords.x+offsetsW[i],Texcoords.y)).xyz;
	}
	for(int i=0;i<3;i++)
	{
		texColor+=kernel[i]*texture(screenTexture,vec2(Texcoords.x,Texcoords.y+offsetsH[i])).xyz;
	}
	color =vec4(texColor,1.0f);
}