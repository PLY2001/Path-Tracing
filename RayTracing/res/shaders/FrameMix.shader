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
uniform sampler2D screenTexture2;
uniform sampler2D depthTexture;
uniform sampler2D lastDepthTexture;
uniform sampler2D NormalTexture;
uniform float frameCounter;
uniform float lastFrameCounter;
uniform float lastFrameWeight;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_lastview;
uniform mat4 u_lastprojection;






void main() 
{
	// 和上一帧混合

	vec4 ScreenPos = vec4(Texcoords.x*2.0-1.0,Texcoords.y*2.0-1.0,texture(depthTexture,Texcoords).r,1.0);
	vec4 LastWorldPos = inverse(u_view)*inverse(u_projection)*ScreenPos;
	LastWorldPos = LastWorldPos/LastWorldPos.w;
	vec4 LastScreenPos = u_lastprojection*u_lastview*LastWorldPos;
	LastScreenPos = LastScreenPos/LastScreenPos.w;
	
	
	vec2 LastTexcoords = vec2(LastScreenPos.x*0.5+0.5,LastScreenPos.y*0.5+0.5);
	vec3 lastColor =vec3(0.0);
	vec3 newColor = vec3(0.0);
	vec3 thisColor =vec3(0.0);


	


	if(frameCounter > lastFrameWeight)
	{
		lastColor = texture(screenTexture2, Texcoords).rgb;
		newColor = texture(screenTexture, Texcoords).rgb;
		thisColor = mix(lastColor, newColor, 1.0/float(frameCounter+1.0)).rgb;
	}
	else
	{
		if(LastTexcoords.x>0&&LastTexcoords.x<1&&LastTexcoords.y>0&&LastTexcoords.y<1&&ScreenPos.z<1)
		{
			if(abs(LastScreenPos.z-texture(lastDepthTexture,LastTexcoords).r)<0.01)
			{
				lastColor = texture(screenTexture2, LastTexcoords).rgb;
				newColor = texture(screenTexture, Texcoords).rgb;
				//thisColor = mix(lastColor, newColor,0.1).rgb;
				thisColor = mix(lastColor, newColor, 1.0/float(lastFrameCounter+1.0)).rgb;
			}
			else
			{
				lastColor = texture(screenTexture2, Texcoords).rgb;
				newColor = texture(screenTexture, Texcoords).rgb;
				thisColor = mix(lastColor, newColor, 1.0/float(frameCounter+1.0)).rgb;
			}
		}
		else
		{
			lastColor = texture(screenTexture2, Texcoords).rgb;
			newColor = texture(screenTexture, Texcoords).rgb;
			thisColor = mix(lastColor, newColor, 1.0/float(frameCounter+1.0)).rgb;
		}
	}
	
	

	color = vec4(thisColor,1.0f);
	//color = vec4(vec3(texture(NormalTexture, Texcoords).rgb),1.0);
	
}