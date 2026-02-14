# Thirteen

[![Build Status](https://github.com/Atrix256/Thirteen/actions/workflows/build.yml/badge.svg)](https://github.com/Atrix256/Thirteen/actions)

**Thirteen** is a header-only C++ library that initializes a window and gives you a pointer to RGBA uint8 pixels to write to, which are copied to the screen every time you call `Render()`.

It is inspired by the simplicity of the Mode 13h days where you initialized the graphics mode and then started writing pixels directly to the screen. Just include the header, initialize, and start drawing!

Platforms Supported:
* Win32/DX12
* MacOS/Metal
* Linux/X11+OpenGL

## Usage

```cpp
#include "thirteen.h"

int main()
{
    unsigned char* pixels = Thirteen::Init(800, 600);
    if (!pixels)
        return 1;

    unsigned int frameIndex = 0;

    // Go until window is closed or escape is pressed
    while (Thirteen::Render() && !Thirteen::GetKey(VK_ESCAPE))
    {
        // Write to pixels (RGBA format, 4 bytes per pixel)
        for (int i = 0; i < 800 * 600 * 4; i += 4)
        {
            pixels[i + 0] = 255; // Red
            pixels[i + 1] = unsigned char(frameIndex);   // Green
            pixels[i + 2] = unsigned char(frameIndex / 2);   // Blue
            pixels[i + 3] = 255; // Alpha
        }
        frameIndex++;
    }

    Thirteen::Shutdown();
    return 0;
}
```

## Examples

How to build examples:
* **Windows**: build Examples.sln
* **Mac**: For each example: clang++ main.cpp -framework Metal -framework Cocoa -std=c++11 -DTARGET_OS_OSX
* **Linux**: For each example: g++ main.cpp -o Simple

### Simple
A basic example showing the fundamentals of Thirteen. [View source code](Examples/Simple/main.cpp)

![Simple Example](Simple.png)

### Mandelbrot
A Mandelbrot set fractal visualizer with interactive navigation. [View source code](Examples/Mandelbrot/main.cpp)

![Mandelbrot Example](Mandelbrot.png)

### Minesweeper
A classic Minesweeper game implementation. [View source code](Examples/Minesweeper/main.cpp)

![Minesweeper Example](Minesweeper.png)

## API Reference

#### `uint8* Init(uint32 width = 1024, uint32 height = 768, bool fullscreen = false)`
Initializes the window and DirectX 12 renderer.

**Parameters:**
- `width` - Width of the rendering surface in pixels (default: 1024)
- `height` - Height of the rendering surface in pixels (default: 768)
- `fullscreen` - Whether to start in fullscreen mode (default: false)

**Returns:** Pointer to the pixel buffer (RGBA format, 4 bytes per pixel) on success, or `nullptr` on failure.

---

#### `bool Render()`
Processes window messages, updates input state, tracks frame timing, and copies the pixel buffer to the screen.

**Returns:** `true` to continue rendering, `false` when the application should quit (e.g., window closed).

---

#### `void Shutdown()`
Cleans up all DirectX resources and destroys the window. Call this before exiting your application.

---

#### `void SetVSync(bool enabled)`
Enables or disables vertical synchronization.

**Parameters:**
- `enabled` - `true` to enable vsync (limit to monitor refresh rate), `false` to disable

---

#### `bool GetVSync()`
**Returns:** `true` if vsync is enabled, `false` otherwise.

---

#### `void SetFullscreen(bool fullscreen)`
Switches between windowed and fullscreen mode.

**Parameters:**
- `fullscreen` - `true` to switch to fullscreen (borderless window), `false` for windowed mode

---

#### `bool GetFullscreen()`
**Returns:** `true` if currently in fullscreen mode, `false` if windowed.

---

#### `uint32 GetWidth()`
**Returns:** The current width of the rendering surface in pixels.

---

#### `uint32 GetHeight()`
**Returns:** The current height of the rendering surface in pixels.

---

#### `uint8* SetSize(uint32 width, uint32 height)`
Resizes the rendering surface to the specified dimensions. This changes the window size, recreates internal DirectX buffers and reallocates the pixel buffer.

**Parameters:**
- `width` - New width in pixels
- `height` - New height in pixels

**Returns:** Pointer to the pixel buffer (which may be different from the pointer returned by `Init()`) on success, or `nullptr` if the resize failed.

**Note:** Always use the returned pointer as the pixel buffer address may change after resizing.

---

#### `void SetApplicationName(const char* name)`
Sets the application name displayed in the window title bar (along with FPS).

**Parameters:**
- `name` - The name to display in the title bar

---

#### `double GetDeltaTime()`
**Returns:** The duration of the previous frame in seconds. Useful for frame-independent movement and animation.

---

#### `void GetMousePosition(int& x, int& y)`
Gets the current mouse position relative to the window.

**Parameters:**
- `x` - Reference to store the X coordinate in pixels
- `y` - Reference to store the Y coordinate in pixels

---

#### `void GetMousePositionLastFrame(int& x, int& y)`
Gets the mouse position from the previous frame.

**Parameters:**
- `x` - Reference to store the X coordinate in pixels
- `y` - Reference to store the Y coordinate in pixels

---

#### `bool GetMouseButton(int button)`
Checks if a mouse button is currently pressed.

**Parameters:**
- `button` - Button to check: `0` = left, `1` = right, `2` = middle

**Returns:** `true` if the button is currently pressed.

---

#### `bool GetMouseButtonLastFrame(int button)`
Checks if a mouse button was pressed in the previous frame.

**Parameters:**
- `button` - Button to check: `0` = left, `1` = right, `2` = middle

**Returns:** `true` if the button was pressed in the previous frame.

---

#### `bool GetKey(int keyCode)`
Checks if a keyboard key is currently pressed.

**Parameters:**
- `keyCode` - Windows virtual key code (e.g., `VK_ESCAPE`, `VK_SPACE`, `'A'`, etc.)

**Returns:** `true` if the key is currently pressed.

---

#### `bool GetKeyLastFrame(int keyCode)`
Checks if a keyboard key was pressed in the previous frame.

**Parameters:**
- `keyCode` - Windows virtual key code

**Returns:** `true` if the key was pressed in the previous frame.

---

### Input Helpers

To detect a single key press or button click (not held down), compare current and previous frame states:

```cpp
// Detect key just pressed this frame
bool keyPressed = Thirteen::GetKey(VK_SPACE) && !Thirteen::GetKeyLastFrame(VK_SPACE);

// Detect key just released this frame
bool keyReleased = !Thirteen::GetKey(VK_SPACE) && Thirteen::GetKeyLastFrame(VK_SPACE);

// Same pattern works for mouse buttons
bool clicked = Thirteen::GetMouseButton(0) && !Thirteen::GetMouseButtonLastFrame(0);
```

## Credits

Alan Wolfe - API, examples and Win32/DX12 implementation

Francesco Carucci - MacOS/Metal implementation

Nikita Lisitsa - Linux/X11+OpenGL
