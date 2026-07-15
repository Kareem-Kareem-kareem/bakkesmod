#include "console.h"
#include <imgui.h>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
Console& Console::Get() {
    static Console instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
void Console::Log(const std::string& message) {
    // Prefix with a simple timestamp HH:MM:SS
    auto now  = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[16];
    struct tm tmInfo;
    localtime_s(&tmInfo, &t);
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tmInfo);

    m_logLines.push_back(std::string("[") + buf + "] " + message);
    m_scrollToBottom = true;

    // Cap at 2000 lines to avoid unbounded growth
    if (m_logLines.size() > 2000)
        m_logLines.erase(m_logLines.begin());
}

// ─────────────────────────────────────────────────────────────────────────────
void Console::RegisterTab(const std::string& name, std::function<void()> renderFn) {
    // Avoid duplicates
    for (auto& t : m_tabs)
        if (t.name == name) return;
    m_tabs.push_back({ name, std::move(renderFn) });
    Log("Plugin loaded: " + name);
}

void Console::UnregisterTab(const std::string& name) {
    m_tabs.erase(
        std::remove_if(m_tabs.begin(), m_tabs.end(),
            [&](const Tab& t) { return t.name == name; }),
        m_tabs.end()
    );
    Log("Plugin unloaded: " + name);
}

// ─────────────────────────────────────────────────────────────────────────────
void Console::Draw() {
    ImGuiIO& io = ImGui::GetIO();
    const float W = io.DisplaySize.x * 0.55f;
    const float H = io.DisplaySize.y * 0.65f;

    ImGui::SetNextWindowSize(ImVec2(W, H), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        ImVec2(io.DisplaySize.x * 0.5f - W * 0.5f,
               io.DisplaySize.y * 0.5f - H * 0.5f),
        ImGuiCond_FirstUseEver
    );

    // Semi-transparent background
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("RLMod Console  [F1 to close]", nullptr, flags)) {
        ImGui::End();
        return;
    }

    // ── Tab bar ───────────────────────────────────────────────────────────────
    if (ImGui::BeginTabBar("##tabs")) {

        // Built-in log tab
        if (ImGui::BeginTabItem("Log")) {
            DrawLogTab();
            ImGui::EndTabItem();
        }

        // Plugin tabs
        for (auto& tab : m_tabs) {
            if (ImGui::BeginTabItem(tab.name.c_str())) {
                tab.renderFn();
                ImGui::EndTabItem();
            }
        }

        // Plugins management tab
        if (ImGui::BeginTabItem("Plugins")) {
            ImGui::TextDisabled("Loaded plugins:");
            ImGui::Separator();
            if (m_tabs.empty()) {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.f),
                    "No plugins loaded. Drop DLLs into the 'plugins' folder.");
            } else {
                for (auto& t : m_tabs) {
                    ImGui::BulletText("%s", t.name.c_str());
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// ─────────────────────────────────────────────────────────────────────────────
void Console::DrawLogTab() {
    // Scrollable log output
    const float inputHeight = ImGui::GetFrameHeightWithSpacing() + 4.f;
    ImGui::BeginChild("##log",
        ImVec2(0.f, -inputHeight), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& line : m_logLines) {
        // Colour-code lines that start with error keywords
        if (line.find("ERROR") != std::string::npos ||
            line.find("error") != std::string::npos)
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "%s", line.c_str());
        else if (line.find("WARN") != std::string::npos)
            ImGui::TextColored(ImVec4(1.f, 0.85f, 0.2f, 1.f), "%s", line.c_str());
        else if (line.find("Plugin") != std::string::npos)
            ImGui::TextColored(ImVec4(0.4f, 1.f, 0.6f, 1.f), "%s", line.c_str());
        else
            ImGui::TextUnformatted(line.c_str());
    }

    if (m_scrollToBottom) {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }

    ImGui::EndChild();

    // ── Command input ─────────────────────────────────────────────────────────
    ImGui::Separator();
    bool reclaim = false;
    ImGuiInputTextFlags inputFlags =
        ImGuiInputTextFlags_EnterReturnsTrue |
        ImGuiInputTextFlags_EscapeClearsAll;

    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::InputText("##input", m_inputBuf, sizeof(m_inputBuf), inputFlags)) {
        std::string cmd(m_inputBuf);
        if (!cmd.empty()) {
            Log("> " + cmd);
            ExecuteCommand(cmd);
        }
        m_inputBuf[0] = '\0';
        reclaim = true;
    }
    // Keep focus on the input box
    ImGui::SetItemDefaultFocus();
    if (reclaim) ImGui::SetKeyboardFocusHere(-1);
}

// ─────────────────────────────────────────────────────────────────────────────
void Console::ExecuteCommand(const std::string& cmd) {
    if (cmd == "help") {
        Log("Available commands:");
        Log("  help       - show this message");
        Log("  clear      - clear the log");
        Log("  version    - print RLMod version");
    } else if (cmd == "clear") {
        m_logLines.clear();
    } else if (cmd == "version") {
        Log("RLMod v0.1.0 — Educational mod framework");
    } else {
        Log("Unknown command: '" + cmd + "'. Type 'help' for a list.");
    }
}
