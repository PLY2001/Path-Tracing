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
uniform sampler2D depthTexture;
uniform sampler2D NormalTexture;
uniform float WinWidth;
uniform float WinHeight;

//vec2 offset[25]=vec2[](vec2(-1,1),vec2(0,1),vec2(1,1),vec2(-1,0),vec2(0,0),vec2(1,0),vec2(-1,-1),vec2(0,-1),vec2(1,-1),vec2(-2,2),vec2(-1,2),vec2(0,2),vec2(1,2),vec2(2,2),vec2(-2,1),vec2(2,1),vec2(-2,0),vec2(2,0),vec2(-2,-1),vec2(2,-1),vec2(-2,-2),vec2(-1,-2),vec2(0,-2),vec2(1,-2),vec2(2,-2));

vec3 toneMapping(in vec3 c, float limit) 
{
    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;
    return c * 1.0 / (1.0 + luminance / limit);
}

vec3 ACESToneMapping(vec3 color, float adapted_lum)
{
	float A = 2.51f;
	float B = 0.03f;
	float C = 2.43f;
	float D = 0.59f;
	float E = 0.14f;

	color *= adapted_lum;
	return (color * (A * color + B)) / (color * (C * color + D) + E);
}

void main() 
{
	vec3 finalColor = vec3(0.0);
	//¾ùÖµÄ£ºý
	float w = 0;
	float totalw = 0;
	float PosDistance = 0;
	float ColorDistance = 0;
	float NormalDistance = 0;
	float DepthDistance = 0;

	float PDvariance = 4.0/3.0;
	float CDvariance = 1.732*1.732/12.0/100.0;
	float NDvariance = 1.0/12.0;
	float DDvariance = 1.0/12.0 / 100000;

	vec3 thisColor = vec3(0.0);
	vec2 thisTexcoords = vec2(0.0);
	//float bias = 1.0;
	vec4 texColor = vec4(0.0);
	if(texture(depthTexture, Texcoords).r < 1.0)
	{
		int passes = 5;
		for (int pass = 0; pass < passes; pass++)
		{
			for (int filterX = -2; filterX <= 2; filterX++) 
			{
				for (int filterY = -2; filterY <= 2; filterY++)
				{

					float m = pow(2, pass) * filterX;
					float n = pow(2, pass) * filterY;
			
					thisTexcoords = vec2(Texcoords.x + m / WinWidth, Texcoords.y + n / WinHeight);//vec2(Texcoords.x + offset[i].x / WinWidth * bias, Texcoords.y + offset[i].y / WinHeight * bias);
					thisColor = texture(screenTexture, thisTexcoords).rgb;
					PosDistance = 0;//distance(vec2(m,n),vec2(0.0));
					ColorDistance = distance(thisColor,texture(screenTexture, Texcoords).rgb);
					NormalDistance = dot(texture(NormalTexture, thisTexcoords).rgb*2.0 - vec3(1.0),texture(NormalTexture, Texcoords).rgb*2.0 - vec3(1.0))*0.5 - 0.5;
					DepthDistance = texture(depthTexture, thisTexcoords).r - texture(depthTexture, Texcoords).r;

					

					w = exp(0.5*(-PosDistance*PosDistance / PDvariance - ColorDistance*ColorDistance / CDvariance - NormalDistance*NormalDistance / NDvariance - DepthDistance*DepthDistance / DDvariance));
					finalColor = finalColor + thisColor*w;
					totalw = totalw + w;
				}
			}
		}

		texColor=vec4(finalColor / totalw,1.0);
	}
	else
	{
		texColor=texture(screenTexture,Texcoords);
	}

	

	//texColor=texture(NormalTexture,Texcoords);
	
	texColor = vec4(ACESToneMapping(texColor.rgb, 1.5),1.0f);
	
	float gamma=2.2;
	
	texColor = vec4(pow(texColor.rgb,vec3(1.0/gamma)),1.0f);


	

    color =texColor;
}