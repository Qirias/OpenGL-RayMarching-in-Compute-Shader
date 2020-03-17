#version 430
#define MAX_STEPS 64
#define MIN_DISTANCE .0001
const int TILE_W      = 32;
const int TILE_H      = 32;
const ivec2 TILE_SIZE = ivec2(TILE_W, TILE_H);
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

vec3 getCameraRayDir(vec2 uv, vec3 camPos, vec3 camTarget);
float sdSphere(vec3 p, float r);
float sdf(vec3 pos);
float castRay(vec3 rayOrigin, vec3 rayDir);
vec3 render(vec3 rayOrigin, vec3 rayDir);

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

    vec3 camPos    = vec3(0, 0, -1);
    vec3 camTarget = vec3(0, 0, 0);

    vec2 uv     = vec2(x, y);
    vec3 rayDir = getCameraRayDir(uv, camPos, camTarget);

    pixel = vec4(render(camPos, rayDir), 1.0);

    imageStore(img_output, pixel_coords, pixel);
}

vec3 getCameraRayDir(vec2 uv, vec3 camPos, vec3 camTarget)
{
    // Calculate camera's "orthonormal basis", i.e. its transform matrix components
    vec3 camForward = normalize(camTarget - camPos);
    vec3 camRight   = normalize(cross(vec3(0.0, 1.0, 0.0), camForward));
    vec3 camUp      = normalize(cross(camForward, camRight));

    float fPersp = radians(45);
    vec3 vDir    = normalize(uv.x * camRight + uv.y * camUp + camForward * fPersp);

    return vDir;
}

float sdSphere(vec3 p, float r) { return length(p) - r; }

float sdf(vec3 pos)
{
    float t = sdSphere(pos - vec3(0.0, 0.0, 10.0), 3.0);

    return t;
}

float castRay(vec3 rayOrigin, vec3 rayDir)
{
    float t = 0.0; // Stores current distance along ray

    for (int i = 0; i < MAX_STEPS; i++) {
        float res = sdf(rayOrigin + rayDir * t);
        if (res < (MIN_DISTANCE * t)) {
            return t;
        }
        t += res;
    }

    return -1.0;
}

vec3 render(vec3 rayOrigin, vec3 rayDir)
{
    float t = castRay(rayOrigin, rayDir);

    // Visualize depth
    vec3 col = vec3(1.0 - t * 0.075);

    return col;
}
