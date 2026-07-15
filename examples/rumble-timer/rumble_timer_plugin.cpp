/**
 * Rumble Timer Plugin
 * ─────────────────────────────────────────────────────────────────────────────
 * Lets you read and write the rumble item timer while a match is running.
 *
 * HOW THE ADDRESS IS FOUND
 * ────────────────────────
 * The rumble timer is a float (4 bytes) that counts down from ~10 seconds.
 * To find it for the current game version:
 *
 *   1. Open Cheat Engine, attach to RocketLeague.exe.
 *   2. Start a rumble match and let a powerup appear.
 *   3. Scan for "Float" value ~10.0 (the initial countdown).
 *   4. Wait a few seconds, scan again for the decreased value.
 *   5. Repeat until you have 1-4 addresses.
 *   6. Right-click the address → "Find out what writes to this address".
 *   7. The instruction that writes to it will show a byte pattern nearby.
 *   8. Copy ~12 bytes before the write instruction as your pattern.
 *   9. Put that pattern string in RUMBLE_TIMER_PATTERN below.
 *  10. The PATTERN_OFFSET is the number of bytes from the pattern match to
 *      the float value itself (count forward from your pattern start).
 *
 * The pattern approach means the plugin keeps working after minor patches
 * as long as the surrounding code hasn't changed.
 *
 * EXAMPLE PATTERN (placeholder — find the real one with Cheat Engine):
 *   "F3 0F 11 83 ? ? ? ? F3 0F 10 05"
 *   PATTERN_OFFSET = 4  (the ?? ?? ?? ?? is the offset to the float)
 */

#include <imgui.h>
#include "../../include/plugin_api.h"
#include <string>
#include <sstream>
#include <iomanip>

// ── Configuration ─────────────────────────────────────────────────────────────
// Replace this with the real pattern you find via Cheat Engine.
static const char* RUMBLE_TIMER_PATTERN = "F3 0F 11 83 ? ? ? ? F3 0F 10 05";
// Byte offset from the start of the pattern match to the float address.
// For the pattern above the ?? ?? ?? ?? bytes hold the relative offset —
// you will need to adjust this to the exact value you find.
static const uintptr_t PATTERN_OFFSET = 4;

// ─────────────────────────────────────────────────────────────────────────────
class RumbleTimerPlugin : public IPlugin {
public:
    const char* GetName()    const override { return "Rumble Timer"; }
    const char* GetVersion() const override { return "1.0.0"; }

    // ── Lifecycle ──────────────────────────────────────────────────────────────
    void OnLoad(IPluginHost* host) override {
        m_host = host;
        m_mem  = host->GetGameMemory();
        // Sync this DLL's ImGui context with the host's so UI calls don't crash.
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(host->GetImGuiContext()));
        host->Log("Rumble Timer plugin loaded.");

        if (!m_mem || !m_mem->IsAttached()) {
            host->Log("WARN: Game memory not attached. Start Rocket League before RLMod.");
            return;
        }
        FindTimerAddress();
    }

    void OnUnload() override {
        if (m_host) m_host->Log("Rumble Timer plugin unloaded.");
    }

    void Destroy() override { delete this; }

    // ── UI ─────────────────────────────────────────────────────────────────────
    void OnRender() override {
        if (!m_mem) {
            ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "No game memory access.");
            return;
        }
        if (!m_mem->IsAttached()) {
            ImGui::TextColored(ImVec4(1,0.4f,0.4f,1), "Not attached to RocketLeague.exe.");
            ImGui::Text("Start Rocket League, then restart RLMod.");
            return;
        }

        // ── Address status ────────────────────────────────────────────────────
        ImGui::SeparatorText("Timer Address");

        if (m_timerAddr == 0) {
            ImGui::TextColored(ImVec4(1,0.85f,0.2f,1),
                "Address not found. Start a Rumble match, then click Scan.");
            if (ImGui::Button("Scan for timer address"))
                FindTimerAddress();
        } else {
            ImGui::Text("Address: 0x%llX", (unsigned long long)m_timerAddr);
            ImGui::SameLine();
            if (ImGui::SmallButton("Re-scan"))
                FindTimerAddress();
        }

        ImGui::Spacing();
        ImGui::SeparatorText("Live Value");

        // ── Live read ─────────────────────────────────────────────────────────
        if (m_timerAddr != 0) {
            float current = m_mem->Read<float>(m_timerAddr);

            // Sanity-check: if the read value is nonsense the address is wrong
            bool validRange = current >= 0.f && current <= 60.f;

            if (validRange)
                ImGui::Text("Current timer: %.3f s", current);
            else
                ImGui::TextColored(ImVec4(1,0.4f,0.4f,1),
                    "Current timer: %.3f  (out of expected range — rescan)", current);

            ImGui::Spacing();
            ImGui::SeparatorText("Edit Timer");

            ImGui::SetNextItemWidth(180.f);
            ImGui::InputFloat("New value (seconds)", &m_newValue, 0.5f, 1.0f, "%.1f");
            if (m_newValue < 0.f)  m_newValue = 0.f;
            if (m_newValue > 60.f) m_newValue = 60.f;

            ImGui::Spacing();

            if (ImGui::Button("Apply##set")) {
                if (m_mem->Write<float>(m_timerAddr, m_newValue)) {
                    std::ostringstream ss;
                    ss << "Rumble timer set to " << std::fixed << std::setprecision(1)
                       << m_newValue << " s";
                    m_host->Log(ss.str());
                    m_lastStatus = "Applied!";
                } else {
                    m_lastStatus = "Write failed — are you in a match?";
                    m_host->Log("ERROR: Failed to write rumble timer.");
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Freeze at value")) {
                m_frozen      = !m_frozen;
                m_frozenValue = m_newValue;
            }
            ImGui::SameLine();
            ImGui::TextDisabled(m_frozen ? "(frozen)" : "");

            if (!m_lastStatus.empty()) {
                ImGui::TextColored(ImVec4(0.4f,1,0.6f,1), "%s", m_lastStatus.c_str());
            }

            // Freeze: re-write every frame to hold the value
            if (m_frozen && m_timerAddr != 0)
                m_mem->Write<float>(m_timerAddr, m_frozenValue);
        }

        // ── Help / instructions ───────────────────────────────────────────────
        ImGui::Spacing();
        ImGui::SeparatorText("How to find the pattern");
        ImGui::TextWrapped(
            "If 'Scan' can't find the address automatically:\n"
            "1. Open Cheat Engine and attach to RocketLeague.exe\n"
            "2. Start a Rumble match and wait for a powerup\n"
            "3. Scan for a float ~10.0, then narrow it down\n"
            "4. Right-click the address -> 'Find out what writes to this'\n"
            "5. Copy the byte pattern before the write instruction\n"
            "6. Update RUMBLE_TIMER_PATTERN in rumble_timer_plugin.cpp"
        );
    }

private:
    IPluginHost*  m_host       = nullptr;
    IGameMemory*  m_mem        = nullptr;
    uintptr_t     m_timerAddr  = 0;
    float         m_newValue   = 10.0f;
    float         m_frozenValue = 10.0f;
    bool          m_frozen     = false;
    std::string   m_lastStatus;

    void FindTimerAddress() {
        m_timerAddr = 0;
        if (!m_mem || !m_mem->IsAttached()) return;

        if (m_host) m_host->Log("Scanning for rumble timer address...");
        uintptr_t match = m_mem->PatternScan(RUMBLE_TIMER_PATTERN);
        if (match == 0) {
            if (m_host) m_host->Log(
                "WARN: Pattern not found. Make sure you are in a Rumble match "
                "and update RUMBLE_TIMER_PATTERN with the correct bytes.");
            return;
        }
        m_timerAddr = match + PATTERN_OFFSET;
        if (m_host) {
            std::ostringstream ss;
            ss << "Rumble timer address found: 0x" << std::hex << std::uppercase << m_timerAddr;
            m_host->Log(ss.str());
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
RLMOD_PLUGIN_EXPORT IPlugin* CreatePlugin() {
    return new RumbleTimerPlugin();
}
