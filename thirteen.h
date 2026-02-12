/*
Thirteen v1.0.0
by Alan Wolfe
MIT licensed https://github.com/Atrix256/Thirteen

Thirteen is a header-only C++ library that initializes a DirectX 12 window and gives you a pointer to RGBA uint8 pixels to write to, which are copied to the screen every time you call Render().

It is inspired by the simplicity of the Mode 13h days where you initialized the graphics mode and then started writing pixels directly to the screen. Just include the header, initialize, and start drawing!
*/

#pragma once

#define DX12VALIDATION() (_DEBUG && false)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

namespace Thirteen
{
    using uint8 = unsigned char;
    using uint32 = unsigned int;

    // ========== Function Prototypes ==========

    // Initializes window and DX12. Returns a pointer to the pixel buffer on success, or nullptr on failure.
    uint8* Init(uint32 width = 1024, uint32 height = 768, bool fullscreen = false);

    // Renders a frame by copying Pixels to the screen. Returns false when the application should quit.
    [[nodiscard]] bool Render();

    // Cleans up all resources and shuts down DirectX and the window.
    void Shutdown();

    // Enables or disables vertical sync.
    void SetVSync(bool enabled);

    // Returns whether vertical sync is enabled.
    [[nodiscard]] bool GetVSync();

    // Sets the application name displayed in the window title bar.
    void SetApplicationName(const char* name);

    // Switches between windowed and fullscreen mode.
    void SetFullscreen(bool fullscreen);

    // Returns whether the application is currently in fullscreen mode.
    [[nodiscard]] bool GetFullscreen();

    // Returns the current width of the rendering surface in pixels.
    [[nodiscard]] uint32 GetWidth();

    // Returns the current height of the rendering surface in pixels.
    [[nodiscard]] uint32 GetHeight();

    // Sets the size of the rendering surface. Recreates internal buffers. Returns the new pixel buffer pointer on success, or nullptr on failure. The returned pointer may differ from the one returned by Init().
    [[nodiscard]] uint8* SetSize(uint32 width, uint32 height);

    // Returns the duration of the previous frame in seconds.
    [[nodiscard]] double GetDeltaTime();

    // Gets the current mouse position in pixels.
    void GetMousePosition(int& x, int& y);

    // Gets the mouse position from the previous frame in pixels.
    void GetMousePositionLastFrame(int& x, int& y);

    // Returns whether a mouse button is currently pressed (0=left, 1=right, 2=middle).
    [[nodiscard]] bool GetMouseButton(int button);

    // Returns whether a mouse button was pressed in the previous frame (0=left, 1=right, 2=middle).
    [[nodiscard]] bool GetMouseButtonLastFrame(int button);

    // Returns whether a keyboard key is currently pressed (use Windows virtual key codes).
    [[nodiscard]] bool GetKey(int keyCode);

    // Returns whether a keyboard key was pressed in the previous frame (use Windows virtual key codes).
    [[nodiscard]] bool GetKeyLastFrame(int keyCode);

    // ========================================

    // Internal state
    namespace Internal
    {
        HWND hwnd = nullptr;
        uint32 width = 320;
        uint32 height = 200;
        bool shouldQuit = false;
        bool vsyncEnabled = true;
        bool tearingSupported = false;
        bool isFullscreen = false;
        std::string appName = "ThirteenApp";

        // Frame timing
        LARGE_INTEGER perfFrequency = {};
        LARGE_INTEGER lastFrameTime = {};
        double lastDeltaTime = 0.0;
        double frameTimeSum = 0.0;
        int frameCount = 0;
        double averageFPS = 0.0;
        double titleUpdateTimer = 0.0;

        // Input state
        int mouseX = 0;
        int mouseY = 0;
        int prevMouseX = 0;
        int prevMouseY = 0;
        bool mouseButtons[3] = { false, false, false }; // Left, Right, Middle
        bool prevMouseButtons[3] = { false, false, false };
        bool keys[256] = {};
        bool prevKeys[256] = {};

        // DirectX 12 objects
        ID3D12Device* device = nullptr;
        ID3D12CommandQueue* commandQueue = nullptr;
        IDXGISwapChain3* swapChain = nullptr;
        ID3D12DescriptorHeap* rtvHeap = nullptr;
        ID3D12Resource* renderTargets[2] = { nullptr, nullptr };
        ID3D12CommandAllocator* commandAllocator = nullptr;
        ID3D12GraphicsCommandList* commandList = nullptr;
        ID3D12Resource* uploadBuffer = nullptr;
        ID3D12Fence* fence = nullptr;
        HANDLE fenceEvent = nullptr;
        UINT64 fenceValue = 0;
        UINT frameIndex = 0;
        UINT rtvDescriptorSize = 0;

        // The pixels to write to.
        uint8* Pixels = nullptr;
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        using namespace Internal;
        switch (msg)
        {
            case WM_DESTROY:
            case WM_CLOSE:
            {
                shouldQuit = true;
                return 0;
            }
            case WM_MOUSEMOVE:
            {
                int rawX = (int)(short)LOWORD(lParam);
                int rawY = (int)(short)HIWORD(lParam);

                if (isFullscreen)
                {
                    // In fullscreen, the buffer is stretched to fill the window
                    // Scale mouse coordinates from window space to buffer space
                    RECT clientRect;
                    GetClientRect(hwnd, &clientRect);
                    int windowWidth = clientRect.right - clientRect.left;
                    int windowHeight = clientRect.bottom - clientRect.top;

                    mouseX = (int)((float)rawX * (float)width / (float)windowWidth);
                    mouseY = (int)((float)rawY * (float)height / (float)windowHeight);
                }
                else
                {
                    mouseX = rawX;
                    mouseY = rawY;
                }
                return 0;
            }
            case WM_LBUTTONDOWN:
            {
                mouseButtons[0] = true;
                return 0;
            }
            case WM_LBUTTONUP:
            {
                mouseButtons[0] = false;
                return 0;
            }
            case WM_RBUTTONDOWN:
            {
                mouseButtons[1] = true;
                return 0;
            }
            case WM_RBUTTONUP:
            {
                mouseButtons[1] = false;
                return 0;
            }
            case WM_MBUTTONDOWN:
            {
                mouseButtons[2] = true;
                return 0;
            }
            case WM_MBUTTONUP:
            {
                mouseButtons[2] = false;
                return 0;
            }
            case WM_KEYDOWN:
            {
                if (wParam < 256)
                    keys[wParam] = true;
                return 0;
            }
            case WM_KEYUP:
            {
                if (wParam < 256)
                    keys[wParam] = false;
                return 0;
            }
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    uint8* Init(uint32 width, uint32 height, bool fullscreen)
    {
        using namespace Internal;
        Internal::width = width;
        Internal::height = height;
        Internal::Pixels = (uint8*)malloc(width * height * 4);

        // Create window
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"ThirteenWindowClass";
        RegisterClassExW(&wc);

        DWORD style = (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
        RECT rect = { 0, 0, (LONG)width, (LONG)height };
        AdjustWindowRect(&rect, style, FALSE);

        hwnd = CreateWindowExW(
            0,
            L"ThirteenWindowClass",
            L"Thirteen",
            style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr, nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (!hwnd)
            return nullptr;

        ShowWindow(hwnd, SW_SHOW);

        // Enable debug layer in debug builds
        #if DX12VALIDATION()
        ID3D12Debug* debugController = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable GPU-based validation
            ID3D12Debug1* debugController1 = nullptr;
            if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
            {
                debugController1->SetEnableGPUBasedValidation(TRUE);
                debugController1->Release();
            }

            debugController->Release();
        }
        #endif

        // Create device
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
            return nullptr;

        #if DX12VALIDATION()
        // Break on errors and corruption
        ID3D12InfoQueue* infoQueue = nullptr;
        if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
        {
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            infoQueue->Release();
        }
        #endif

        // Create command queue
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))))
            return nullptr;

        // Create swap chain
        IDXGIFactory4* factory = nullptr;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
            return nullptr;

        // Check for tearing support
        IDXGIFactory5* factory5 = nullptr;
        if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory5))))
        {
            BOOL allowTearing = FALSE;
            if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                tearingSupported = (allowTearing == TRUE);
            }
            factory5->Release();
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.Flags = tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        IDXGISwapChain1* swapChain1 = nullptr;
        HRESULT hr = factory->CreateSwapChainForHwnd(
            commandQueue,
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1
        );
        factory->Release();

        if (FAILED(hr))
            return nullptr;

        swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain));
        swapChain1->Release();
        frameIndex = swapChain->GetCurrentBackBufferIndex();

        // Create RTV descriptor heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))))
            return nullptr;

        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // Create RTVs
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < 2; i++)
        {
            if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]))))
                return nullptr;
            device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }

        // Create command allocator
        if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))))
            return nullptr;

        // Create command list
        if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList))))
            return nullptr;
        commandList->Close();

        // Create upload buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = width * height * 4;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if (FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer))))
            return nullptr;

        // Create synchronization objects
        if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
            return nullptr;

        fenceValue = 1;
        fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent)
            return nullptr;

        // Initialize frame timing
        QueryPerformanceFrequency(&perfFrequency);
        QueryPerformanceCounter(&lastFrameTime);

        if (fullscreen)
            SetFullscreen(true);

        return Internal::Pixels;
    }

    bool Render()
    {
        using namespace Internal;

        // Copy current input state to previous
        prevMouseX = mouseX;
        prevMouseY = mouseY;
        memcpy(prevMouseButtons, mouseButtons, sizeof(mouseButtons));
        memcpy(prevKeys, keys, sizeof(keys));

        // Calculate frame time
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);
        lastDeltaTime = (double)(currentTime.QuadPart - lastFrameTime.QuadPart) / (double)perfFrequency.QuadPart;
        lastFrameTime = currentTime;

        // Track frame times
        frameTimeSum += lastDeltaTime;
        frameCount++;

        // Calculate FPS when we've accumulated more than 1 second
        if (frameTimeSum >= 1.0)
        {
            averageFPS = (double)frameCount / frameTimeSum;
            frameTimeSum = 0.0;
            frameCount = 0;
        }

        // Update title bar every 0.25 seconds
        titleUpdateTimer += lastDeltaTime;
        if (titleUpdateTimer >= 0.25)
        {
            titleUpdateTimer = 0.0;
            char titleBuffer[256];
            sprintf_s(titleBuffer, "%s - %.1f FPS (%.1f ms)", appName.c_str(), averageFPS, 1000.0f / averageFPS);
            SetWindowTextA(hwnd, titleBuffer);
        }

        // Process messages
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (shouldQuit)
            return false;

        // Wait for previous frame
        const UINT64 currentFenceValue = fenceValue;
        commandQueue->Signal(fence, currentFenceValue);
        fenceValue++;

        if (fence->GetCompletedValue() < currentFenceValue)
        {
            fence->SetEventOnCompletion(currentFenceValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }

        frameIndex = swapChain->GetCurrentBackBufferIndex();

        // Upload pixel data
        void* mappedData = nullptr;
        D3D12_RANGE readRange = { 1, 0 };
        uploadBuffer->Map(0, &readRange, &mappedData);
        memcpy(mappedData, Internal::Pixels, width * height * 4);
        uploadBuffer->Unmap(0, nullptr);

        // Record commands
        commandAllocator->Reset();
        commandList->Reset(commandAllocator, nullptr);

        // Transition render target to copy dest
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = renderTargets[frameIndex];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &barrier);

        // Copy from upload buffer to render target
        D3D12_TEXTURE_COPY_LOCATION dst = {};
        dst.pResource = renderTargets[frameIndex];
        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION src = {};
        src.pResource = uploadBuffer;
        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        src.PlacedFootprint.Footprint.Width = width;
        src.PlacedFootprint.Footprint.Height = height;
        src.PlacedFootprint.Footprint.Depth = 1;
        src.PlacedFootprint.Footprint.RowPitch = width * 4;

        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // Transition render target to present
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        commandList->ResourceBarrier(1, &barrier);

        commandList->Close();

        // Execute command list
        ID3D12CommandList* cmdLists[] = { commandList };
        commandQueue->ExecuteCommandLists(1, cmdLists);

        // Present
        UINT syncInterval = vsyncEnabled ? 1 : 0;
        UINT presentFlags = (!vsyncEnabled && tearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;
        swapChain->Present(syncInterval, presentFlags);

        return !shouldQuit;
    }

    void SetVSync(bool enabled)
    {
        Internal::vsyncEnabled = enabled;
    }

    bool GetVSync()
    {
        return Internal::vsyncEnabled;
    }

    void SetApplicationName(const char* name)
    {
        Internal::appName = name;
    }

    void SetFullscreen(bool fullscreen)
    {
        using namespace Internal;
        if (isFullscreen == fullscreen)
            return;

        isFullscreen = fullscreen;

        if (fullscreen)
        {
            // Switch to fullscreen (borderless window)
            SetWindowLongW(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

            // Get monitor dimensions
            HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(mi) };
            GetMonitorInfo(hMonitor, &mi);

            SetWindowPos(hwnd, HWND_TOP,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom - mi.rcMonitor.top,
                SWP_FRAMECHANGED);
        }
        else
        {
            // Switch to windowed mode
            DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
            SetWindowLongW(hwnd, GWL_STYLE, style | WS_VISIBLE);

            RECT rect = { 0, 0, (LONG)width, (LONG)height };
            AdjustWindowRect(&rect, style, FALSE);

            // Center window on screen
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int windowWidth = rect.right - rect.left;
            int windowHeight = rect.bottom - rect.top;
            int x = (screenWidth - windowWidth) / 2;
            int y = (screenHeight - windowHeight) / 2;

            SetWindowPos(hwnd, HWND_TOP, x, y, windowWidth, windowHeight, SWP_FRAMECHANGED);
        }
    }

    bool GetFullscreen()
    {
        return Internal::isFullscreen;
    }

    uint32 GetWidth()
    {
        return Internal::width;
    }

    uint32 GetHeight()
    {
        return Internal::height;
    }

    uint8* SetSize(uint32 width, uint32 height)
    {
        using namespace Internal;

        if (width == Internal::width && height == Internal::height)
            return Pixels;

        // Wait for GPU to finish
        if (fence && commandQueue)
        {
            const UINT64 currentFenceValue = fenceValue;
            commandQueue->Signal(fence, currentFenceValue);
            fenceValue++;

            if (fence->GetCompletedValue() < currentFenceValue)
            {
                fence->SetEventOnCompletion(currentFenceValue, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }
        }

        // Release old render targets
        if (renderTargets[0])
        {
            renderTargets[0]->Release();
            renderTargets[0] = nullptr;
        }
        if (renderTargets[1])
        {
            renderTargets[1]->Release();
            renderTargets[1] = nullptr;
        }

        // Release old upload buffer
        if (uploadBuffer)
        {
            uploadBuffer->Release();
            uploadBuffer = nullptr;
        }

        // Reallocate pixel buffer
        Pixels = (uint8*)realloc(Pixels, width * height * 4);
        if (!Pixels)
            return nullptr;

        // Update dimensions
        Internal::width = width;
        Internal::height = height;

        // Resize swap chain buffers
        HRESULT hr = swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
            tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
        if (FAILED(hr))
            return nullptr;

        // Recreate render target views
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < 2; i++)
        {
            if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]))))
                return nullptr;
            device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }

        // Recreate upload buffer
        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = width * height * 4;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        if (FAILED(device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer))))
            return nullptr;

        frameIndex = swapChain->GetCurrentBackBufferIndex();

        // Resize window if not fullscreen
        if (!isFullscreen)
        {
            DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
            RECT rect = { 0, 0, (LONG)width, (LONG)height };
            AdjustWindowRect(&rect, style, FALSE);

            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            int windowWidth = rect.right - rect.left;
            int windowHeight = rect.bottom - rect.top;
            int x = (screenWidth - windowWidth) / 2;
            int y = (screenHeight - windowHeight) / 2;

            SetWindowPos(hwnd, HWND_TOP, x, y, windowWidth, windowHeight, SWP_FRAMECHANGED);
        }

        return Pixels;
    }

    double GetDeltaTime()
    {
        return Internal::lastDeltaTime;
    }

    // Input query functions
    void GetMousePosition(int& x, int& y)
    {
        x = Internal::mouseX;
        y = Internal::mouseY;
    }

    void GetMousePositionLastFrame(int& x, int& y)
    {
        x = Internal::prevMouseX;
        y = Internal::prevMouseY;
    }

    bool GetMouseButton(int button)
    {
        if (button >= 0 && button < 3)
            return Internal::mouseButtons[button];
        return false;
    }

    bool GetMouseButtonLastFrame(int button)
    {
        if (button >= 0 && button < 3)
            return Internal::prevMouseButtons[button];
        return false;
    }

    bool GetKey(int keyCode)
    {
        if (keyCode >= 0 && keyCode < 256)
            return Internal::keys[keyCode];
        return false;
    }

    bool GetKeyLastFrame(int keyCode)
    {
        if (keyCode >= 0 && keyCode < 256)
            return Internal::prevKeys[keyCode];
        return false;
    }

    void Shutdown()
    {
        using namespace Internal;

        // Wait for GPU to finish
        if (fence && commandQueue)
        {
            const UINT64 currentFenceValue = fenceValue;
            commandQueue->Signal(fence, currentFenceValue);
            fenceValue++;

            if (fence->GetCompletedValue() < currentFenceValue)
            {
                fence->SetEventOnCompletion(currentFenceValue, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }
        }

        // Release resources
        if (fenceEvent)
            CloseHandle(fenceEvent);
        if (fence)
            fence->Release();
        if (uploadBuffer)
            uploadBuffer->Release();
        if (commandList)
            commandList->Release();
        if (commandAllocator)
            commandAllocator->Release();
        if (renderTargets[0])
            renderTargets[0]->Release();
        if (renderTargets[1])
            renderTargets[1]->Release();
        if (rtvHeap)
            rtvHeap->Release();
        if (swapChain)
            swapChain->Release();
        if (commandQueue)
            commandQueue->Release();
        if (device)
            device->Release();

        if (hwnd)
        {
            DestroyWindow(hwnd);
            UnregisterClassW(L"ThirteenWindowClass", GetModuleHandle(nullptr));
        }

        free(Internal::Pixels);
        Internal::Pixels = nullptr;
    }
}
