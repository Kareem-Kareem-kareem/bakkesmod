#include "plugin_manager.h"
#include "console.h"
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
PluginManager& PluginManager::Get() {
    static PluginManager instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
void PluginManager::ScanAndLoad() {
    // Resolve <exe_dir>/plugins/
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    fs::path pluginsDir = fs::path(exePath).parent_path() / L"plugins";

    if (!fs::exists(pluginsDir)) {
        fs::create_directories(pluginsDir);
        Log("Created plugins folder: " + pluginsDir.string());
        return;
    }

    int loaded = 0;
    for (const auto& entry : fs::directory_iterator(pluginsDir)) {
        if (entry.path().extension() == L".dll") {
            LoadPlugin(entry.path().wstring());
            ++loaded;
        }
    }

    if (loaded == 0)
        Log("No plugins found in plugins/ folder.");
}

// ─────────────────────────────────────────────────────────────────────────────
void PluginManager::LoadPlugin(const std::wstring& dllPath) {
    HMODULE mod = LoadLibraryW(dllPath.c_str());
    if (!mod) {
        std::ostringstream ss;
        ss << "ERROR: Failed to load DLL (error " << GetLastError() << "): "
           << fs::path(dllPath).filename().string();
        Log(ss.str());
        return;
    }

    using CreatePluginFn = IPlugin*(*)();
    auto createFn = reinterpret_cast<CreatePluginFn>(
        GetProcAddress(mod, "CreatePlugin"));

    if (!createFn) {
        Log("WARN: DLL missing CreatePlugin export: " +
            fs::path(dllPath).filename().string());
        FreeLibrary(mod);
        return;
    }

    IPlugin* plugin = createFn();
    if (!plugin) {
        Log("ERROR: CreatePlugin returned nullptr for: " +
            fs::path(dllPath).filename().string());
        FreeLibrary(mod);
        return;
    }

    plugin->OnLoad(this);

    // Register a tab in the console for this plugin
    Console::Get().RegisterTab(
        plugin->GetName(),
        [plugin]() { plugin->OnRender(); }
    );

    m_plugins.push_back({ plugin->GetName(), mod, plugin });
}

// ─────────────────────────────────────────────────────────────────────────────
void PluginManager::UnloadAll() {
    for (auto& p : m_plugins) {
        if (p.plugin) {
            Console::Get().UnregisterTab(p.name);
            p.plugin->OnUnload();
            p.plugin->Destroy();
        }
        if (p.module)
            FreeLibrary(p.module);
    }
    m_plugins.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
void PluginManager::Log(const std::string& message) {
    Console::Get().Log(message);
}
