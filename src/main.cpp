#include <windows.h>
#include "overlay.h"
#include "console.h"
#include "game_memory.h"
#include "plugin_manager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    Console::Get().Log("RLMod v0.1.0 starting...");
    Console::Get().Log("Press F1 to open / close this console.");
    Console::Get().Log("Type 'help' for available commands.");

    // Attach to Rocket League process (must be running already)
    if (GameMemory::Get().Attach("RocketLeague.exe")) {
        Console::Get().Log("Game memory ready. Plugins can now read/write game values.");
    } else {
        Console::Get().Log("WARN: RocketLeague.exe not found. Start Rocket League first,");
        Console::Get().Log("      then restart RLMod. Memory features will be unavailable.");
    }

    // Load plugins from <exe dir>/plugins/
    PluginManager::Get().ScanAndLoad();

    // Create overlay window + DX11 device
    Overlay overlay;
    if (!overlay.Init(hInstance)) {
        MessageBoxW(nullptr,
            L"Failed to initialise the overlay.\n"
            L"Make sure DirectX 11 is available on this machine.",
            L"RLMod \u2013 Fatal Error",
            MB_ICONERROR | MB_OK);
        return 1;
    }

    Console::Get().Log("Overlay ready. Waiting for F1...");

    overlay.Run();

    PluginManager::Get().UnloadAll();
    GameMemory::Get().Detach();
    overlay.Shutdown();
    return 0;
}
