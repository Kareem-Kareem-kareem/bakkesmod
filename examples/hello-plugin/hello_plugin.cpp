/**
 * Hello Plugin — example RLMod plugin
 * ─────────────────────────────────────────────────────────────────────────────
 * Build with CMake (add_subdirectory) or compile as a DLL and drop it into the
 * plugins/ folder next to RLMod.exe.
 *
 * GitHub Actions builds this automatically alongside the main project.
 */

#include <imgui.h>
#include "../../include/plugin_api.h"
#include <string>

class HelloPlugin : public IPlugin {
public:
    const char* GetName()    const override { return "Hello Plugin"; }
    const char* GetVersion() const override { return "1.0.0"; }

    void OnLoad(IPluginHost* host) override {
        m_host = host;
        // Sync this DLL's ImGui context with the host's so UI calls don't crash.
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(host->GetImGuiContext()));
        m_host->Log("Hello Plugin loaded! This is your first RLMod plugin.");
    }

    void OnRender() override {
        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.6f, 1.f), "Hello from RLMod!");
        ImGui::Separator();
        ImGui::Text("This is an example plugin tab.");
        ImGui::Spacing();

        ImGui::Text("Click counter: %d", m_clicks);
        if (ImGui::Button("Click me!"))
            ++m_clicks;

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextDisabled("Copy this file to make your own plugin.");
    }

    void OnUnload() override {
        if (m_host) m_host->Log("Hello Plugin unloaded.");
    }

    void Destroy() override { delete this; }

private:
    IPluginHost* m_host   = nullptr;
    int          m_clicks = 0;
};

RLMOD_PLUGIN_EXPORT IPlugin* CreatePlugin() {
    return new HelloPlugin();
}
