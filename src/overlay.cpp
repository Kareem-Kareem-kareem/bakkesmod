#include "overlay.h"
#include "console.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <dwmapi.h>
#include <tchar.h>

// Forward declared by imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
    HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Singleton pointer so the static WndProc can reach the instance.
static Overlay* g_overlay = nullptr;

// ─────────────────────────────────────────────────────────────────────────────
bool Overlay::Init(HINSTANCE hInstance) {
    g_overlay = this;

    // ── Window class ──────────────────────────────────────────────────────────
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"RLModOverlay";
    RegisterClassEx(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    // ── Create layered, transparent, always-on-top overlay window ─────────────
    m_hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        L"RLModOverlay", L"RLMod",
        WS_POPUP,
        0, 0, screenW, screenH,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!m_hwnd) return false;

    // Fully opaque colour key (we use DX alpha blending instead).
    SetLayeredWindowAttributes(m_hwnd, 0, 255, LWA_ALPHA);

    // Remove title bar from DWM hit-testing (prevents taskbar icon, etc.).
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(m_hwnd, &margins);

    // ── DX11 swap chain ───────────────────────────────────────────────────────
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount                        = 2;
    sd.BufferDesc.Width                   = 0;
    sd.BufferDesc.Height                  = 0;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags                              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow                       = m_hwnd;
    sd.SampleDesc.Count                   = 1;
    sd.Windowed                           = TRUE;
    sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        0, levels, 2,
        D3D11_SDK_VERSION,
        &sd, &m_swapChain,
        &m_device, &featureLevel, &m_context
    );
    if (FAILED(hr)) return false;

    CreateRenderTarget();

    // ── ImGui ─────────────────────────────────────────────────────────────────
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Disable ini saves (we don't want a stray imgui.ini in the game folder)
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 6.0f;
    style.FrameRounding     = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;

    ImGui_ImplWin32_Init(m_hwnd);
    ImGui_ImplDX11_Init(m_device, m_context);

    // ── Global hotkey F1 ─────────────────────────────────────────────────────
    // ID 1 = F1. NOREPEAT prevents continuous fire while held.
    RegisterHotKey(m_hwnd, 1, MOD_NOREPEAT, VK_F1);

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
void Overlay::Run() {
    MSG msg = {};
    while (m_running) {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) m_running = false;
        }
        if (!m_running) break;
        Render();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void Overlay::Render() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_consoleVisible)
        Console::Get().Draw();

    ImGui::Render();

    const float clear[4] = { 0.f, 0.f, 0.f, 0.f };   // fully transparent
    m_context->OMSetRenderTargets(1, &m_renderTarget, nullptr);
    m_context->ClearRenderTargetView(m_renderTarget, clear);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    m_swapChain->Present(1, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
void Overlay::SetConsoleVisible(bool visible) {
    m_consoleVisible = visible;

    // When console is visible we need to receive mouse/keyboard input.
    // When hidden, let all input pass through to the game.
    LONG_PTR exStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
    if (visible) {
        exStyle &= ~WS_EX_TRANSPARENT;
        exStyle &= ~WS_EX_NOACTIVATE;
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle);
        SetForegroundWindow(m_hwnd);
    } else {
        exStyle |= WS_EX_TRANSPARENT;
        exStyle |= WS_EX_NOACTIVATE;
        SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void Overlay::CreateRenderTarget() {
    ID3D11Texture2D* backBuf = nullptr;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuf));
    m_device->CreateRenderTargetView(backBuf, nullptr, &m_renderTarget);
    backBuf->Release();
}

void Overlay::CleanupRenderTarget() {
    if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
}

// ─────────────────────────────────────────────────────────────────────────────
void Overlay::Shutdown() {
    UnregisterHotKey(m_hwnd, 1);
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupRenderTarget();
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context)   { m_context->Release();   m_context   = nullptr; }
    if (m_device)    { m_device->Release();     m_device    = nullptr; }
    DestroyWindow(m_hwnd);
    UnregisterClass(L"RLModOverlay", GetModuleHandle(nullptr));
}

// ─────────────────────────────────────────────────────────────────────────────
LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;

    switch (msg) {
        case WM_HOTKEY:
            if (wParam == 1 && g_overlay)   // F1
                g_overlay->SetConsoleVisible(!g_overlay->IsConsoleVisible());
            return 0;

        case WM_SIZE:
            if (g_overlay && g_overlay->m_device && wParam != SIZE_MINIMIZED) {
                g_overlay->CleanupRenderTarget();
                g_overlay->m_swapChain->ResizeBuffers(
                    0, LOWORD(lParam), HIWORD(lParam),
                    DXGI_FORMAT_UNKNOWN, 0);
                g_overlay->CreateRenderTarget();
            }
            return 0;

        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
