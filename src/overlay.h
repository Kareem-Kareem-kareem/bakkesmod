#pragma once
#include <windows.h>
#include <d3d11.h>
#include <functional>

/**
 * Overlay
 * ────────
 * Creates a fullscreen, transparent, always-on-top window and a DX11 device.
 * The window is WS_EX_TRANSPARENT (click-through) when the console is hidden,
 * and becomes interactive when the console is shown — fully EAC-safe because
 * we never touch the game process.
 */
class Overlay {
public:
    Overlay()  = default;
    ~Overlay() = default;

    // Create the window and DX11 device. Returns false on failure.
    bool Init(HINSTANCE hInstance);

    // Main message + render loop. Blocks until the user quits.
    void Run();

    // Clean up all DX11 and window resources.
    void Shutdown();

    // Show / hide the console (called by the hotkey handler).
    void SetConsoleVisible(bool visible);
    bool IsConsoleVisible() const { return m_consoleVisible; }

    // Expose DX11 objects so the ImGui backend can be initialised once.
    ID3D11Device*           GetDevice()        const { return m_device; }
    ID3D11DeviceContext*    GetDeviceContext()  const { return m_context; }
    IDXGISwapChain*         GetSwapChain()      const { return m_swapChain; }

private:
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void Render();

    // Win32 window procedure (static, forwards to instance).
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    HWND                    m_hwnd        = nullptr;
    ID3D11Device*           m_device      = nullptr;
    ID3D11DeviceContext*    m_context     = nullptr;
    IDXGISwapChain*         m_swapChain   = nullptr;
    ID3D11RenderTargetView* m_renderTarget = nullptr;

    bool m_consoleVisible = false;
    bool m_running        = true;
};
