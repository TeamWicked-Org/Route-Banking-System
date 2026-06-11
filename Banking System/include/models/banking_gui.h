#pragma once
// =============================================================================
// banking_gui.h  --  ImGui/D3D11 GUI for the Banking System
// =============================================================================
#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>

#include "../../external/imgui/imgui.h"
#include "../../external/imgui/imgui_impl_win32.h"
#include "../../external/imgui/imgui_impl_dx11.h"

namespace banking_gui {
    bool init(HWND hwnd, ID3D11Device* dev, ID3D11DeviceContext* ctx);
    void tick(ID3D11ShaderResourceView* bgImg, int img_width, int img_height);
    void shutdown(HWND hwnd);
}