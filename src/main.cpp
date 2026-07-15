#include <windows.h>
#include "overlay.h"
#include "console.h"
#include "plugin_manager.h"

// Entry point — WIN32 subsystem (no console window)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    Console::Get().Log("RLMod v0.1.0 starting...");
    Console::Get().Log("Press F1 to open / close this console.");
    Console::Get().Log("Type 'help' for available commands.");

    // Load plugins from <exe dir>/plugins/
    PluginManager::Get().ScanAndLoad();

    // Create overlay window + DX11 device
    Overlay overlay;
    if (!overlay.Init(hInstance)) {
        MessageBoxW(nullptr,
            L"Failed to initialise the overlay.\n"
            L"Make sure DirectX 11 is available on this machine.",
            L"RLMod – Fatal Error",
            MB_ICONERROR | MB_OK);
        return 1;
    }

    Console::Get().Log("Overlay ready. Waiting for F1...");

    // Block here until the user closes RLMod
    overlay.Run();

    // Clean shutdown
    PluginManager::Get().UnloadAll();
    overlay.Shutdown();
    return 0;
}
