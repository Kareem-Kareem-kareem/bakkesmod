#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include "../include/plugin_api.h"

/**
 * PluginManager
 * ─────────────
 * Scans the "plugins" folder (next to the .exe) for DLLs, loads them, and
 * registers their console tabs. Call ScanAndLoad() once at startup.
 */
class PluginManager : public IPluginHost {
public:
    static PluginManager& Get();

    // Load all DLLs from <exe_dir>/plugins/
    void ScanAndLoad();

    // Unload all plugins cleanly.
    void UnloadAll();

    // IPluginHost
    void Log(const std::string& message) override;

private:
    PluginManager() = default;

    struct LoadedPlugin {
        std::string name;
        HMODULE     module  = nullptr;
        IPlugin*    plugin  = nullptr;
    };

    std::vector<LoadedPlugin> m_plugins;

    void LoadPlugin(const std::wstring& dllPath);
};
