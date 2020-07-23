#include <iostream>

class FullScreenQuad
{
public:
    float quadVertices[24] = {
            //positions   texture Coords
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f
        };
    unsigned int quadVAO, quadVBO;

    void GenBuffer();
    void DeleteVertex();
    void DeleteBuffer();
};