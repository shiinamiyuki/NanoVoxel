const char *computeShaderSource = R"(#line 0
layout(local_size_x = 16, local_size_y = 16,local_size_z = 1) in;
layout(binding = 0) uniform sampler3D world;
layout(binding = 1, rgba32f)  uniform image2D accumlatedImage;
layout(binding = 2, rgba32f)  uniform image2D seeds;
layout(binding = 3, rgba32f)  writeonly uniform image2D composedImage;
uniform vec2 iResolution;
uniform mat4 cameraOrigin;
uniform mat4 cameraDirection;
uniform ivec3 worldDimension;
uniform int iTime;
uniform vec3 sunPos;
uniform uint options;

#define ENABLE_ATMOSPHERE_SCATTERING 0x1

struct Material {
    vec3 emission;
    vec3 baseColor;
    float roughness;
    float metalness;
    float specular;
};
#define MATERIAL_COUNT 256
layout(std430, binding = 4) readonly buffer Materials{
    vec4 MaterialEmission[MATERIAL_COUNT];
    vec4 MaterialBaseColor[MATERIAL_COUNT];
    float MaterialRoughness[MATERIAL_COUNT];
    float MaterialBetalness[MATERIAL_COUNT];
    float MaterialSpecular[MATERIAL_COUNT];
    float MaterialEmissionStrength[MATERIAL_COUNT];
};


const float M_PI = 3.1415926535;
float maxComp(vec3 o){
    return max(max(o.x,o.y),o.z);
}
float minComp(vec3 o){
	return min(min(o.x,o.y),o.z);
}
bool fleq(float x, float y){
	return abs(x - y) < 0.001;
}
const float RayBias = 1e-3f;
float intersectBox(vec3 o, vec3 d, vec3 p1, vec3 p2, out vec3 n){
	vec3 t0 = (p1 - o)/d;
    vec3 t1 = (p2 - o)/d;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    float t = maxComp(tmin);
    if(t < minComp(tmax)){
    	t = max(t, 0.0);
        if(t > minComp(tmax)){
        	return -1.0;
        }
        vec3 p = o + t *d;
        if(fleq(p1.x,p.x)){
        	n = vec3(-1,0,0);
        }else if(fleq(p2.x,p.x)){
        	n = vec3(1,0,0);
        }else if(fleq(p1.y,p.y)){
           n = vec3(0,-1,0);
        }else if(fleq(p2.y,p.y)){
        	n = vec3(0,1,0);
        }else if(fleq(p1.z,p.z)){
           n = vec3(0,0,-1);
        }else {
        	n = vec3(0,0,1);
        }
        return t;
    }
    return -1.0;
}
struct Intersection{
    float t;
    vec3 n;
    vec3 p;
    Material mat;
};
bool insideWorld(vec3 p){
    return all(lessThan(p, vec3(worldDimension)+vec3(1))) && all(greaterThanEqual(p, vec3(-2)));
}
int map(vec3 p){
    return int(texelFetch(world, ivec3(p), 0).r * 255.0);
}


bool intersect1(vec3 ro, vec3 rd, out Intersection isct)
{
    vec3 n;
    float distance = intersectBox(ro, rd, vec3(-1.5), vec3(worldDimension)+vec3(0.5), n);
    if(distance < 0.0){
        return false;
    }

    const int maxSteps = 100;
    ro += distance * rd;
    vec3 p0 = ro;
	vec3 p = floor(p0);
	vec3 stp = sign(rd);
	vec3 invd =clamp(vec3(1, 1, 1) / rd, vec3(-1e10, -1e10, -1e10),vec3(1e10,1e10,1e10));
	vec3 tMax = abs((p + max(stp, vec3(0,0,0)) - p0) * invd);
	vec3 delta = abs(invd);
    isct.n = n;
	vec3 mask = vec3(0, 0, 0);
	float t = 0;
    int maxIter = int(dot(worldDimension, ivec3(1)));
	for (int i = 0; i < maxIter; ++i) {
		if (!insideWorld(p)) {
			break;
		}
		int mat = map(p);

		if (mat > 0 && t >= RayBias) {
			isct.p = p0 + rd* t;
			isct.t = distance + t;
			isct.n = -sign(rd) * mask;
            isct.mat.baseColor = MaterialBaseColor[mat].rgb;
            isct.mat.emission = MaterialEmission[mat].rgb *  MaterialEmissionStrength[mat];
			return true;
		}
		if (tMax.x < tMax.y) {
			if (tMax.x < tMax.z) {
				p.x += stp.x;
				t = tMax.x;
				tMax.x += delta.x;
				mask = vec3(1, 0, 0);
			}
			else {
				p.z += stp.z; t = tMax.z;
				tMax.z += delta.z;

				mask = vec3(0, 0, 1);
			}
		}
		else {
			if (tMax.y < tMax.z) {
				p.y += stp.y; t = tMax.y;
				tMax.y += delta.y;

				mask = vec3(0, 1, 0);
			}
			else {
				p.z += stp.z; t = tMax.z;
				tMax.z += delta.z;

				mask = vec3(0, 0, 1);
			}
		}
	}
    return false;
}

bool intersect(vec3 ro, vec3 rd, out Intersection isct){
     float t = ro.y / -rd.y;
    if(intersect1(ro, rd,isct) && (t < RayBias || isct.t < t)){
        return true;
    }
    if(t < RayBias)
        return false;
    isct.t = t;
    isct.p = ro + t * rd;
    isct.n = vec3(0,1,0);
    isct.mat.emission = vec3(0);
    isct.mat.baseColor = vec3(1);
    return true;
}

// Returns 2D random point in [0,1]²
vec2 random2(vec2 st){
  st = vec2( dot(st,vec2(127.1,311.7)),
             dot(st,vec2(269.5,183.3)) );
  return fract(sin(st)*43758.5453123);
}
// Inputs:
//   st  3D seed
// Returns 2D random point in [0,1]²
vec2 random2(vec3 st){
  vec2 S = vec2( dot(st,vec3(127.1,311.7,783.089)),
             dot(st,vec3(269.5,183.3,173.542)) );
  return fract(sin(S)*43758.5453123);
}

struct Sampler{
    int dimension;
    uint seed;
};

float nextFloat(inout Sampler sampler){
    sampler.seed = (1103515245*(sampler.seed) + 12345);
    sampler.dimension++;
    return float(sampler.seed) * 2.3283064370807974e-10;
}

vec2 nextFloat2(inout Sampler sampler){
    return vec2(nextFloat(sampler), nextFloat(sampler));
}
struct LocalFrame{
    vec3 N, T, B;
};

void computeLocalFrame(vec3 N, out LocalFrame frame){
    frame.N = N;
    if(abs(N.x) > abs(N.y)){
        frame.T = vec3(-N.z, 0.0f, N.x) / sqrt(N.z * N.z + N.x * N.x);
    }else{
        frame.T = vec3(0.0f, -N.z, N.y) / sqrt(N.z * N.z + N.y * N.y);
    }
    frame.B = normalize(cross(N, frame.T));
}

vec3 worldToLocal(vec3 v, in LocalFrame frame){
    return vec3(dot(v, frame.T),dot(v,frame.N),dot(v, frame.B));
}

vec3 localToWorld(vec3 v, in LocalFrame frame){
    return v.x * frame.T +  v.y * frame.N + v.z * frame.B;
}

vec2 diskSampling(vec2 u){
    float r = u.x;
    float t = u.y * 2.0 * M_PI;
    r = sqrt(r);
    return vec2(r * cos(t),r * sin(t));
}

vec3 cosineHemisphereSampling(vec2 u){
    vec2 d = diskSampling(u);
    float h = 1.0 - dot(d, d);
    return vec3(d.x, sqrt(h), d.y);
}

vec3 LiBackground(vec3 o, vec3 d){
    if(0 != (options & ENABLE_ATMOSPHERE_SCATTERING)){
        float theta = 45.0/180.0 * M_PI;
        vec3 color = atmosphere(
            normalize(d),                   // normalized ray direction
            vec3(0,6372e3,0),               // ray origin
            sunPos,                        // position of the sun
            22.0,                           // intensity of the sun
            6371e3,                         // radius of the planet in meters
            6471e3,                         // radius of the atmosphere in meters
            vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
            21e-6,                          // Mie scattering coefficient
            8e3,                            // Rayleigh scale height
            1.2e3,                          // Mie scale height
            0.758                           // Mie preferred scattering direction
        );

        // Apply exposure.
        color = 1.0 - exp(-1.0 * color);
        return color;
    }else{
        return vec3(0);
    }
}

vec3 sampleBSDF(const Material mat, out vec3 wi, out float pdf){
    return vec3(0);
}
vec3 evaluateBSDF(const Material mat, const vec3 wo, const vec3 wi){
    return vec3(0);
}
float evaluatePdf(const Material mat, const vec3 wo, const vec3 wi){
    return 0.0f;
}

//#define AO
#ifdef AO
vec3 Li(vec3 o, vec3 d, inout Sampler sampler) {
    Intersection isct;
    float tmax = 100.0f;
    if(!intersect(o, d, isct)){
        return LiBackground(o, d);
    }
    LocalFrame frame;
    computeLocalFrame(isct.n, frame);
    vec3 wi = cosineHemisphereSampling(nextFloat2(sampler));
    wi = localToWorld(wi, frame);
    o = isct.p + RayBias * wi;
    d = wi;
    if(!intersect(o, d, isct) || isct.t > 30.0){
        return vec3(1);
    }
    return vec3(0);
}
#else
const int maxDepth = 5;
vec3 Li(vec3 o, vec3 d, inout Sampler sampler) {
    Intersection isct;
    float tmax = 100.0f;
    vec3 L = vec3(0);
    vec3 beta = vec3(1);
    for(int depth = 0;depth < maxDepth;depth++){
        if(!intersect(o, d, isct)){
            L += beta * LiBackground(o, d);
            break;
        }
        L += beta * isct.mat.emission;
        LocalFrame frame;
        computeLocalFrame(isct.n, frame);
        vec3 wo = worldToLocal(-d, frame);
        vec3 wi = cosineHemisphereSampling(nextFloat2(sampler));
        if(wo.y * wi.y <0.0f){
            wi.y *= -1.0;
        }
        wi = localToWorld(wi, frame);

        o = isct.p;
        d = wi;
        float pdf = abs(dot(isct.n, wi)) / M_PI;
        beta *= isct.mat.baseColor / M_PI * abs(dot(isct.n, wi)) / pdf;
    }
    return L;
}
#endif
vec3 removeNaN(vec3 v){
    return mix(v, vec3(0), isnan(v));
}
void main() {
    if(any(greaterThanEqual(gl_GlobalInvocationID.xy, iResolution)))
        return;
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    vec2 uv = pixelCoord.xy / iResolution;
    Sampler sampler;
    sampler.dimension = 0;
    sampler.seed = floatBitsToUint(imageLoad(seeds, pixelCoord).r);

    uv = 2.0 * uv - vec2(1.0);
    uv.y *= -1.0f;
    uv.x *= iResolution.x / iResolution.y;
    vec4 _o = (cameraOrigin * vec4(vec3(0), 1));
    vec3 o = _o.xyz / _o.w;
    float fov = 60.0 / 180.0 * M_PI;
    float z = 1.0 / tan(fov / 2.0);
    vec3 d = normalize(mat3(cameraDirection) * normalize(vec3(uv, z) - vec3(0,0,0)));
    vec4 color = vec4(removeNaN(Li(o, d, sampler)), 1.0);
    vec4 prevColor = imageLoad(accumlatedImage,  pixelCoord);
    if(iTime > 0)
        color += prevColor;
    imageStore(composedImage, pixelCoord, vec4(pow(color.rgb / color.a,vec3(1.0/2.2)), 1.0));
    imageStore(accumlatedImage,  pixelCoord, color);
    imageStore(seeds, pixelCoord, vec4(uintBitsToFloat(sampler.seed)));
}
)";