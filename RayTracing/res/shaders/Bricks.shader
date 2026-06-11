#shader vertex
#version 330 core 

layout(location=0) in vec3 position; 
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texcoord; //贴图坐标
layout(location=3) in vec3 tangent; //切线


out VS_OUT{
	vec2 v_texcoord;//传递（vary）给片元着色器的变量
	vec4 v_TangentPosition;
	vec4 v_TangentLightPos;
	vec4 v_TangentCameraPos;
}vs_out;


layout(std140) uniform Matrices
{
	mat4 u_view;
	mat4 u_projection;
};

uniform vec4 u_LightPosition;
uniform vec4 u_CameraPosition;
uniform mat4 model;

void main() 
{ 
	
	vec3 T = normalize(vec3(model*vec4(tangent,0.0f)));
	vec3 N = normalize(vec3(model*vec4(normal,0.0f)));
	//vec3 g = normalize(vec3(1.0f,0.0f,1.0f));
	T = normalize(T-dot(T,N)*N);//保证TBN矩阵是正交化
	vec3 B = cross(N,T);
	mat3 TBN = transpose(mat3(T,B,N));

	vs_out.v_texcoord = texcoord;
	vs_out.v_TangentPosition = vec4(TBN*(model*vec4(position,1.0f)).xyz,1.0f);
	vs_out.v_TangentLightPos = vec4(TBN*(u_LightPosition.xyz),1.0f);
	vs_out.v_TangentCameraPos = vec4(TBN*(u_CameraPosition.xyz),1.0f);
	
	gl_Position =u_projection*u_view*model*vec4(position,1.0f); 

}




#shader fragment
#version 330 core 


out vec4 color; 


uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_displacement;



in VS_OUT{
	vec2 v_texcoord;//传递（vary）给片元着色器的变量
	vec4 v_TangentPosition;
	vec4 v_TangentLightPos;
	vec4 v_TangentCameraPos;
}fs_in;


void main() 
{
	//求视差贴图的偏移后的纹理坐标
	vec3 CameraDir = normalize(fs_in.v_TangentCameraPos.xyz - fs_in.v_TangentPosition.xyz);//指向相机的方向
	
	const float numLayers = 10;//偏移次数
	float deltaDepth = 1.0/numLayers;//每次偏移增加的深度

	float currentDepth = 0.0;//当前点偏移的深度
	float lastDepth = currentDepth;//上一点偏移的深度

	vec2 p1 = CameraDir.xy;//偏移方向
	vec2 deltaTexcoords = p1 / numLayers*0.1f;//每次偏移的单位偏移坐标大小，越大则凹凸越明显

	vec2 currentTexcoords = fs_in.v_texcoord;//当前点的纹理坐标
	vec2 lastTexcoords = currentTexcoords;//上一点的纹理坐标

	float currentDepthMapValue = texture(texture_displacement,currentTexcoords).r;//当前点的深度贴图的实际深度
	float lastDepthMapValue = currentDepthMapValue;//上一点的深度贴图的实际深度

	while(currentDepthMapValue>currentDepth)//还未越过应当偏移的纹理坐标时
	{
		//给上一点各参量赋值
		lastTexcoords = currentTexcoords;
		lastDepthMapValue = currentDepthMapValue;
		lastDepth = currentDepth;

		currentTexcoords -= deltaTexcoords;//偏移当前点的纹理坐标（与指向相机的方向是相反的，所以是-）
		currentDepthMapValue = texture(texture_displacement,currentTexcoords).r;//当前点的深度贴图的实际深度
		currentDepth += deltaDepth;//当前点偏移的深度

		if(currentDepthMapValue<currentDepth)//刚刚越过应当偏移的纹理坐标时
		{
			vec2 backOffset =  deltaTexcoords*(currentDepth-currentDepthMapValue)/(lastDepthMapValue-lastDepth+currentDepth-currentDepthMapValue);//插值
			currentTexcoords = currentTexcoords + backOffset;//得到精确的偏移后的纹理坐标
		}
	}
	vec2 finalTexcoords = currentTexcoords;//偏移后的纹理坐标
	
	if(finalTexcoords.x>1.0||finalTexcoords.y>1.0||finalTexcoords.x<0.0||finalTexcoords.y<0.0)//如果偏移得超出了纹理坐标范围，就不渲染
		discard;

	vec3 LightDir = normalize(fs_in.v_TangentLightPos.xyz - fs_in.v_TangentPosition.xyz);//光源方向
	vec3 Normal= texture(texture_normal,finalTexcoords).xyz;
	Normal = Normal*2.0f - vec3(1.0f);
	//漫反射
	float diffuse = max(0.0f,dot(Normal,LightDir));
	vec3 diffuseColor=2.0f*texture(texture_diffuse,finalTexcoords).xyz*diffuse;
	//高光反射
	//vec3 CameraDir = normalize(fs_in.v_TangentCameraPos.xyz - fs_in.v_TangentPosition.xyz);
	vec3 ha= normalize(CameraDir+LightDir);
	float specular = pow(max(0.0f,dot(Normal,ha)),100.0f);
	vec3 specularColor =  vec3(2.0f)*specular;
	//环境光
	vec3 ambientColor = vec3(0.05f,0.05f,0.05f);
	

	
	color =vec4((ambientColor+diffuseColor+specularColor),1.0f);//*(1.0f-shadowColor) //texColor;//u_color;//vec4(0.2,0.7,0.3,1.0); 
}