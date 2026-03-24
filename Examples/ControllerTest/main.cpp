#define THIRTEEN_IMPLEMENTATION
#include "../../thirteen.h"

#include <math.h>
#include <stdio.h>

static const unsigned int c_width = 64;
static const unsigned int c_height = 64;
static const bool c_fullscreen = false;

static const float c_thumbStickEpsilon = 0.01f;

int main(int argc, char** argv)
{
    Thirteen::SetApplicationName("Thirteen Demo - ControllerTest");

    unsigned char* pixels = Thirteen::Init(c_width, c_height, c_fullscreen);
    if (!pixels)
    {
        printf("Could not initialize Thirteen\n");
        return 1;
    }

    printf("Interact with the controller to see controller values\n\n");

    do
    {
        struct ButtonInfo
        {
            Thirteen::ControllerButton button;
            const char* name;
        };

        static const ButtonInfo buttons[] =
        {
            { Thirteen::ControllerButton::DPadUp, "DPadUp" },
            { Thirteen::ControllerButton::DPadDown, "DPadDown" },
            { Thirteen::ControllerButton::DPadLeft, "DPadLeft" },
            { Thirteen::ControllerButton::DPadRight, "DPadRight" },
            { Thirteen::ControllerButton::Start, "Start" },
            { Thirteen::ControllerButton::Back, "Back" },
            { Thirteen::ControllerButton::LeftThumb, "LeftThumb" },
            { Thirteen::ControllerButton::RightThumb, "RightThumb" },
            { Thirteen::ControllerButton::LeftShoulder, "LeftShoulder" },
            { Thirteen::ControllerButton::RightShoulder, "RightShoulder" },
            { Thirteen::ControllerButton::A, "A" },
            { Thirteen::ControllerButton::B, "B" },
            { Thirteen::ControllerButton::X, "X" },
            { Thirteen::ControllerButton::Y, "Y" }
        };

        for(int controllerIndex = 0; controllerIndex < 4; controllerIndex++)
        {

            for (const ButtonInfo& buttonInfo : buttons)
            {
                const bool currentState = Thirteen::GetControllerButton(controllerIndex, buttonInfo.button);
                const bool previousState = Thirteen::GetControllerButtonLastFrame(controllerIndex, buttonInfo.button);

                if (currentState != previousState)
                {
                    if (currentState)
                        printf("[%d]: %s pressed\n", controllerIndex, buttonInfo.name);
                    else
                        printf("[%d]: %s released\n", controllerIndex, buttonInfo.name);
                }
            }

            const float leftTrigger = Thirteen::GetControllerTrigger(controllerIndex, true);
            const float leftTriggerLastFrame = Thirteen::GetControllerTriggerLastFrame(controllerIndex, true);
            if (fabsf(leftTrigger - leftTriggerLastFrame) > c_thumbStickEpsilon)
                printf("[%d]: LeftTrigger: %.3f\n", controllerIndex, leftTrigger);

            const float rightTrigger = Thirteen::GetControllerTrigger(controllerIndex, false);
            const float rightTriggerLastFrame = Thirteen::GetControllerTriggerLastFrame(controllerIndex, false);
            if (fabsf(rightTrigger - rightTriggerLastFrame) > c_thumbStickEpsilon)
                printf("[%d]: RightTrigger: %.3f\n", controllerIndex, rightTrigger);

            float leftThumbX = 0.0f;
            float leftThumbY = 0.0f;
            float leftThumbXLastFrame = 0.0f;
            float leftThumbYLastFrame = 0.0f;
            Thirteen::GetControllerThumbstick(controllerIndex, true, leftThumbX, leftThumbY);
            Thirteen::GetControllerThumbstickLastFrame(controllerIndex, true, leftThumbXLastFrame, leftThumbYLastFrame);
            if (fabsf(leftThumbX - leftThumbXLastFrame) > c_thumbStickEpsilon || fabsf(leftThumbY - leftThumbYLastFrame) > c_thumbStickEpsilon)
                printf("[%d]: LeftThumb: x=%.3f, y=%.3f\n", controllerIndex, leftThumbX, leftThumbY);

            float rightThumbX = 0.0f;
            float rightThumbY = 0.0f;
            float rightThumbXLastFrame = 0.0f;
            float rightThumbYLastFrame = 0.0f;
            Thirteen::GetControllerThumbstick(controllerIndex, false, rightThumbX, rightThumbY);
            Thirteen::GetControllerThumbstickLastFrame(controllerIndex, false, rightThumbXLastFrame, rightThumbYLastFrame);
            if (fabsf(rightThumbX - rightThumbXLastFrame) > c_thumbStickEpsilon || fabsf(rightThumbY - rightThumbYLastFrame) > c_thumbStickEpsilon)
                printf("[%d]: RightThumb: x=%.3f, y=%.3f\n", controllerIndex, rightThumbX, rightThumbY);
        }
    }
    while (Thirteen::Render() && !Thirteen::GetKey(VK_ESCAPE));

    Thirteen::Shutdown();

    return 0;
}
