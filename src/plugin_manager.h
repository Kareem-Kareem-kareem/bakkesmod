#pragma once
#ifndef RLMOD_PLUGIN_MANAGER_H
#define RLMOD_PLUGIN_MANAGER_H
#include <string>
#include <vector>
#include <windows.h>
#include "plugin_api.h"

class PluginManager : public IPluginHost {
public:
    static PluginManager& Get();

    void ScanAndLoad();
    void UnloadAll();

    // IPluginHost
    void         Log(const std::string& message) override;
    IGameMemory* GetGameMemory()                 override;

private:
    PluginManager() = default;

    struct LoadedPlugin {
        std::string name;
        HMODULE     module = nullptr;
        IPlugin*    plugin = nullptr;
    };

    std::vector<LoadedPlugin> m_plugins;
    void LoadPlugin(const std::wstring& dllPath);
};
#endif // RLMOD_PLUGIN_MANAGER_H
