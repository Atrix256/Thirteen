#define _CRT_SECURE_NO_WARNINGS // for STB

#include "../../thirteen.h"

#include <stdio.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

static const unsigned int c_width = 1024;
static const unsigned int c_height = 768;
static const bool c_fullscreen = false;

int main(int argc, char** argv)
{
    Thirteen::SetApplicationName("Thirteen Demo - Simple");

    unsigned char* pixels = Thirteen::Init(c_width, c_height, c_fullscreen);
    if (!pixels)
    {
        printf("Could not initialize Thirteen\n");
        return 1;
    }

    unsigned int frameIndex = 0;
    do
    {
        // V to toggle vsync
        if (Thirteen::GetKey('V') && !Thirteen::GetKeyLastFrame('V'))
            Thirteen::SetVSync(!Thirteen::GetVSync());

        // F to toggle full screen
        if (Thirteen::GetKey('F') && !Thirteen::GetKeyLastFrame('F'))
            Thirteen::SetFullscreen(!Thirteen::GetFullscreen());

        // S to save a screenshot
        if (Thirteen::GetKey('S') && !Thirteen::GetKeyLastFrame('S'))
            stbi_write_png("screenshot.png", c_width, c_height, 4, pixels, 0);

        for (size_t iy = 0; iy < c_height; ++iy)
        {
            for (size_t ix = 0; ix < c_width; ++ix)
            {
                size_t i = (iy * c_width + ix) * 4;
                pixels[i + 0] = static_cast<unsigned char>(frameIndex + ix);
                pixels[i + 1] = static_cast<unsigned char>(frameIndex + iy);
                pixels[i + 2] = static_cast<unsigned char>(frameIndex);
                pixels[i + 3] = 255;
            }
        }
        frameIndex++;
    }
    while (Thirteen::Render() && !Thirteen::GetKey(VK_ESCAPE));

    Thirteen::Shutdown();

    return 0;
}
