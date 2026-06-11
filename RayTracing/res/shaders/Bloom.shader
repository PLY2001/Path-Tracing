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
	float alpha = 1.0f;
	if (texColor.r+texColor.g+texColor.b<2.4f)
		alpha = 0.0f;
	else
		alpha = 0.5f;
	color = vec4(texColor.rgb,alpha);
}
