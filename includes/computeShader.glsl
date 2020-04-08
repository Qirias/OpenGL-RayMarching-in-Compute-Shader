#version 430
#define MAX_STEPS 256
#define MIN_DIST .0001
#define M_PI 3.1415926

const int TILE_W      = 32;
const int TILE_H      = 32;
const ivec2 TILE_SIZE = ivec2(TILE_W, TILE_H);
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

float sdSphere(vec3 p, float r);
float sdf(vec3 pos);
float castRay(vec3 rayOrigin, vec3 rayDir);
vec3 render(vec3 rayOrigin, vec3 rayDir);
vec3 sphereNormal(vec3 pos);
float sdPlane(vec3 p, vec4 n);
vec4 lookAtRH(vec2 uv, vec3 eye, vec3 center);

vec4 quatMult(vec4 p, vec4 q);
vec4 quatRotor(vec3 axis, float phi);
vec3 quatRotate(vec3 point, vec4 rotor);

uniform float iTime, xaxis, zaxis, yaxis;
// Calculated Euler Angles
uniform vec3 mouse;
// Carries the current pixel location of the mouse cursor
uniform vec2 iMouse;

vec3 key = vec3(xaxis, 0, zaxis);

vec4 quatMult(vec4 p, vec4 q)
{
    return vec4(p.w * q.xyz + q.w * p.xyz + cross(p.xyz, q.xyz), p.w * q.w - dot(p.xyz, q.xyz));
}

vec4 quatRotor(vec3 axis, float phi)
{
    phi *= 0.5;
    return vec4(sin(phi) * normalize(axis), cos(phi));
}

vec3 quatRotate(vec3 point, vec4 rotor)
{
    return quatMult(rotor, vec4(point * rotor.w - cross(point, rotor.xyz), dot(point, rotor.xyz))).xyz;
}

vec4 lookAtRH(vec2 uv, vec3 eye, vec3 center)
{
    vec4 rotor = quatRotor(vec3(0., -1., 0.), -iMouse.x);
    rotor      = quatMult(rotor, quatRotor(vec3(1., 0., 0.), iMouse.y));
    vec3 f     = normalize(center - eye);
    vec3 r     = normalize(cross(vec3(0.0, 1.0, 0.0), f));
    vec3 u     = normalize(cross(f, r));

    float fPersp = radians(45);
    vec4 dir     = vec4(normalize(uv.x * r + uv.y * u + f * fPersp), 1.0);

    mat4 m;
    m[ 0 ][ 0 ] = r.x;
    m[ 0 ][ 1 ] = r.y;
    m[ 0 ][ 2 ] = r.z;
    m[ 1 ][ 0 ] = u.x;
    m[ 1 ][ 1 ] = u.y;
    m[ 1 ][ 2 ] = u.z;
    m[ 2 ][ 0 ] = -f.x;
    m[ 2 ][ 1 ] = -f.y;
    m[ 2 ][ 2 ] = -f.z;
    m[ 3 ][ 0 ] = center.x;
    m[ 3 ][ 1 ] = center.y;
    m[ 3 ][ 2 ] = center.z;
    m[ 3 ][ 3 ] = 1.0;

    dir     = m * dir;
    dir.xyz = quatRotate(dir.xyz, rotor);

    return dir;
}

float sdSphere(vec3 p, float r) { return length(p) - r; }

float sdPlane(vec3 p, vec4 n) { return dot(p, n.xyz) + n.w; }

float opU(float d1, float d2) { return (d1.x < d2) ? d1 : d2; }

float sdf(vec3 pos)
{
    float t = sdSphere(pos - (vec3(0.0, 0.0, -10.0)), 3.0);
    t       = opU(t, sdPlane(pos, vec4(0, 1, 0, 5.5)));
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

    vec3 eye    = vec3(key);
    vec3 center = vec3(0, 0, 0);

    vec2 uv = vec2(x, y);

    vec4 dir = lookAtRH(uv, eye, center);

    pixel = vec4(render(eye, dir.xyz), 1.0);

    imageStore(img_output, pixel_coords, pixel);
}
