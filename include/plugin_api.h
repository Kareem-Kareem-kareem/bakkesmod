#pragma once
/**
 * RLMod Plugin API
 * ─────────────────────────────────────────────────────────────────────────────
 * Every plugin is a Windows DLL that exports a single factory function:
 *
 *   extern "C" __declspec(dllexport) IPlugin* CreatePlugin();
 *
 * The host calls CreatePlugin() at load time. The returned pointer is owned by
 * the plugin DLL — do NOT delete it from host code; call plugin->Destroy()
 * instead so the DLL frees memory in its own heap.
 *
 * Lifecycle:
 *   1. CreatePlugin()  → host gets IPlugin*
 *   2. OnLoad()        → called once after the plugin is registered
 *   3. OnRender()      → called every frame while the console is open
 *                        (inside an ImGui::Begin/End block scoped to your tab)
 *   4. OnUnload()      → called before the plugin is removed
 *   5. Destroy()       → host calls this to free the object
 */

#include <string>

// ── Forward declaration so plugins don't need to include imgui directly.
// Plugins that want to draw UI must include <imgui.h> themselves.
struct ImGuiContext;

// ── Host services exposed to plugins ─────────────────────────────────────────
struct IPluginHost {
    virtual ~IPluginHost() = default;

    // Print a line to the mod console log.
    virtual void Log(const std::string& message) = 0;
};

// ── Plugin interface ──────────────────────────────────────────────────────────
struct IPlugin {
    virtual ~IPlugin() = default;

    // Human-readable name shown in the console tab bar.
    virtual const char* GetName() const = 0;

    // Version string (e.g. "1.0.0").
    virtual const char* GetVersion() const = 0;

    // Called once when the plugin is loaded.
    virtual void OnLoad(IPluginHost* host) = 0;

    // Called every frame while the console window is visible.
    // Draw your ImGui widgets here; you are already inside a tab item.
    virtual void OnRender() = 0;

    // Called once before the plugin is unloaded.
    virtual void OnUnload() = 0;

    // Free this object. Implement as: void Destroy() override { delete this; }
    virtual void Destroy() = 0;
};

// ── Convenience macro for plugin DLLs ────────────────────────────────────────
#define RLMOD_PLUGIN_EXPORT extern "C" __declspec(dllexport)
