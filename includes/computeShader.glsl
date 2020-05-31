#version 430
#define MAX_STEPS 256
#define MIN_DIST .0001
#define M_PI 3.1415926

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
Ray castRay(vec2 uv);

// Time since glfw was initialized
uniform float iTime;
// Calculated Euler Angles
uniform vec3 mouse;
// Carries the current pixel location of the mouse cursor
uniform vec2 iMouse;
// Camera rotation and movement values
uniform Camera camera;

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
    t       = opU(t, sdSphere(pos - (vec3(0.0, 0.0, 30.0)), 3.0));
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
    vec3 col;
    vec3 L = normalize(vec3(1.0, 2.0, 1.0)); // normalize(vec3(sin(iTime), 2.0, cos(iTime)));

    if (t == -1.0) {
        col = vec3(0.30, 0.36, 0.60) - (rayDir.y * 0.7);
    }
    else {
        vec3 pos = rayOrigin + rayDir * t;
        vec3 N   = sphereNormal(pos);
        col      = N * vec3(0.5) + vec3(0.5);

        float NoL         = max(dot(N, L), 0.0);
        vec3 LDirectional = vec3(0.9, 0.9, 0.8) * NoL;
        vec3 LAmbient     = vec3(0.03, 0.04, 0.1);
        vec3 diffuse      = col * (LDirectional + LAmbient);
        col               = diffuse;

        float d = RayMarch(pos + N * .01, L);
        if (d != -1) {
            col = vec3(col * 0.1);
        }
    }
    // Gamma Correction
    col = pow(col, vec3(0.4545));
    return col;
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
