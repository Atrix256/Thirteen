/*
Thirteen v1.0.0
by Alan Wolfe
MIT licensed https://github.com/Atrix256/Thirteen

Thirteen is a header-only C++ library that initializes a DirectX 12 window and gives you a pointer to RGBA uint8 pixels to write to, which are copied to the screen every time you call Render().

It is inspired by the simplicity of the Mode 13h days where you initialized the graphics mode and then started writing pixels directly to the screen. Just include the header, initialize, and start drawing!
*/

#pragma once

#define DX12VALIDATION() (_DEBUG && false)

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <d3d12.h>
    #include <dxgi1_6.h>
#elif defined(__APPLE__)
    #include <objc/objc.h>
    #include <objc/runtime.h>
    #include <objc/message.h>
    #include <CoreGraphics/CoreGraphics.h>
    #include <TargetConditionals.h>
#endif

#include <cstdlib>
#include <cstring>
#include <chrono>
#include <cstdio>
#include <string>

#if defined(_WIN32)
    #pragma comment(lib, "d3d12.lib")
    #pragma comment(lib, "dxgi.lib")
#endif

#if !defined(_WIN32)
    #ifndef VK_ESCAPE
        #define VK_ESCAPE 0x1B
    #endif
    #ifndef VK_SPACE
        #define VK_SPACE 0x20
    #endif
#endif

#if defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
    #define THIRTEEN_PLATFORM_MACOS 1
#else
    #define THIRTEEN_PLATFORM_MACOS 0
#endif

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

    #if defined(_WIN32)
    // Internal window procedure.
    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    #endif

    #if defined(_WIN32)
        using NativeWindowHandle = HWND;
    #else
        using NativeWindowHandle = void*;
    #endif

    // ========================================

    // Internal state
    namespace Internal
    {
        inline double NowSeconds()
        {
            using clock = std::chrono::steady_clock;
            return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
        }

        uint32 width = 320;
        uint32 height = 200;
        bool shouldQuit = false;
        bool vsyncEnabled = true;
        bool isFullscreen = false;
        std::string appName = "ThirteenApp";

        // Frame timing
        double lastFrameTime = 0.0;
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

        // The pixels to write to.
        uint8* Pixels = nullptr;

        #if defined(_WIN32)
        struct PlatformWin32
        {
            HWND hwnd = nullptr;
            bool ownsClassRegistration = false;
            static constexpr const wchar_t* c_windowClassName = L"ThirteenWindowClass";

            bool InitWindow(uint32 width, uint32 height)
            {
                WNDCLASSEXW wc = {};
                wc.cbSize = sizeof(WNDCLASSEXW);
                wc.style = CS_HREDRAW | CS_VREDRAW;
                wc.lpfnWndProc = Thirteen::WndProc;
                wc.hInstance = GetModuleHandle(nullptr);
                wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
                wc.lpszClassName = c_windowClassName;
                if (RegisterClassExW(&wc))
                {
                    ownsClassRegistration = true;
                }
                else if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
                {
                    ownsClassRegistration = false;
                }
                else
                {
                    return false;
                }

                DWORD style = (WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX));
                RECT rect = { 0, 0, (LONG)width, (LONG)height };
                AdjustWindowRect(&rect, style, FALSE);

                hwnd = CreateWindowExW(
                    0,
                    c_windowClassName,
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
                    return false;

                ShowWindow(hwnd, SW_SHOW);
                return true;
            }

            void PumpMessages()
            {
                MSG msg;
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }

            void SetTitle(const char* title)
            {
                if (hwnd)
                    SetWindowTextA(hwnd, title);
            }

            void SetFullscreen(bool fullscreen, uint32 width, uint32 height)
            {
                if (!hwnd)
                    return;

                if (fullscreen)
                {
                    SetWindowLongW(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

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
                    DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
                    SetWindowLongW(hwnd, GWL_STYLE, style | WS_VISIBLE);

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
            }

            void ResizeWindow(uint32 width, uint32 height, bool isFullscreen)
            {
                if (!hwnd || isFullscreen)
                    return;

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

            NativeWindowHandle GetWindowHandle() const
            {
                return hwnd;
            }

            void ShutdownWindow()
            {
                if (hwnd)
                {
                    DestroyWindow(hwnd);
                    hwnd = nullptr;
                }
                if (ownsClassRegistration)
                {
                    UnregisterClassW(c_windowClassName, GetModuleHandle(nullptr));
                    ownsClassRegistration = false;
                }
            }
        };

        struct RendererD3D12
        {
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
            bool tearingSupported = false;

            void WaitForGpu()
            {
                if (!fence || !commandQueue)
                    return;

                const UINT64 currentFenceValue = fenceValue;
                commandQueue->Signal(fence, currentFenceValue);
                fenceValue++;

                if (fence->GetCompletedValue() < currentFenceValue)
                {
                    fence->SetEventOnCompletion(currentFenceValue, fenceEvent);
                    WaitForSingleObject(fenceEvent, INFINITE);
                }
            }

            void ReleaseRenderTargets()
            {
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
            }

            bool CreateUploadBuffer(uint32 width, uint32 height)
            {
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

                return SUCCEEDED(device->CreateCommittedResource(
                    &heapProps,
                    D3D12_HEAP_FLAG_NONE,
                    &bufferDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_PPV_ARGS(&uploadBuffer)));
            }

            bool Init(NativeWindowHandle hwnd, uint32 width, uint32 height)
            {
                #if DX12VALIDATION()
                ID3D12Debug* debugController = nullptr;
                if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
                {
                    debugController->EnableDebugLayer();

                    ID3D12Debug1* debugController1 = nullptr;
                    if (SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1))))
                    {
                        debugController1->SetEnableGPUBasedValidation(TRUE);
                        debugController1->Release();
                    }

                    debugController->Release();
                }
                #endif

                if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
                    return false;

                #if DX12VALIDATION()
                ID3D12InfoQueue* infoQueue = nullptr;
                if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
                {
                    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
                    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
                    infoQueue->Release();
                }
                #endif

                D3D12_COMMAND_QUEUE_DESC queueDesc = {};
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
                queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
                if (FAILED(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))))
                    return false;

                IDXGIFactory4* factory = nullptr;
                if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
                    return false;

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
                    return false;

                swapChain1->QueryInterface(IID_PPV_ARGS(&swapChain));
                swapChain1->Release();
                frameIndex = swapChain->GetCurrentBackBufferIndex();

                D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
                rtvHeapDesc.NumDescriptors = 2;
                rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
                rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))))
                    return false;

                rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
                for (UINT i = 0; i < 2; i++)
                {
                    if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]))))
                        return false;
                    device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
                    rtvHandle.ptr += rtvDescriptorSize;
                }

                if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator))))
                    return false;

                if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList))))
                    return false;
                commandList->Close();

                if (!CreateUploadBuffer(width, height))
                    return false;

                if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))))
                    return false;

                fenceValue = 1;
                fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
                return fenceEvent != nullptr;
            }

            bool Render(const uint8* pixels, uint32 width, uint32 height, bool vsyncEnabled)
            {
                WaitForGpu();
                frameIndex = swapChain->GetCurrentBackBufferIndex();

                void* mappedData = nullptr;
                D3D12_RANGE readRange = { 1, 0 };
                uploadBuffer->Map(0, &readRange, &mappedData);
                memcpy(mappedData, pixels, width * height * 4);
                uploadBuffer->Unmap(0, nullptr);

                commandAllocator->Reset();
                commandList->Reset(commandAllocator, nullptr);

                D3D12_RESOURCE_BARRIER barrier = {};
                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                barrier.Transition.pResource = renderTargets[frameIndex];
                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                commandList->ResourceBarrier(1, &barrier);

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

                barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
                barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
                commandList->ResourceBarrier(1, &barrier);

                commandList->Close();

                ID3D12CommandList* cmdLists[] = { commandList };
                commandQueue->ExecuteCommandLists(1, cmdLists);

                UINT syncInterval = vsyncEnabled ? 1 : 0;
                UINT presentFlags = (!vsyncEnabled && tearingSupported) ? DXGI_PRESENT_ALLOW_TEARING : 0;
                return SUCCEEDED(swapChain->Present(syncInterval, presentFlags));
            }

            bool Resize(uint32 width, uint32 height)
            {
                WaitForGpu();

                ReleaseRenderTargets();

                if (uploadBuffer)
                {
                    uploadBuffer->Release();
                    uploadBuffer = nullptr;
                }

                HRESULT hr = swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM,
                    tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
                if (FAILED(hr))
                    return false;

                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
                for (UINT i = 0; i < 2; i++)
                {
                    if (FAILED(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]))))
                        return false;
                    device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
                    rtvHandle.ptr += rtvDescriptorSize;
                }

                if (!CreateUploadBuffer(width, height))
                    return false;

                frameIndex = swapChain->GetCurrentBackBufferIndex();
                return true;
            }

            void Shutdown()
            {
                WaitForGpu();

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
                ReleaseRenderTargets();
                if (rtvHeap)
                    rtvHeap->Release();
                if (swapChain)
                    swapChain->Release();
                if (commandQueue)
                    commandQueue->Release();
                if (device)
                    device->Release();
            }
        };
        #elif THIRTEEN_PLATFORM_MACOS
        extern "C" void* MTLCreateSystemDefaultDevice(void);

        using NSUInteger = unsigned long;
        using NSInteger = long;

        struct MTLSize        {
            NSUInteger width;
            NSUInteger height;
            NSUInteger depth;
        };

        struct MTLOrigin        {
            NSUInteger x;
            NSUInteger y;
            NSUInteger z;
        };

        inline SEL Sel(const char* name)
        {
            return sel_registerName(name);
        }

        // NSEventType values from AppKit's NSEvent.h.
        constexpr NSInteger NSEventTypeLeftMouseDown = 1;
        constexpr NSInteger NSEventTypeLeftMouseUp = 2;
        constexpr NSInteger NSEventTypeRightMouseDown = 3;
        constexpr NSInteger NSEventTypeRightMouseUp = 4;
        constexpr NSInteger NSEventTypeMouseMoved = 5;
        constexpr NSInteger NSEventTypeLeftMouseDragged = 6;
        constexpr NSInteger NSEventTypeRightMouseDragged = 7;
        constexpr NSInteger NSEventTypeKeyDown = 10;
        constexpr NSInteger NSEventTypeKeyUp = 11;
        constexpr NSInteger NSEventTypeOtherMouseDown = 25;
        constexpr NSInteger NSEventTypeOtherMouseUp = 26;
        constexpr NSInteger NSEventTypeOtherMouseDragged = 27;

        struct PlatformMetal
        {
            id app = nullptr;
            id window = nullptr;
            id contentView = nullptr;

            bool InitWindow(uint32 width, uint32 height)
            {
                id nsApplicationClass = (id)objc_getClass("NSApplication");
                app = ((id(*)(id, SEL))objc_msgSend)(nsApplicationClass, Sel("sharedApplication"));
                if (!app)
                    return false;

                ((void(*)(id, SEL, NSInteger))objc_msgSend)(app, Sel("setActivationPolicy:"), 0);

                id nsWindowClass = (id)objc_getClass("NSWindow");
                id windowAlloc = ((id(*)(id, SEL))objc_msgSend)(nsWindowClass, Sel("alloc"));
                if (!windowAlloc)
                    return false;

                const NSUInteger styleMask = (1ull << 0) | (1ull << 1) | (1ull << 2);
                const NSUInteger backingStoreBuffered = 2;
                CGRect frame = CGRectMake(100.0, 100.0, (double)width, (double)height);
                window = ((id(*)(id, SEL, CGRect, NSUInteger, NSUInteger, bool))objc_msgSend)(
                    windowAlloc,
                    Sel("initWithContentRect:styleMask:backing:defer:"),
                    frame,
                    styleMask,
                    backingStoreBuffered,
                    false
                );
                if (!window)
                    return false;

                ((void(*)(id, SEL, bool))objc_msgSend)(window, Sel("setReleasedWhenClosed:"), false);
                contentView = ((id(*)(id, SEL))objc_msgSend)(window, Sel("contentView"));
                if (!contentView)
                    return false;

                ((void(*)(id, SEL, id))objc_msgSend)(window, Sel("makeKeyAndOrderFront:"), nullptr);
                ((void(*)(id, SEL, bool))objc_msgSend)(app, Sel("activateIgnoringOtherApps:"), true);
                return true;
            }

            void PumpMessages()
            {
                if (!app)
                    return;

                id dateClass = (id)objc_getClass("NSDate");
                id distantPast = ((id(*)(id, SEL))objc_msgSend)(dateClass, Sel("distantPast"));
                id nsStringClass = (id)objc_getClass("NSString");
                id defaultMode = ((id(*)(id, SEL, const char*))objc_msgSend)(nsStringClass, Sel("stringWithUTF8String:"), "kCFRunLoopDefaultMode");

                const unsigned long long anyMask = ~0ull;
                while (true)
                {
                    id event = ((id(*)(id, SEL, unsigned long long, id, id, bool))objc_msgSend)(
                        app,
                        Sel("nextEventMatchingMask:untilDate:inMode:dequeue:"),
                        anyMask,
                        distantPast,
                        defaultMode,
                        true
                    );

                    if (!event)
                        break;

                    NSInteger eventType = ((NSInteger(*)(id, SEL))objc_msgSend)(event, Sel("type"));
                    const bool isMouseDownEvent =
                        eventType == NSEventTypeLeftMouseDown ||
                        eventType == NSEventTypeRightMouseDown ||
                        eventType == NSEventTypeOtherMouseDown;
                    const bool isMouseUpEvent =
                        eventType == NSEventTypeLeftMouseUp ||
                        eventType == NSEventTypeRightMouseUp ||
                        eventType == NSEventTypeOtherMouseUp;
                    const bool isMousePositionEvent =
                        eventType == NSEventTypeMouseMoved ||
                        eventType == NSEventTypeLeftMouseDragged ||
                        eventType == NSEventTypeRightMouseDragged ||
                        eventType == NSEventTypeOtherMouseDragged ||
                        isMouseDownEvent ||
                        isMouseUpEvent;

                    if (eventType == NSEventTypeKeyDown)
                    {
                        id chars = ((id(*)(id, SEL))objc_msgSend)(event, Sel("charactersIgnoringModifiers"));
                        if (chars)
                        {
                            const char* utf8 = ((const char*(*)(id, SEL))objc_msgSend)(chars, Sel("UTF8String"));
                            if (utf8 && utf8[0] != '\0')
                                keys[(unsigned char)utf8[0]] = true;
                        }
                    }
                    else if (eventType == NSEventTypeKeyUp)
                    {
                        id chars = ((id(*)(id, SEL))objc_msgSend)(event, Sel("charactersIgnoringModifiers"));
                        if (chars)
                        {
                            const char* utf8 = ((const char*(*)(id, SEL))objc_msgSend)(chars, Sel("UTF8String"));
                            if (utf8 && utf8[0] != '\0')
                                keys[(unsigned char)utf8[0]] = false;
                        }
                    }
                    else if (isMouseDownEvent)
                    {
                        NSInteger buttonNumber = ((NSInteger(*)(id, SEL))objc_msgSend)(event, Sel("buttonNumber"));
                        if (buttonNumber >= 0 && buttonNumber < 3)
                            mouseButtons[buttonNumber] = true;
                    }
                    else if (isMouseUpEvent)
                    {
                        NSInteger buttonNumber = ((NSInteger(*)(id, SEL))objc_msgSend)(event, Sel("buttonNumber"));
                        if (buttonNumber >= 0 && buttonNumber < 3)
                            mouseButtons[buttonNumber] = false;
                    }
                    else if (isMousePositionEvent)
                    {
                        CGPoint p = ((CGPoint(*)(id, SEL))objc_msgSend)(event, Sel("locationInWindow"));
                        mouseX = (int)p.x;
                        mouseY = (int)(height - p.y);
                    }

                    ((void(*)(id, SEL, id))objc_msgSend)(app, Sel("sendEvent:"), event);
                }

                if (window)
                {
                    bool visible = ((bool(*)(id, SEL))objc_msgSend)(window, Sel("isVisible"));
                    if (!visible)
                        shouldQuit = true;
                }
            }

            void SetTitle(const char* title)
            {
                if (!window)
                    return;
                id nsStringClass = (id)objc_getClass("NSString");
                id nsTitle = ((id(*)(id, SEL, const char*))objc_msgSend)(nsStringClass, Sel("stringWithUTF8String:"), title);
                ((void(*)(id, SEL, id))objc_msgSend)(window, Sel("setTitle:"), nsTitle);
            }

            void SetFullscreen(bool, uint32, uint32)
            {
                if (window)
                    ((void(*)(id, SEL, id))objc_msgSend)(window, Sel("toggleFullScreen:"), nullptr);
            }

            void ResizeWindow(uint32 width, uint32 height, bool isFullscreen)
            {
                if (!window || isFullscreen)
                    return;
                CGSize contentSize = CGSizeMake((double)width, (double)height);
                ((void(*)(id, SEL, CGSize))objc_msgSend)(window, Sel("setContentSize:"), contentSize);
            }

            NativeWindowHandle GetWindowHandle() const
            {
                return contentView;
            }

            void ShutdownWindow()
            {
                if (window)
                {
                    ((void(*)(id, SEL))objc_msgSend)(window, Sel("close"));
                    window = nullptr;
                }
                contentView = nullptr;
                app = nullptr;
            }
        };

        struct RendererMetal
        {
            id device = nullptr;
            id commandQueue = nullptr;
            id metalLayer = nullptr;
            id uploadBuffer = nullptr;
            NativeWindowHandle hostView = nullptr;
            uint32 bufferWidth = 0;
            uint32 bufferHeight = 0;
            size_t uploadSize = 0;

            bool EnsureUploadBuffer(uint32 width, uint32 height)
            {
                const size_t requiredSize = (size_t)width * (size_t)height * 4u;
                if (uploadBuffer && requiredSize == uploadSize)
                    return true;

                if (uploadBuffer)
                {
                    ((void(*)(id, SEL))objc_msgSend)(uploadBuffer, Sel("release"));
                    uploadBuffer = nullptr;
                }

                // MTLResourceStorageModeShared = 0
                uploadBuffer = ((id(*)(id, SEL, NSUInteger, NSUInteger))objc_msgSend)(
                    device,
                    Sel("newBufferWithLength:options:"),
                    (NSUInteger)requiredSize,
                    (NSUInteger)0
                );
                if (!uploadBuffer)
                    return false;

                uploadSize = requiredSize;
                return true;
            }

            bool Init(NativeWindowHandle hwnd, uint32 width, uint32 height)
            {
                hostView = hwnd;
                device = (id)MTLCreateSystemDefaultDevice();
                if (!device)
                    return false;

                commandQueue = ((id(*)(id, SEL))objc_msgSend)(device, Sel("newCommandQueue"));
                if (!commandQueue)
                    return false;

                id layerClass = (id)objc_getClass("CAMetalLayer");
                metalLayer = ((id(*)(id, SEL))objc_msgSend)(layerClass, Sel("layer"));
                if (!metalLayer)
                    return false;

                ((void(*)(id, SEL, id))objc_msgSend)(metalLayer, Sel("setDevice:"), device);
                // MTLPixelFormatRGBA8Unorm = 70
                ((void(*)(id, SEL, NSUInteger))objc_msgSend)(metalLayer, Sel("setPixelFormat:"), (NSUInteger)70);
                ((void(*)(id, SEL, bool))objc_msgSend)(metalLayer, Sel("setFramebufferOnly:"), false);

                CGRect layerFrame = CGRectMake(0.0, 0.0, (double)width, (double)height);
                ((void(*)(id, SEL, CGRect))objc_msgSend)(metalLayer, Sel("setFrame:"), layerFrame);

                if (hostView)
                {
                    id view = (id)hostView;
                    ((void(*)(id, SEL, bool))objc_msgSend)(view, Sel("setWantsLayer:"), true);
                    ((void(*)(id, SEL, id))objc_msgSend)(view, Sel("setLayer:"), metalLayer);
                }

                bufferWidth = width;
                bufferHeight = height;
                return EnsureUploadBuffer(width, height);
            }

            bool Render(const uint8* pixels, uint32 width, uint32 height, bool)
            {
                if (!metalLayer || !commandQueue || !EnsureUploadBuffer(width, height))
                    return false;

                id drawable = ((id(*)(id, SEL))objc_msgSend)(metalLayer, Sel("nextDrawable"));
                if (!drawable)
                    return true;

                void* mapped = ((void*(*)(id, SEL))objc_msgSend)(uploadBuffer, Sel("contents"));
                if (!mapped)
                    return false;
                memcpy(mapped, pixels, (size_t)width * (size_t)height * 4u);

                id texture = ((id(*)(id, SEL))objc_msgSend)(drawable, Sel("texture"));
                id commandBuffer = ((id(*)(id, SEL))objc_msgSend)(commandQueue, Sel("commandBuffer"));
                if (!texture || !commandBuffer)
                    return false;

                id blit = ((id(*)(id, SEL))objc_msgSend)(commandBuffer, Sel("blitCommandEncoder"));
                if (!blit)
                    return false;

                MTLSize sourceSize = { (NSUInteger)width, (NSUInteger)height, (NSUInteger)1 };
                MTLOrigin destOrigin = { (NSUInteger)0, (NSUInteger)0, (NSUInteger)0 };
                ((void(*)(id, SEL, id, NSUInteger, NSUInteger, NSUInteger, MTLSize, id, NSUInteger, NSUInteger, MTLOrigin))objc_msgSend)(
                    blit,
                    Sel("copyFromBuffer:sourceOffset:sourceBytesPerRow:sourceBytesPerImage:sourceSize:toTexture:destinationSlice:destinationLevel:destinationOrigin:"),
                    uploadBuffer,
                    (NSUInteger)0,
                    (NSUInteger)(width * 4),
                    (NSUInteger)(width * height * 4),
                    sourceSize,
                    texture,
                    (NSUInteger)0,
                    (NSUInteger)0,
                    destOrigin
                );

                ((void(*)(id, SEL))objc_msgSend)(blit, Sel("endEncoding"));
                ((void(*)(id, SEL, id))objc_msgSend)(commandBuffer, Sel("presentDrawable:"), drawable);
                ((void(*)(id, SEL))objc_msgSend)(commandBuffer, Sel("commit"));
                return true;
            }

            bool Resize(uint32 width, uint32 height)
            {
                bufferWidth = width;
                bufferHeight = height;
                if (metalLayer)
                {
                    CGRect layerFrame = CGRectMake(0.0, 0.0, (double)width, (double)height);
                    ((void(*)(id, SEL, CGRect))objc_msgSend)(metalLayer, Sel("setFrame:"), layerFrame);
                }
                return EnsureUploadBuffer(width, height);
            }

            void Shutdown()
            {
                if (uploadBuffer)
                {
                    ((void(*)(id, SEL))objc_msgSend)(uploadBuffer, Sel("release"));
                    uploadBuffer = nullptr;
                }
                if (commandQueue)
                {
                    ((void(*)(id, SEL))objc_msgSend)(commandQueue, Sel("release"));
                    commandQueue = nullptr;
                }
                device = nullptr;
                metalLayer = nullptr;
                hostView = nullptr;
                bufferWidth = 0;
                bufferHeight = 0;
                uploadSize = 0;
            }
        };
        #else
        struct PlatformStub
        {
            bool InitWindow(uint32, uint32) { return true; }
            void PumpMessages() {}
            void SetTitle(const char*) {}
            void SetFullscreen(bool, uint32, uint32) {}
            void ResizeWindow(uint32, uint32, bool) {}
            NativeWindowHandle GetWindowHandle() const { return nullptr; }
            void ShutdownWindow() {}
        };

        struct RendererStub
        {
            bool Init(NativeWindowHandle, uint32, uint32) { return false; }
            bool Render(const uint8*, uint32, uint32, bool) { return false; }
            bool Resize(uint32, uint32) { return false; }
            void Shutdown() {}
        };
        #endif

        struct BackendTraits
        {
            #if defined(_WIN32)
            using Platform = PlatformWin32;
            using Renderer = RendererD3D12;
            #elif THIRTEEN_PLATFORM_MACOS
            using Platform = PlatformMetal;
            using Renderer = RendererMetal;
            #else
            using Platform = PlatformStub;
            using Renderer = RendererStub;
            #endif
        };

        using PlatformBackend = BackendTraits::Platform;
        using RendererBackend = BackendTraits::Renderer;

        PlatformBackend* platform = nullptr;
        RendererBackend* renderer = nullptr;
    }

    #if defined(_WIN32)
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
    #endif

    uint8* Init(uint32 width, uint32 height, bool fullscreen)
    {
        using namespace Internal;
        Internal::width = width;
        Internal::height = height;
        Internal::Pixels = (uint8*)malloc(width * height * 4);
        if (!Internal::Pixels)
            return nullptr;

        platform = new PlatformBackend();
        if (!platform->InitWindow(width, height))
        {
            delete platform;
            platform = nullptr;
            free(Internal::Pixels);
            Internal::Pixels = nullptr;
            return nullptr;
        }

        renderer = new RendererBackend();
        if (!renderer->Init(platform->GetWindowHandle(), width, height))
        {
            renderer->Shutdown();
            delete renderer;
            renderer = nullptr;
            platform->ShutdownWindow();
            delete platform;
            platform = nullptr;
            free(Internal::Pixels);
            Internal::Pixels = nullptr;
            return nullptr;
        }

        // Initialize frame timing
        lastFrameTime = NowSeconds();

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
        double currentTime = NowSeconds();
        lastDeltaTime = currentTime - lastFrameTime;
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
            std::snprintf(titleBuffer, sizeof(titleBuffer), "%s - %.1f FPS (%.1f ms)", appName.c_str(), averageFPS, 1000.0f / averageFPS);
            if (platform)
                platform->SetTitle(titleBuffer);
        }

        // Process messages
        if (platform)
            platform->PumpMessages();

        if (shouldQuit)
            return false;
        if (!renderer)
            return false;
        renderer->Render(Internal::Pixels, width, height, vsyncEnabled);
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
        if (platform)
            platform->SetFullscreen(fullscreen, width, height);
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

        // Reallocate pixel buffer
        Pixels = (uint8*)realloc(Pixels, width * height * 4);
        if (!Pixels)
            return nullptr;

        // Update dimensions
        Internal::width = width;
        Internal::height = height;
        if (!renderer || !renderer->Resize(width, height))
            return nullptr;

        if (platform)
            platform->ResizeWindow(width, height, isFullscreen);

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
        if (renderer)
        {
            renderer->Shutdown();
            delete renderer;
            renderer = nullptr;
        }

        if (platform)
        {
            platform->ShutdownWindow();
            delete platform;
            platform = nullptr;
        }

        free(Internal::Pixels);
        Internal::Pixels = nullptr;
    }
}
