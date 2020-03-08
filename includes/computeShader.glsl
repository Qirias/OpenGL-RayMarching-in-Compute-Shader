#version 430
const int TILE_W      = 32;
const int TILE_H      = 32;
const ivec2 TILE_SIZE = ivec2(TILE_W, TILE_H);
layout(local_size_x = 32, local_size_y = 32) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

uniform float c_x, c_y, c_z, c_i;
// int maxIt = 100;

// clang-format off
const vec3 color_map[] = {
    {0.0,  0.0,  0.0},
    {0.26, 0.18, 0.06},
    {0.1,  0.03, 0.1}, 
    {0.04, 0.0,  0.18},
    {0.02, 0.02, 0.29},
    {0.0,  0.03, 0.39},
    {0.05, 0.17, 0.54},
    {0.09, 0.32, 0.69},
    {0.22, 0.49, 0.82},
    {0.52, 0.71, 0.9},
    {0.82, 0.92, 0.97},
    {0.94, 0.91, 0.75},
    {0.97, 0.79, 0.37},
    {1.0,  0.67, 0.0},
    {0.8,  0.5,  0.0},
    {0.6,  0.34, 0.0},
    {0.41, 0.2,  0.01}
};
// clang-format on

int Mandelbrot(double zreal, double zimaginary, double creal, double cimaginary, int maxIt)
{
    int it = 0;
    while (it < maxIt && zreal * zreal + zimaginary * zimaginary < 4.0) {
        double xtemp = zreal * zreal - zimaginary * zimaginary + creal;
        zimaginary   = 2 * zreal * zimaginary + cimaginary;
        zreal        = xtemp;
        it           = it + 1;
    }
    return it;
}

void main()
{
    vec4 pixel;
    // Compute global x, y coordinates utilizing local group ID
    const ivec2 tile_xy      = ivec2(gl_WorkGroupID);
    const ivec2 thread_xy    = ivec2(gl_LocalInvocationID);
    const ivec2 pixel_coords = tile_xy * TILE_SIZE + thread_xy;
    // ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    int maxIt = int(c_i);

    ivec2 dims = imageSize(img_output);

    double x = (double(pixel_coords.x * 2 - dims.x) / (dims.x / 1.5)) * c_z + c_x;
    double y = (double(pixel_coords.y * 2 - dims.y) / (dims.y / 1.5)) * c_z + c_y;

    double width  = dims.x;
    double height = dims.y;

    // float x = float(pixel_coords.x / float((width - 1) / 3.5) - 2.5);
    // float y = float(pixel_coords.y / float((height - 1) / 2) - 1.5);

    double zreal = 0, zimaginary = 0;
    double creal = x, cimaginary = y;

    int n = Mandelbrot(zreal, zimaginary, creal, cimaginary, maxIt);

    // float r = (n * 4 % 255) / 100;
    // float g = (n * n % 255) / 100;
    // float b = (n * 8 % 255) / 100;

    // vec3 color = vec3(r, g, b);
    // pixel      = vec4(color, 1.0);

    int row_index = (n * 100 / maxIt % 17);
    pixel         = vec4((n == maxIt ? vec3(0.0) : color_map[ row_index ]), 1.0);

    imageStore(img_output, pixel_coords, pixel);
}
