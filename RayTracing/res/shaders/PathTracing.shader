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


#define SIZE_TRIANGLE   12
#define SIZE_BVHNODE    4

#define PI  3.14159265
#define INF  100000.0


uniform samplerBuffer triangles;
uniform int nTriangles;
uniform samplerBuffer nodes;
uniform int nNodes;
uniform uint frameCounter;
uniform float WinWidth;
uniform float WinHeight;
uniform sampler2D lastFrame;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 CameraPos;
uniform sampler2D hdrMap;
uniform sampler2D hdrCache;
uniform int hdrResolution;

// 物体表面材质定义
struct Material
{
	vec3 emissive;  // 作为光源时的发光颜色
	vec3 baseColor;
	float subsurface;
	float metallic;
	float specular;
	float specularTint;
	float roughness;
	float anisotropic;
	float sheen;
	float sheenTint;
	float clearcoat;
	float clearcoatGloss;
	float IOR;
	float transmission;
};

// 三角形定义
struct Triangle
{
	vec3 p1, p2, p3;    // 顶点坐标
	vec3 n1, n2, n3;    // 顶点法线
	Material material;  // 材质
};

// 光线
struct Ray 
{
    vec3 startPoint;
    vec3 direction;
};

// 光线求交结果
struct HitResult 
{
    bool isHit;             // 是否命中
    bool isInside;          // 是否从内部命中
    float distance;         // 与交点的距离
    vec3 hitPoint;          // 光线命中点
    vec3 normal;            // 命中点法线
    vec3 viewDir;           // 击中该点的光线的方向
    Material material;      // 命中点的表面材质
};

// BVH 树节点
struct BVHNode {
    int left, right;    // 左右子树索引
    int n, index;       // 叶子节点信息               
    vec3 AA, BB;        // 碰撞盒
};


Triangle getTriangle(int i)
{
    int offset = i * SIZE_TRIANGLE;
    Triangle t;

    // 顶点坐标
    t.p1 = texelFetch(triangles, offset + 0).xyz;
    t.p2 = texelFetch(triangles, offset + 1).xyz;
    t.p3 = texelFetch(triangles, offset + 2).xyz;
    // 法线
    t.n1 = texelFetch(triangles, offset + 3).xyz;
    t.n2 = texelFetch(triangles, offset + 4).xyz;
    t.n3 = texelFetch(triangles, offset + 5).xyz;

    return t;
}

// 获取第 i 下标的三角形的材质
Material getMaterial(int i)
{
    Material m;

    int offset = i * SIZE_TRIANGLE;
    vec3 param1 = texelFetch(triangles, offset + 8).xyz;
    vec3 param2 = texelFetch(triangles, offset + 9).xyz;
    vec3 param3 = texelFetch(triangles, offset + 10).xyz;
    vec3 param4 = texelFetch(triangles, offset + 11).xyz;
    
    m.emissive = texelFetch(triangles, offset + 6).xyz;
    m.baseColor = texelFetch(triangles, offset + 7).xyz;
    m.subsurface = param1.x;
    m.metallic = param1.y;
    m.specular = param1.z;
    m.specularTint = param2.x;
    m.roughness = param2.y;
    m.anisotropic = param2.z;
    m.sheen = param3.x;
    m.sheenTint = param3.y;
    m.clearcoat = param3.z;
    m.clearcoatGloss = param4.x;
    m.IOR = param4.y;
    m.transmission = param4.z;

    return m;
}

// 获取第 i 下标的 BVHNode 对象
BVHNode getBVHNode(int i) 
{
    BVHNode node;

    // 左右子树
    int offset = i * SIZE_BVHNODE;
    ivec3 childs = ivec3(texelFetch(nodes, offset + 0).xyz);
    ivec3 leafInfo = ivec3(texelFetch(nodes, offset + 1).xyz);
    node.left = int(childs.x);
    node.right = int(childs.y);
    node.n = int(leafInfo.x);
    node.index = int(leafInfo.y);

    // 包围盒
    node.AA = texelFetch(nodes, offset + 2).xyz;
    node.BB = texelFetch(nodes, offset + 3).xyz;

    return node;
}

// 光线和三角形求交 
HitResult hitTriangle(Triangle triangle, Ray ray)
{


	HitResult res;
    res.distance = INF;
    res.isHit = false;
    res.isInside = false;

	vec3 p1 = triangle.p1;
    vec3 p2 = triangle.p2;
    vec3 p3 = triangle.p3;

    vec3 O = ray.startPoint;    // 射线起点
    vec3 D = ray.direction;     // 射线方向
    vec3 N = normalize(cross(p2-p1, p3-p1));    // 法向量

	// 从三角形背后（模型内部）击中
    if (dot(N, D) > 0.0f) 
	{
        N = -N;   
        res.isInside = true;
    }

	// 如果视线和三角形平行
    if (abs(dot(N, D)) < 0.00001f) return res;

	//求交计算
	vec3 E1 = p2 - p1;
	vec3 E2 = p3 - p1;
	vec3 S = O - p1;
	vec3 S1 = cross(D, E2);
	vec3 S2 = cross(S, E1);

	// 距离
	float t = dot(S2, E2) / dot(S1, E1);
	if (t < 0.0005f) return res;    // 如果三角形在相机背面
	
	float b1 = dot(S1, S) / dot(S1, E1);
	float b2 = dot(S2, D) / dot(S1, E1);
	
	// 判断交点是否在三角形中
	if (b1 < 0 || b2 < 0 || (1 - b1 - b2) < 0) return res;

	
	// 得到交点
	vec3 P = O + D * t;

	// 装填返回结果
	res.isHit = true;
	res.distance = t;
	res.hitPoint = P;
	res.material = triangle.material;
	res.viewDir = D;

	vec3 Nsmooth = (1 - b1 - b2) * triangle.n1 + b1 * triangle.n2 + b2 * triangle.n3;
    Nsmooth = normalize(Nsmooth);
    res.normal = (res.isInside) ? (-Nsmooth) : (Nsmooth);

    return res;
}



// 暴力遍历数组下标范围 [l, r] 求最近交点
HitResult hitArray(Ray ray, int l, int r)
{
    HitResult res;
    res.isHit = false;
    res.distance = INF;
    for(int i=l; i<=r; i++) {
        Triangle triangle = getTriangle(i);
        HitResult r = hitTriangle(triangle, ray);
        if(r.isHit && r.distance<res.distance) {
            res = r;
            res.material = getMaterial(i);
        }
    }
    return res;
}

// 和 aabb 盒子求交，没有交点则返回 -1
float hitAABB(Ray r, vec3 AA, vec3 BB) 
{
    vec3 invdir = 1.0 / r.direction;

    vec3 f = (BB - r.startPoint) * invdir;
    vec3 n = (AA - r.startPoint) * invdir;

    vec3 tmax = max(f, n);
    vec3 tmin = min(f, n);

    float t1 = min(tmax.x, min(tmax.y, tmax.z));
    float t0 = max(tmin.x, max(tmin.y, tmin.z));

    return (t1 >= t0) ? ((t0 > 0.0) ? (t0) : (t1)) : (-1);
}


// 遍历 BVH 求交
HitResult hitBVH(Ray ray) {
    HitResult res;
    res.isHit = false;
    res.distance = INF;

    // 栈
    int stack[256];
    int sp = 0;

    stack[sp++] = 1;
    while(sp>0) {
        int top = stack[--sp];
        BVHNode node = getBVHNode(top);
        
        // 是叶子节点，遍历三角形，求最近交点
        if(node.n>0) {
            int L = node.index;
            int R = node.index + node.n - 1;
            HitResult r = hitArray(ray, L, R);
            if(r.isHit && r.distance<res.distance) res = r;


            continue;
        }
        
        // 和左右盒子 AABB 求交
        float d1 = INF; // 左盒子距离
        float d2 = INF; // 右盒子距离
        if(node.left>0) {
            BVHNode leftNode = getBVHNode(node.left);
            d1 = hitAABB(ray, leftNode.AA, leftNode.BB);
        }
        if(node.right>0) {
            BVHNode rightNode = getBVHNode(node.right);
            d2 = hitAABB(ray, rightNode.AA, rightNode.BB);
        }

        // 在最近的盒子中搜索
        if(d1>0 && d2>0) {
            if(d1<d2) { // d1<d2, 左边先
                stack[sp++] = node.right;
                stack[sp++] = node.left;
            } else {    // d2<d1, 右边先
                stack[sp++] = node.left;
                stack[sp++] = node.right;
            }
        } else if(d1>0) {   // 仅命中左边
            stack[sp++] = node.left;
        } else if(d2>0) {   // 仅命中右边
            stack[sp++] = node.right;
        }
    }

    return res;
}

//0~1随机数
uint seed = uint(
    uint(gl_FragCoord.x)  * uint(1973) + 
    uint(gl_FragCoord.y) * uint(9277) + 
    uint(frameCounter) * uint(26699)) | uint(1);

uint wang_hash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}
 
float rand() {
    return float(wang_hash(seed)) / 4294967296.0;
}


// 半球均匀采样
//vec3 SampleHemisphere() {
//    float z = rand();
//    float r = max(0, sqrt(1.0 - z*z));
//    float phi = 2.0 * PI * rand();
//    return vec3(r * cos(phi), r * sin(phi), z);
//}

// 将向量 v 投影到 N 的法向半球
vec3 toNormalHemisphere(vec3 v, vec3 N) {
    vec3 helper = vec3(1, 0, 0);
    if(abs(N.x)>0.999) helper = vec3(0, 0, 1);
    vec3 tangent = normalize(cross(N, helper));
    vec3 bitangent = normalize(cross(N, tangent));
    return v.x * tangent + v.y * bitangent + v.z * N;
}

// 余弦加权的法向半球采样，代替半球均匀采样
vec3 SampleCosineHemisphere(float xi_1, float xi_2, vec3 N) {
    // 均匀采样 xy 圆盘然后投影到 z 半球
    float r = sqrt(xi_1);
    float theta = xi_2 * 2.0 * PI;
    float x = r * cos(theta);
    float y = r * sin(theta);
    float z = sqrt(1.0 - x*x - y*y);

    // 从 z 半球投影到法向半球
    vec3 L = toNormalHemisphere(vec3(x, y, z), N);
    return L;
}


// 将三维向量 v 转为 HDR map 的纹理坐标 uv
vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv /= vec2(2.0 * PI, PI);
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

// 获取 HDR 环境颜色
vec3 sampleHdr(vec3 v) {
    vec2 uv = SampleSphericalMap(normalize(v));
    vec3 hdrcolor = texture(hdrMap, uv).rgb;
    //color = min(color, vec3(10));
    return hdrcolor;
}

// 采样预计算的 HDR cache
vec3 sampleHdrCache(float xi_1, float xi_2) {
    vec2 xy = texture(hdrCache, vec2(xi_1, xi_2)).rg; // x, y
    xy.y = 1.0 - xy.y; // flip y

    // 获取角度
    float phi = 2.0 * PI * (xy.x - 0.5);    // [-pi ~ pi]
    float theta = PI * (xy.y - 0.5);        // [-pi/2 ~ pi/2]   

    // 球坐标计算方向
    vec3 L = vec3(cos(theta)*cos(phi), sin(theta), cos(theta)*sin(phi));

    return L;
}


// 输入光线方向 L 获取 HDR 在该位置的概率密度
// hdr 分辨率为 4096 x 2048 --> hdrResolution = 4096
float hdrPdf(vec3 L, int hdrResolution) {
    vec2 uv = SampleSphericalMap(normalize(L));   // 方向向量转 uv 纹理坐标

    float pdf = texture(hdrCache, uv).b;      // 采样概率密度
    float theta = PI * (0.5 - uv.y);            // theta 范围 [-pi/2 ~ pi/2]
    float sin_theta = max(sin(theta), 1e-10);

    // 球坐标和图片积分域的转换系数
    float p_convert = float(hdrResolution*hdrResolution/2.0) / (2.0 * PI * PI * sin_theta);  
    
    return pdf * p_convert;
}

//Disney风格BRDF模型F项
float SchlickFresnel(float u) {
    float m = clamp(1-u, 0, 1);
    float m2 = m*m;
    return m2*m2*m; // pow(m,5)
}

//Disney风格BRDF模型D项（gamma=1情况）
float GTR1(float NdotH, float a) {
    if (a >= 1) return 1/PI;
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return (a2-1) / (PI*log(a2)*t);
}

//Disney风格BRDF模型D项（gamma=2情况）
float GTR2(float NdotH, float a) {
    float a2 = a*a;
    float t = 1 + (a2-1)*NdotH*NdotH;
    return a2 / (PI * t*t);
}

// GTR1 重要性采样
vec3 SampleGTR1(float xi_1, float xi_2, vec3 V, vec3 N, float alpha) {
    float phi_h = 2.0 * PI * xi_1;
    float sin_phi_h = sin(phi_h);
    float cos_phi_h = cos(phi_h);
    
    float cos_theta_h = sqrt((1.0-pow(alpha*alpha, 1.0-xi_2))/(1.0-alpha*alpha));
    float sin_theta_h = sqrt(max(0.0, 1.0 - cos_theta_h * cos_theta_h));

    // 采样 "微平面" 的法向量 作为镜面反射的半角向量 h 
    vec3 H = vec3(sin_theta_h*cos_phi_h, sin_theta_h*sin_phi_h, cos_theta_h);
    H = toNormalHemisphere(H, N);   // 投影到真正的法向半球

    // 根据 "微法线" 计算反射光方向
    vec3 L = reflect(-V, H);

    return L;
}

// GTR2 重要性采样
vec3 SampleGTR2(float xi_1, float xi_2, vec3 V, vec3 N, float alpha) {
    
    float phi_h = 2.0 * PI * xi_1;
    float sin_phi_h = sin(phi_h);
    float cos_phi_h = cos(phi_h);

    float cos_theta_h = sqrt((1.0-xi_2)/(1.0+(alpha*alpha-1.0)*xi_2));
    float sin_theta_h = sqrt(max(0.0, 1.0 - cos_theta_h * cos_theta_h));

    // 采样 "微平面" 的法向量 作为镜面反射的半角向量 h 
    vec3 H = vec3(sin_theta_h*cos_phi_h, sin_theta_h*sin_phi_h, cos_theta_h);
    H = toNormalHemisphere(H, N);   // 投影到真正的法向半球

    // 根据 "微法线" 计算反射光方向
    vec3 L = reflect(-V, H);

    return L;
}

//Disney风格BRDF模型G项
float smithG_GGX(float NdotV, float alphaG) {
    float a = alphaG*alphaG;
    float b = NdotV*NdotV;
    return 1 / (NdotV + sqrt(a + b - a*b));
}

// 按照辐射度分布分别采样三种 BRDF
vec3 SampleBRDF(float xi_1, float xi_2, float xi_3, vec3 V, vec3 N, in Material material) {
    float alpha_GTR1 = mix(0.1, 0.001, material.clearcoatGloss);
    float alpha_GTR2 = max(0.001, material.roughness*material.roughness);
    
    // 辐射度统计
    float r_diffuse = (1.0 - material.metallic);
    float r_specular = 1.0;
    float r_clearcoat = 0.25 * material.clearcoat;
    float r_sum = r_diffuse + r_specular + r_clearcoat;

    // 根据辐射度计算概率
    float p_diffuse = r_diffuse / r_sum;
    float p_specular = r_specular / r_sum;
    float p_clearcoat = r_clearcoat / r_sum;

    // 按照概率采样
    float rd = xi_3;

    // 漫反射
    if(rd <= p_diffuse) {
        return SampleCosineHemisphere(xi_1, xi_2, N);
    } 
    // 镜面反射
    else if(p_diffuse < rd && rd <= p_diffuse + p_specular) {    
        return SampleGTR2(xi_1, xi_2, V, N, alpha_GTR2);
    } 
    // 清漆
    else if(p_diffuse + p_specular < rd) {
        return SampleGTR1(xi_1, xi_2, V, N, alpha_GTR1);
    }
    return vec3(0, 1, 0);
}

// 获取 BRDF 在 L 方向上的概率密度
float BRDF_Pdf(vec3 V, vec3 N, vec3 L, in Material material) {
    float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    if(NdotL < 0 || NdotV < 0) return 0;

    vec3 H = normalize(L + V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);
     
    // 镜面反射 -- 各向同性
    float alpha = max(0.001, material.roughness*material.roughness);
    float Ds = GTR2(NdotH, alpha); 
    float Dr = GTR1(NdotH, mix(0.1, 0.001, material.clearcoatGloss));   // 清漆

    // 分别计算三种 BRDF 的概率密度
    float pdf_diffuse = NdotL / PI;
    float pdf_specular = Ds * NdotH / (4.0 * dot(L, H));
    float pdf_clearcoat = Dr * NdotH / (4.0 * dot(L, H));

    // 辐射度统计
    float r_diffuse = (1.0 - material.metallic);
    float r_specular = 1.0;
    float r_clearcoat = 0.25 * material.clearcoat;
    float r_sum = r_diffuse + r_specular + r_clearcoat;

    // 根据辐射度计算选择某种采样方式的概率
    float p_diffuse = r_diffuse / r_sum;
    float p_specular = r_specular / r_sum;
    float p_clearcoat = r_clearcoat / r_sum;

    // 根据概率混合 pdf
    float pdf = p_diffuse   * pdf_diffuse 
              + p_specular  * pdf_specular
              + p_clearcoat * pdf_clearcoat;

    pdf = max(1e-10, pdf);
    return pdf;
}

vec3 BRDF_Evaluate(vec3 V, vec3 N, vec3 L, in Material material) {
	
	float NdotL = dot(N, L);
    float NdotV = dot(N, V);
    if(NdotL < 0 || NdotV < 0) return vec3(0);

    vec3 H = normalize(L + V);
    float NdotH = dot(N, H);
    float LdotH = dot(L, H);

    // 各种颜色
    vec3 Cdlin = material.baseColor;
    float Cdlum = 0.3 * Cdlin.r + 0.6 * Cdlin.g  + 0.1 * Cdlin.b;
    vec3 Ctint = (Cdlum > 0) ? (Cdlin/Cdlum) : (vec3(1));   
    vec3 Cspec = material.specular * mix(vec3(1), Ctint, material.specularTint);
    vec3 Cspec0 = mix(0.08*Cspec, Cdlin, material.metallic); // 0° 镜面反射颜色
    vec3 Csheen = mix(vec3(1), Ctint, material.sheenTint);   // 织物颜色

    // 漫反射
    float Fd90 = 0.5 + 2.0 * LdotH * LdotH * material.roughness;
    float FL = SchlickFresnel(NdotL);
    float FV = SchlickFresnel(NdotV);
    float Fd = mix(1.0, Fd90, FL) * mix(1.0, Fd90, FV);

    // 次表面散射
    float Fss90 = LdotH * LdotH * material.roughness;
    float Fss = mix(1.0, Fss90, FL) * mix(1.0, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0 / (NdotL + NdotV) - 0.5) + 0.5);
     
    // 镜面反射 -- 各向同性
    float alpha = max(0.001, material.roughness*material.roughness);
    float Ds = GTR2(NdotH, alpha);
    float FH = SchlickFresnel(LdotH);
    vec3 Fs = mix(Cspec0, vec3(1), FH);
    float Gs = smithG_GGX(NdotL, material.roughness);
    Gs *= smithG_GGX(NdotV, material.roughness);

    // 清漆
    float Dr = GTR1(NdotH, mix(0.1, 0.001, material.clearcoatGloss));
    float Fr = mix(0.04, 1.0, FH);
    float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);

    // sheen
    vec3 Fsheen = FH * material.sheen * Csheen;
    
    vec3 diffuse = (1.0/PI) * mix(Fd, ss, material.subsurface) * Cdlin + Fsheen;
    vec3 specular = Gs * Fs * Ds;
    vec3 clearcoat = vec3(0.25 * Gr * Fr * Dr * material.clearcoat);

    return diffuse * (1.0 - material.metallic) + specular + clearcoat;//Disney风格采用diffuse+微表面模型specular的形式。加diffuse是为了抵消反射次数少导致模型偏暗的问题。
}


float misMixWeight(float a, float b) {
    float t = a * a;
    return t / (b*b + t);
}

// 路径追踪
vec3 pathTracing(HitResult hit, int maxBounce) {

    vec3 Lo = vec3(0);      // 最终的颜色
    vec3 history = vec3(1); // 递归积累的颜色

    for(int bounce=0; bounce<maxBounce; bounce++) {

		vec3 V = -hit.viewDir;
        vec3 N = hit.normal;
        //vec3 L = toNormalHemisphere(SampleHemisphere(), hit.normal);    // 随机出射方向 wi
		//vec3 L = mix(reflect(hit.viewDir, hit.normal),toNormalHemisphere(SampleHemisphere(), hit.normal),hit.material.roughness);
        //float pdf = 1.0 / (2.0 * PI);                                   // 半球均匀采样概率密度
		float xi_1 = rand();
		float xi_2 = rand();
		float alpha = max(0.001, hit.material.roughness*hit.material.roughness);
		//vec3 L = SampleCosineHemisphere(xi_1, xi_2, N);
		vec3 L = SampleGTR2(xi_1,xi_2,V, N, alpha);
		vec3 H = normalize(L + V);
		float pdf = dot(N,H) / PI;
        float cosine_o = max(0, dot(V, N));                             // 入射光和法线夹角余弦
        float cosine_i = max(0, dot(L, N));                             // 出射光和法线夹角余弦
        
		vec3 f_r = BRDF_Evaluate(V, N, L, hit.material);
		//vec3 f_r = hit.material.baseColor / PI;                         // 漫反射 BRDF

        // 漫反射: 随机发射光线
        Ray randomRay;
        randomRay.startPoint = hit.hitPoint;
        randomRay.direction = L;
        HitResult newHit = hitBVH(randomRay);
		

        // 未命中
		if(!newHit.isHit)
		{
			vec3 skyColor = sampleHdr(randomRay.direction);
			Lo += history * skyColor * f_r * cosine_i / pdf;
			break;
		}
        
        // 命中光源积累颜色
        vec3 Le = newHit.material.emissive;
        Lo += history * Le * f_r * cosine_i / pdf;
        

        // 递归(步进)
        hit = newHit;
        history *= f_r * cosine_i / pdf;  // 累积颜色
    }
    
	//Lo.r = clamp(Lo.r,0.0,1.0);
	//Lo.g = clamp(Lo.g,0.0,1.0);
	//Lo.b = clamp(Lo.b,0.0,1.0);

    return Lo;
}

// 路径追踪 -- 重要性采样版本
vec3 pathTracingImportanceSampling(HitResult hit, int maxBounce) {

    vec3 Lo = vec3(0);      // 最终的颜色
    vec3 history = vec3(1); // 递归积累的颜色

    for(int bounce=0; bounce<maxBounce; bounce++) {
        vec3 V = -hit.viewDir;
        vec3 N = hit.normal;       
     
		// HDR 环境贴图重要性采样    
		Ray hdrTestRay;
		hdrTestRay.startPoint = hit.hitPoint;
		hdrTestRay.direction = sampleHdrCache(rand(), rand());//得到指向任一光源处的方向
		
		// 进行一次求交测试 判断是否有遮挡
		if(dot(N, hdrTestRay.direction) > 0.0) { // 如果采样方向背向点 p 则放弃测试, 因为 N dot L < 0            
		    HitResult hdrHit = hitBVH(hdrTestRay);
		    
		    // 天空光仅在没有遮挡的情况下积累亮度
		    if(!hdrHit.isHit) {
		        vec3 L = hdrTestRay.direction;
		        vec3 color = sampleHdr(L);
		        float pdf_light = hdrPdf(L, hdrResolution);
				
		        vec3 f_r = BRDF_Evaluate(V, N, L, hit.material);
		        float pdf_brdf = BRDF_Pdf(V, N, L, hit.material);
            	
                // 多重重要性采样
				//float mis_weight = misMixWeight(pdf_light, pdf_brdf);
				//Lo += mis_weight * history * color * f_r * dot(N, L) / pdf_light;
				Lo += history * color * f_r * dot(N, L)/100000.0;	// 累计亮度，上一行似乎有溢出问题，只能这样近似了//1000

				


		    }
		}


        // 获取 3 个随机数        
        float xi_1 = rand();
		float xi_2 = rand();
		float xi_3 = rand();

        // 采样 BRDF 得到一个方向 L
        vec3 L = SampleBRDF(xi_1, xi_2, xi_3, V, N, hit.material); 
        float NdotL = dot(N, L);
        if(NdotL <= 0.0) break;

        // 发射光线
        Ray randomRay;
        randomRay.startPoint = hit.hitPoint;
        randomRay.direction = L;
        HitResult newHit = hitBVH(randomRay);

        // 获取 L 方向上的 BRDF 值和概率密度
        vec3 f_r = BRDF_Evaluate(V, N, L, hit.material);
        float pdf_brdf = BRDF_Pdf(V, N, L, hit.material);
        if(pdf_brdf <= 0.0) break;

        // 未命中        
        if(!newHit.isHit) {
            vec3 color = sampleHdr(L);
            Lo += history * color * f_r * NdotL / pdf_brdf;
            break;

			//vec3 color = sampleHdr(L);
			//float pdf_light = hdrPdf(L, hdrResolution);            
			//
			//// 多重重要性采样
			//float mis_weight = misMixWeight(pdf_brdf, pdf_light);  
			//Lo += mis_weight * history * color * f_r * NdotL / pdf_brdf;
			//
			//break;
        }
        
        // 命中光源积累颜色
        vec3 Le = newHit.material.emissive;
        Lo += history * Le * f_r * NdotL / pdf_brdf;             

        // 递归(步进)
        hit = newHit;
        history *= f_r * NdotL / pdf_brdf;   // 累积颜色
    }
    

    return Lo;
}




void main() 
{


	
	
	Ray ray;
	//ray.startPoint = vec3(0, 0, 4);
	//vec3 dir = vec3(gl_FragCoord.x/WinWidth*2.0f-1.0f,gl_FragCoord.y/WinHeight*2.0f-1.0f, 1.1f) - ray.startPoint;
	//ray.direction = normalize(dir);

	ray.startPoint = CameraPos;
	ray.direction = vec3(gl_FragCoord.x/WinWidth*2.0f-1.0f,gl_FragCoord.y/WinHeight*2.0f-1.0f,0.0f);
	// MSAA
	ray.direction.x += (rand() - 0.5f) / WinWidth;
	ray.direction.y += (rand() - 0.5f) / WinHeight;

	vec4 worldDirection = inverse(u_view)*inverse(u_projection)*vec4(ray.direction,1.0f);
	worldDirection = worldDirection / worldDirection.w;
	//worldDirection = worldDirection*;
	ray.direction = normalize(worldDirection.xyz-ray.startPoint);
	
	

	vec3 fragColor = vec3(0);

	// primary hit
	HitResult firstHit = hitBVH(ray);

	if(!firstHit.isHit)
	{
		fragColor = sampleHdr(ray.direction);
	}
	else
	{
		vec3 Le = firstHit.material.emissive;
		int maxBounce = 2;
		//vec3 Li = pathTracing(firstHit, maxBounce);
		vec3 Li = pathTracingImportanceSampling(firstHit, maxBounce);
		fragColor = Le + Li;
	}

	
	

	color = vec4(fragColor, 1.0);
}