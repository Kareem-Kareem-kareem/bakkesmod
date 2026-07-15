#pragma once
#include <string>
#include <vector>
#include <functional>

/**
 * Console
 * ────────
 * Singleton ImGui window that renders as a tabbed console:
 *   • "Log"     – scrollable output log
 *   • One tab per loaded plugin (added by PluginManager)
 *
 * Press F1 (handled in Overlay) to show / hide.
 */
class Console {
public:
    static Console& Get();

    // Called every frame by Overlay::Render() when visible.
    void Draw();

    // Append a line to the log tab.
    void Log(const std::string& message);

    // Register / remove a named render callback (used by PluginManager).
    void RegisterTab(const std::string& name, std::function<void()> renderFn);
    void UnregisterTab(const std::string& name);

private:
    Console() = default;

    struct Tab {
        std::string            name;
        std::function<void()>  renderFn;
    };

    std::vector<std::string> m_logLines;
    std::vector<Tab>         m_tabs;
    bool                     m_scrollToBottom = false;
    char                     m_inputBuf[256]  = {};

    void DrawLogTab();
    void ExecuteCommand(const std::string& cmd);
};
