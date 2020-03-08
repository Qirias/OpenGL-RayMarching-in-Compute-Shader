#version 430
const int TILE_W      = 42;
const int TILE_H      = 42;
const ivec2 TILE_SIZE = ivec2(TILE_W, TILE_H);
layout(local_size_x = 42, local_size_y = 42) in;
layout(rgba32f, binding = 0) uniform image2D img_output;

uniform float c_x, c_y, c_z, c_i;

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
        zimaginary   = 2.0lf * zreal * zimaginary + cimaginary;
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

    // ivec2 dims = imageSize(img_output);

    double x = pixel_coords.x / 1024.0lf * c_z + c_x;
    double y = pixel_coords.y / 1024.0lf * c_z + c_y;


    int n = Mandelbrot(0.0lf, 0.0lf, x, y, maxIt);


    int row_index = (n * 100 / maxIt % 17);
    pixel         = vec4((n == maxIt ? vec3(0.0) : color_map[ row_index ]), 1.0);

    imageStore(img_output, pixel_coords, pixel);
}
