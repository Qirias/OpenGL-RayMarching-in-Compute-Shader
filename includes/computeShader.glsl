#version 430
#define MAX_STEPS 512
#define MIN_DIST .00001
#define M_PI 3.1415926
#define SHADOW_FALLOFF 0.05

const int TILE_W      = 32;
const int TILE_H      = 32;
const ivec2 TILE_SIZE = ivec2(TILE_W, TILE_H);
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

struct Camera {
    vec4 pos;
    vec4 dir;
    vec4 yAxis;
    vec4 xAxis;
};

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct Ray {
    vec3 origin;
    vec3 dir;
};

float sdSphere(vec3 p, float r);
float sdf(vec3 pos);
float RayMarch(vec3 rayOrigin, vec3 rayDir);
vec3 render(vec3 rayOrigin, vec3 rayDir);
vec3 sphereNormal(vec3 pos);
float sdPlane(vec3 p, vec4 n);
vec3 getPointLight(vec3 color, vec3 normal, vec3 pos);

uniform float iTime;
// Calculated Euler Angles
uniform vec3 mouse;
// Carries the current pixel location of the mouse cursor
uniform vec2 iMouse;
// Camera rotation and movement values
uniform Camera camera;
uniform Light light;

float rand(vec2 co)
{
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

Ray castRay(vec2 uv)
{
    float Perpective = radians(45);
    vec4 dir         = normalize(uv.x * camera.xAxis + uv.y * camera.yAxis + camera.dir * Perpective);

    return Ray(camera.pos.xyz, dir.xyz);
}

float sdSphere(vec3 p, float r) { return length(p) - r; }

float sdPlane(vec3 p, vec4 n) { return dot(p, n.xyz) + n.w; }

float opU(float d1, float d2) { return (d1.x < d2) ? d1 : d2; }

float sdf(vec3 pos)
{
    float t = sdSphere(pos - (vec3(0.0, 0.0, -10.0)), 3.0);
    t       = opU(t, sdPlane(pos, vec4(0, 1, 0, 5.5)));
    t       = opU(t, sdSphere(pos - (vec3(0.0, 0.0, 20.0)), 3.0));
    return t;
}

float RayMarch(vec3 rayOrigin, vec3 rayDir)
{
    float t = 0.0; // Stores current distance along ray

    for (int i = 0; i < MAX_STEPS; i++) {
        float res = sdf(rayOrigin + rayDir * t);
        if (res < (MIN_DIST * t)) {
            return t;
        }
        t += res;
    }

    return -1.0;
}

vec3 render(vec3 rayOrigin, vec3 rayDir)
{
    float t = RayMarch(rayOrigin, rayDir);
    vec3 color;

    if (t == -1.0) {
        color = vec3(0.30, 0.36, 0.60) - (rayDir.y * 0.7);
    }
    else {
        vec3 pos      = rayOrigin + rayDir * t;
        vec3 normal   = sphereNormal(pos);
        color         = normal * vec3(0.5) + vec3(0.5);
        color         = getPointLight(color, normal, pos);

        float shadow = 0.0;
        vec3 shadowRayOrigin = pos + normal * 0.02;
        float r = rand(vec2(rayDir.xy)) * 2.0 - 1.0;
        vec3 shadowRayDir = light.position - pos + vec3(1.0 * SHADOW_FALLOFF) * r;
        float d = RayMarch(shadowRayOrigin, shadowRayDir);
        if (d != -1.0)
            shadow += 1.0;
        color = mix(color, color * 0.2, shadow);
    }
    // Gamma Correction
    color = pow(color, vec3(0.4545));
    return color;
}

vec3 getPointLight(vec3 color, vec3 normal, vec3 pos)
{
    vec3 ambient = light.ambient;
    vec3 viewDir = normalize(pos - camera.pos.xyz);

    vec3 lightDir     = normalize(light.position - pos);
    float NtoL_vector = max(dot(normal, lightDir), 0.0);
    vec3 diffuse      = light.diffuse * NtoL_vector;
    
    vec3 reflectDir = reflect(lightDir, normal);
    float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 specular = light.specular * spec;
    
    float distance    = length(light.position - pos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance)); 
    
    diffuse  *= attenuation;
    ambient  *= attenuation;
    specular *= attenuation;

    vec3 final    = color * (diffuse + ambient + specular);
    return final;
}

vec3 sphereNormal(vec3 pos)
{
    float c = sdf(pos);

    vec2 eps_zero = vec2(0.001, 0.0);
    // clang-format off
    return normalize(vec3(sdf(pos + eps_zero.xyy),
                          sdf(pos + eps_zero.yxy),
                          sdf(pos + eps_zero.yyx)) - c);
    // clang-format on
}

void main()
{
    vec4 pixel;
    // Compute global x, y coordinates utilizing local group ID
    const ivec2 tile_xy      = ivec2(gl_WorkGroupID);
    const ivec2 thread_xy    = ivec2(gl_LocalInvocationID);
    const ivec2 pixel_coords = tile_xy * TILE_SIZE + thread_xy;
    // ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    ivec2 dims = imageSize(img_output);
    float x    = (float(pixel_coords.x * 2 - dims.x) / dims.x);
    float y    = (float(pixel_coords.y * 2 - dims.y) / dims.y);

    vec2 uv = vec2(x, y);

    Ray r = castRay(uv);

    pixel = vec4(render(r.origin, r.dir), 1.0);

    imageStore(img_output, pixel_coords, pixel);
}
