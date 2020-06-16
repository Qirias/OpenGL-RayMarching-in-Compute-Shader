#version 450
#define MAX_STEPS 512
#define MIN_DIST .000001
#define M_PI 3.1415926
#define SHADOW_FALLOFF 0.05
#extension GL_NV_compute_shader_derivatives : enable

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

struct RayIntersection {
    float hitpoint;
    vec3 color;
    int object;
};


float sdSphere(vec3 p, float r);
RayIntersection sdf(vec3 pos);
RayIntersection RayMarch(vec3 rayOrigin, vec3 rayDir);
RayIntersection reflectedRay(vec3 rayOrigin, vec3 rayDir);
vec3 render(vec3 rayOrigin, vec3 rayDir);
vec3 GetNormal(vec3 pos);
float sdPlane(vec3 p, vec4 n);
vec3 getPointLight(vec3 color, vec3 normal, vec3 pos);
vec3 bounce(vec3 rayDir, vec3 pos, vec3 normal, vec3 itemCol, vec3 color, float shadow, int object);
float softshadow(vec3 ro, vec3 rd, float k);

uniform int bounceVar; //number of bouncing rays
uniform float drand48; // testing
uniform float iTime; // Time since glfw was initialized
uniform vec3 mouse; // Calculated Euler Angles
uniform vec2 iMouse; // Carries the current pixel location of the mouse cursor
uniform Camera camera; // Camera rotation and movement values
uniform Light light;

Ray castRay(vec2 uv)
{
    float Perpective = radians(45);
    vec4 dir         = normalize(uv.x * camera.xAxis + uv.y * camera.yAxis + camera.dir * Perpective);

    return Ray(camera.pos.xyz, dir.xyz);
}

float sdSphere(vec3 p, float r) { return length(p) - r; }

float sdPlane(vec3 p, vec4 n) { return dot(p, n.xyz) + n.w; }

RayIntersection opU(RayIntersection d1, RayIntersection d2) { return (d1.hitpoint < d2.hitpoint) ? d1 : d2; }

RayIntersection sdf(vec3 pos)
{
    // The last vec3 refers to the color of each objectect
    RayIntersection t;
    t         = RayIntersection(sdSphere(pos - vec3(0.0, 0.0, -10.0), 3.0), vec3(0.8824, 0.0, 1.0), 0);
    t         = opU(t, RayIntersection(sdSphere(pos - vec3(-15.0, 0.0, -10.0), 3.0), vec3(0.0, 0.2667, 1.0), 1));
    t         = opU(t, RayIntersection(sdPlane(pos, vec4(0, 1, 0, 5.5)), vec3(1.0, 1.0, 1.0), 2));
    return t;
}

RayIntersection RayMarch(vec3 rayOrigin, vec3 rayDir)
{
    float t = 0.0; // Stores current distance along ray
    float tmax = 400;
    RayIntersection dummy = {-1.0, vec3(0.0), -1};

    for (int i = 0; i < MAX_STEPS; i++) {
        RayIntersection res = sdf(rayOrigin + rayDir * t);
        if (res.hitpoint < (MIN_DIST * t)) {
            return RayIntersection (t, res.color, res.object); 
        }
        if (res.hitpoint > tmax)
            return dummy;
        t += res.hitpoint;
    }

    return dummy;
}

RayIntersection reflectedRay(vec3 rayOrigin, vec3 rayDir)
{
    float t = 0.0; // Stores current distance along ray
    float tmax = 200;
    RayIntersection dummy = {-1.0, vec3(0.0), -1};

    for (int i = 0; i < MAX_STEPS / 2; i++) {
        RayIntersection res = sdf(rayOrigin + rayDir * t);
        if (res.hitpoint < (MIN_DIST * t)) {
            return RayIntersection (t, res.color, res.object); 
        }
        if (res.hitpoint > tmax)
            return dummy;
        t += res.hitpoint;
    }

    return dummy;
}

vec3 bounce(vec3 rayDir, vec3 pos, vec3 normal, vec3 itemCol, vec3 color, float shadow, int object)
{
    for (int i = 0; i < bounceVar; i++)
    {
        rayDir                  = reflect(rayDir, normal);
        RayIntersection t       = reflectedRay(pos + normal * 0.001, rayDir);
        pos                     = pos + rayDir * t.hitpoint;
        normal                  = GetNormal(pos);
        t.color                 = getPointLight(t.color, normal, pos);
        color                   += t.color * itemCol;
    }
    
    if (object == 2)
        color *= shadow;
    
    color /= bounceVar;
    return color;
}

float softshadow(vec3 ro, vec3 rd, float k)
{
    float res = 1.0;
    float t = 0.0;
    for (int i = 0; i < MAX_STEPS / 8; i++)
    {
        RayIntersection h = sdf(ro + rd * t);
        if(h.hitpoint < 0.001)
            return 0.05;

        res = min(res, k * h.hitpoint / t);
        t += h.hitpoint;
    }

    return res;
}

vec3 render(vec3 rayOrigin, vec3 rayDir)
{
    vec3 color;
    color = vec3(0.30, 0.36, 0.60) - (rayDir.y * 0.2);
    vec3 normal = vec3(0.0,1.0,0.0);
    RayIntersection t = RayMarch(rayOrigin, rayDir);
    float shadow = 0.05;
    
    if (t.hitpoint != -1.0) {
            vec3 pos      = rayOrigin + rayDir * t.hitpoint;
            normal        = GetNormal(pos);
            vec3 itemCol  = t.color;
            color         = t.color;//normal * vec3(0.5) + vec3(0.5);
            color         = getPointLight(color, normal, pos);
        
        if (t.object == 2) // Everything bellow is applied only for the floor plane
        {
            vec3 shadowRayOrigin = pos + normal * 0.02;
            vec3 shadowRayDir = light.position - pos;
            shadow = softshadow(shadowRayOrigin, shadowRayDir, 0.8);
            color *= shadow;
        }

        if (bounceVar > 0)
            color = bounce(rayDir, pos, normal, itemCol, color, shadow, t.object);
        
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

vec3 GetNormal(vec3 pos)
{
    float c = sdf(pos).hitpoint;

    vec2 eps_zero = vec2(0.001, 0.0);
    // clang-format off
    return normalize(vec3(sdf(pos + eps_zero.xyy).hitpoint,
                          sdf(pos + eps_zero.yxy).hitpoint,
                          sdf(pos + eps_zero.yyx).hitpoint) - c);
    // clang-format on
}


void main()
{
    vec4 pixel;
    vec4 finalColor = vec4(0);
    // Compute global x, y coordinates utilizing local group ID
    const ivec2 tile_xy      = ivec2(gl_WorkGroupID);
    const ivec2 thread_xy    = ivec2(gl_LocalInvocationID);
    const ivec2 pixel_coords = tile_xy * TILE_SIZE + thread_xy;
    // ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    ivec2 dims = imageSize(img_output);
    float x    = (float(pixel_coords.x * 2 - dims.x) / dims.x);
    float y    = (float(pixel_coords.y * 2 - dims.y) / dims.y);

    vec2 uv = vec2(x, y);

    float AA = 2.0; // anti aliasing samples, the current is 4 rays
    int cnt = 0;
    for (float AAY = 0.0; AAY < AA; AAY++)
    {
        for (float AAX = 0.0; AAX < AA; AAX++)
        {
            Ray r = castRay(uv + vec2(AAX, AAY) / dims);
            pixel = vec4(render(r.origin, r.dir), 1.0);
            finalColor += pixel;
            cnt++;
        }
    }
    finalColor /= cnt;

    // ====Anti Aliasing off====
    // Ray r = castRay(uv);
    // pixel = vec4(render(r.origin, r.dir), 1.0);

    imageStore(img_output, pixel_coords, finalColor);
}
