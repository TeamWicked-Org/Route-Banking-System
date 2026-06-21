// =============================================================================
// main.cpp  --  Banking System entry point
// D3D11 + ImGui window, no injection or security logic.
// =============================================================================
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dwmapi.lib")

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <dwmapi.h>

#include "include/models/banking_gui.h"
#include "include/models/Client.h"
#include "include/models/Employee.h"
#define STB_IMAGE_IMPLEMENTATION
#include "external/imgLib/stb_image.h"
#include "include/images/images.h"


// ImGui Win32 message handler (defined inside imgui_impl_win32.cpp)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// =============================================================================
// D3D11 globals
// =============================================================================
static ID3D11Device* g_dev = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static IDXGISwapChain* g_sc = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static HWND                    g_hwnd = nullptr;


static constexpr int kW = 1080;
static constexpr int kH = 720;


static ID3D11ShaderResourceView* g_bgTexture = nullptr;
static int g_winW = kW;
static int g_winH = kH;
static constexpr float kClear[4] = { 0.08f, 0.10f, 0.14f, 1.f }; // move out of WinMain

// ── New helper: renders one complete frame ───────────────────────────────────
static void render_frame()
{
    if (!g_rtv || !g_bgTexture) return;
    g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
    g_ctx->ClearRenderTargetView(g_rtv, kClear);
    banking_gui::tick(g_bgTexture, g_winW, g_winH);
    g_sc->Present(1, 0);
}

static bool make_rtv()
{
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(g_sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb)
        return false;
    HRESULT hr = g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
    bb->Release();
    return SUCCEEDED(hr) && g_rtv;
}

static bool init_d3d11()
{
    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = kW;
    sd.BufferDesc.Height = kH;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate = { 180, 1 };     // FPS Controller
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D_FEATURE_LEVEL fl{};
    return SUCCEEDED(D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &sd, &g_sc, &g_dev, &fl, &g_ctx)) && make_rtv();
}

static void free_d3d11()
{
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_sc) { g_sc->Release();  g_sc = nullptr; }
    if (g_dev) { g_dev->Release(); g_dev = nullptr; }
}

// =============================================================================
// Window procedure
// =============================================================================
static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM w, LPARAM l)
{
    if (ImGui_ImplWin32_WndProcHandler(h, msg, w, l)) return 1;

    switch (msg) {
    case WM_SIZE:
        if (w != SIZE_MINIMIZED && g_dev && g_sc) {
            g_winW = LOWORD(l);
            g_winH = HIWORD(l);

            if (g_ctx) {
                g_ctx->OMSetRenderTargets(0, nullptr, nullptr);
                g_ctx->Flush();
            }
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }

            HRESULT hr = g_sc->ResizeBuffers(
                0, g_winW, g_winH, DXGI_FORMAT_UNKNOWN, 0);
            if (SUCCEEDED(hr)) {
                make_rtv();
                render_frame(); // ← renders immediately, no black frame
            }
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}

// =============================================================================
// WinMain
// =============================================================================
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE,
    _In_ LPSTR, _In_ int)
{
    // Register window class
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"BankingSystemWnd";
    RegisterClassExW(&wc);

    // Centre on screen
    const int sx = GetSystemMetrics(SM_CXSCREEN);
    const int sy = GetSystemMetrics(SM_CYSCREEN);
    RECT wr = { 0, 0, kW, kH };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    const int ww = wr.right - wr.left;
    const int wh = wr.bottom - wr.top;

    g_hwnd = CreateWindowExW(
        0, L"BankingSystemWnd", L"Banking System",
        WS_OVERLAPPEDWINDOW,
        (sx - ww) / 2, (sy - wh) / 2, ww, wh,
        nullptr, nullptr, hInst, nullptr);

    if (!init_d3d11()) {
        MessageBoxW(nullptr, L"D3D11 initialisation failed.", L"Error",
            MB_OK | MB_ICONERROR);
        return 1;
    }

    // Rounded window corners on Windows 11+
    DWORD corner = 0;
    DwmSetWindowAttribute(g_hwnd, 33, &corner, sizeof(corner));


    banking_gui::init(g_hwnd, g_dev, g_ctx);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    unsigned char* rgba_bgImg_data = stbi_load_from_memory(kImg_Image01, sizeof(kImg_Image01), &image_width, &image_height, &channels, 4);

    ID3D11ShaderResourceView* myTexture = nullptr;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = rgba_bgImg_data;

    subResource.SysMemPitch = image_width * 4;

    ID3D11Texture2D* pTexture = nullptr;

    HRESULT hr = g_dev->CreateTexture2D(&desc, &subResource, &pTexture);

    if (SUCCEEDED(hr))
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;

        hr = g_dev->CreateShaderResourceView(pTexture, &srvDesc, &myTexture);
        pTexture->Release();
    }
    stbi_image_free(rgba_bgImg_data);

    g_bgTexture = myTexture;

    MSG msg{};
    while (msg.message != WM_QUIT) {
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }
        render_frame();
    }

    if (g_bgTexture) { g_bgTexture->Release(); g_bgTexture = nullptr; }
    banking_gui::shutdown(g_hwnd);
    free_d3d11();
    UnregisterClassW(L"BankingSystemWnd", hInst);
    return 0;
}