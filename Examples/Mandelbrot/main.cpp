#define _CRT_SECURE_NO_WARNINGS // for STB

#include "../../thirteen.h"

#include <stdio.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static const unsigned int c_width = 1024;
static const unsigned int c_height = 768;
static const bool c_fullscreen = false;

static const int c_maxIterations = 1000;

void GetMandelbrotColor(float t, unsigned char& r, unsigned char& g, unsigned char& b)
{
    // Clamp t to [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Create a multi-band color gradient
    // Deep blue -> Cyan -> Green -> Yellow -> Orange -> Red -> Dark Red

    if (t < 0.16f)
    {
        // Deep blue to cyan
        float local_t = t / 0.16f;
        r = (unsigned char)(0 + local_t * 0);
        g = (unsigned char)(0 + local_t * 128);
        b = (unsigned char)(64 + local_t * 191);
    }
    else if (t < 0.33f)
    {
        // Cyan to green
        float local_t = (t - 0.16f) / 0.17f;
        r = (unsigned char)(0 + local_t * 0);
        g = (unsigned char)(128 + local_t * 127);
        b = (unsigned char)(255 - local_t * 255);
    }
    else if (t < 0.5f)
    {
        // Green to yellow
        float local_t = (t - 0.33f) / 0.17f;
        r = (unsigned char)(0 + local_t * 255);
        g = (unsigned char)(255);
        b = (unsigned char)(0);
    }
    else if (t < 0.67f)
    {
        // Yellow to orange
        float local_t = (t - 0.5f) / 0.17f;
        r = (unsigned char)(255);
        g = (unsigned char)(255 - local_t * 100);
        b = (unsigned char)(0);
    }
    else if (t < 0.84f)
    {
        // Orange to red
        float local_t = (t - 0.67f) / 0.17f;
        r = (unsigned char)(255);
        g = (unsigned char)(155 - local_t * 155);
        b = (unsigned char)(0);
    }
    else
    {
        // Red to dark red
        float local_t = (t - 0.84f) / 0.16f;
        r = (unsigned char)(255 - local_t * 128);
        g = (unsigned char)(0);
        b = (unsigned char)(0);
    }
}

float MandelbrotIterations(float x, float y)
{
    double z = 0.0;
    double zi = 0.0;

    for (int i = 0; i < c_maxIterations; ++i)
    {
        double newz = (z * z) - (zi * zi) + double(x);
        double newzi = 2 * z * zi + double(y);
        z = newz;
        zi = newzi;

        if (((z * z) + (zi * zi)) > 4.0)
            return float(i) / float(c_maxIterations - 1);
    }

    return -1.0f;
}

int main(int argc, char** argv)
{
    Thirteen::SetApplicationName("Thirteen Demo - Mandelbrot");

    unsigned char* pixels = Thirteen::Init(c_width, c_height, c_fullscreen);
    if (!pixels)
    {
        printf("Could not initialize Thirteen\n");
        return 1;
    }

    bool dirty = true;
    float centerX = 0.0f;
    float centerY = 0.0f;
    float height = 5.0f;
    static const float c_aspectRatio = float(c_width) / float(c_height);

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

        // Space to reset the camera
        if (Thirteen::GetKey(VK_SPACE) && !Thirteen::GetKeyLastFrame(VK_SPACE))
        {
            centerX = 0.0f;
            centerY = 0.0f;
            height = 5.0f;
            dirty = true;
        }

        // Zoom in on left click
        if (Thirteen::GetMouseButton(0) && !Thirteen::GetMouseButtonLastFrame(0))
        {
            dirty = true;
            int mouseX, mouseY;
            Thirteen::GetMousePosition(mouseX, mouseY);
            float percentX = float(mouseX) / float(c_width);
            float percentY = float(mouseY) / float(c_height);
            centerX += (percentX - 0.5f) * height * c_aspectRatio * 0.5f;
            centerY += (percentY - 0.5f) * height * 0.5f;
            height *= 0.5f;
        }

        // Zoom out on right click
        if (Thirteen::GetMouseButton(1) && !Thirteen::GetMouseButtonLastFrame(1))
        {
            dirty = true;
            int mouseX, mouseY;
            Thirteen::GetMousePosition(mouseX, mouseY);
            float percentX = float(mouseX) / float(c_width);
            float percentY = float(mouseY) / float(c_height);
            centerX += (percentX - 0.5f) * height * c_aspectRatio * -0.5f;
            centerY += (percentY - 0.5f) * height * -0.5f;
            height *= 2.0f;
        }

        // only render when needed
        if (dirty)
        {
            dirty = false;

            #pragma omp parallel for
            for (int iy = 0; iy < c_height; ++iy)
            {
                float percentY = (float(iy) + 0.5f) / float(c_height);
                float posY = centerY + (percentY - 0.5f) * height;

                for (int ix = 0; ix < c_width; ++ix)
                {
                    float percentX = (float(ix) + 0.5f) / float(c_width);
                    float posX = centerX + (percentX - 0.5f) * height * c_aspectRatio;

                    size_t i = (iy * c_width + ix) * 4;

                    float iter = MandelbrotIterations(posX, posY);

                    if (iter < 0.0f)
                    {
                        pixels[i + 0] = 0;
                        pixels[i + 1] = 0;
                        pixels[i + 2] = 0;
                        pixels[i + 3] = 255;
                    }
                    else
                    {
                        GetMandelbrotColor(iter, pixels[i + 0], pixels[i + 1], pixels[i + 2]);
                        pixels[i + 3] = 255;
                    }
                }
            }
        }
    }
    while (Thirteen::Render() && !Thirteen::GetKey(VK_ESCAPE));

    Thirteen::Shutdown();

    return 0;
}
