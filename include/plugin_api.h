#pragma once
#ifndef RLMOD_PLUGIN_API_H
#define RLMOD_PLUGIN_API_H
/**
 * RLMod Plugin API
 * ─────────────────────────────────────────────────────────────────────────────
 * Every plugin is a Windows DLL that exports a single factory function:
 *
 *   extern "C" __declspec(dllexport) IPlugin* CreatePlugin();
 *
 * Lifecycle:
 *   1. CreatePlugin()  → host gets IPlugin*
 *   2. OnLoad()        → called once after the plugin is registered
 *   3. OnRender()      → called every frame while the console is open
 *   4. OnUnload()      → called before the plugin is removed
 *   5. Destroy()       → host calls this to free the object
 */

#include <string>
#include <cstdint>
#include <vector>

// ── Memory access service ─────────────────────────────────────────────────────
struct IGameMemory {
    virtual ~IGameMemory() = default;

    // True once Attach() has succeeded.
    virtual bool IsAttached() const = 0;

    // Base address of RocketLeague.exe in the target process.
    virtual uintptr_t GetBaseAddress() const = 0;

    // Read / write typed values at an absolute address.
    virtual bool ReadBytes (uintptr_t address, void* out, size_t size) const = 0;
    virtual bool WriteBytes(uintptr_t address, const void* in, size_t size) const = 0;

    // Convenience typed wrappers (implemented inline).
    template<typename T>
    T Read(uintptr_t address) const {
        T v{};
        ReadBytes(address, &v, sizeof(T));
        return v;
    }
    template<typename T>
    bool Write(uintptr_t address, const T& value) const {
        return WriteBytes(address, &value, sizeof(T));
    }

    // Follow a pointer chain: base + offsets[0] → deref → + offsets[1] → ...
    virtual uintptr_t Follow(const std::vector<uintptr_t>& offsets) const = 0;

    // Pattern scan (e.g. "48 8B 05 ? ? ? ? 48 85 C0"). Returns 0 if not found.
    virtual uintptr_t PatternScan(const std::string& pattern) const = 0;
};

// ── Host services exposed to plugins ─────────────────────────────────────────
struct IPluginHost {
    virtual ~IPluginHost() = default;

    // Print a line to the mod console log.
    virtual void Log(const std::string& message) = 0;

    // Access to the game's process memory (may return nullptr if not attached).
    virtual IGameMemory* GetGameMemory() = 0;

    // Returns the host's ImGuiContext* as void* so plugin_api.h doesn't need
    // to include imgui.h. Cast to ImGuiContext* and call
    // ImGui::SetCurrentContext() in your plugin's OnLoad() — required because
    // each DLL compiles its own copy of ImGui with a separate GImGui pointer.
    virtual void* GetImGuiContext() = 0;
};

// ── Plugin interface ──────────────────────────────────────────────────────────
struct IPlugin {
    virtual ~IPlugin() = default;

    virtual const char* GetName()    const = 0;
    virtual const char* GetVersion() const = 0;

    virtual void OnLoad  (IPluginHost* host) = 0;
    virtual void OnRender()                  = 0;
    virtual void OnUnload()                  = 0;
    virtual void Destroy ()                  = 0;
};

// ── Convenience macro for plugin DLLs ────────────────────────────────────────
#define RLMOD_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#endif // RLMOD_PLUGIN_API_H
