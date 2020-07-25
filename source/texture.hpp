#include <iostream>

class Texture
{
public:
    int texWidth;
    int texHeight;
    unsigned int texOutput;

    Texture() noexcept;
    Texture(const unsigned int SCREEN_WIDTH, const unsigned int SCREEN_HEIGHT) noexcept;

    void GenerateTexture();
};