# RLMod — Rocket League Mod Framework

> **Educational purposes only.** This project is intended for learning about
> overlay development, plugin systems, and Windows graphics APIs. Use
> responsibly and only on your own machine.

An external overlay mod framework for Rocket League, inspired by BakkesMod.
Because it runs as a **separate process** and never injects into the game, it
is compatible with Easy Anti-Cheat.

---

## Features

| Feature | Details |
|---|---|
| **F1 console** | Press F1 in-game to open/close the mod console |
| **Plugin tabs** | Each plugin gets its own tab in the console UI |
| **Command input** | Type commands in the console (`help`, `clear`, `version`) |
| **Plugin SDK** | Drop a DLL into the `plugins/` folder — no recompile needed |
| **EAC safe** | External overlay only — never touches the game process |

---

## How to get the .exe (no Visual Studio needed)

This repo uses **GitHub Actions** to build automatically.

1. Push to the `main` branch (or open a Pull Request).
2. Go to the **Actions** tab on GitHub.
3. Click the latest **Build RLMod** run.
4. Download **RLMod-x64-Release** from the Artifacts section.
5. Extract the zip — you get `RLMod.exe` and a `plugins/` folder.

To make an official release:

```
git tag v0.1.0
git push origin v0.1.0
```

GitHub Actions will build and attach `RLMod.exe` to a new GitHub Release
automatically.

---

## Usage

1. Start Rocket League.
2. Run `RLMod.exe`.
3. Press **F1** to open the console overlay.
4. Type `help` to see available commands.
5. Press **F1** again (or click outside) to hide the console.

---

## Writing a plugin

Copy `examples/hello-plugin/` as a starting point.

Every plugin is a Windows DLL that exports one function:

```cpp
#include "plugin_api.h"
#include <imgui.h>

class MyPlugin : public IPlugin {
public:
    const char* GetName()    const override { return "My Plugin"; }
    const char* GetVersion() const override { return "1.0.0"; }

    void OnLoad(IPluginHost* host) override {
        host->Log("My plugin loaded!");
    }

    void OnRender() override {
        // Draw ImGui widgets here — you're inside a tab item
        ImGui::Text("Hello from my plugin!");
    }

    void OnUnload() override {}
    void Destroy()  override { delete this; }
};

RLMOD_PLUGIN_EXPORT IPlugin* CreatePlugin() {
    return new MyPlugin();
}
```

Build it as a DLL (see `examples/hello-plugin/CMakeLists.txt`), then drop the
`.dll` into the `plugins/` folder next to `RLMod.exe`. It loads on the next
startup.

---

## Project structure

```
rocket-mod/
├── .github/
│   └── workflows/
│       └── build.yml          ← GitHub Actions CI (builds the .exe for you)
├── include/
│   └── plugin_api.h           ← Plugin interface (IPlugin, IPluginHost)
├── src/
│   ├── main.cpp               ← Entry point
│   ├── overlay.cpp / .h       ← DX11 transparent window + F1 hotkey
│   ├── console.cpp / .h       ← ImGui console UI (tabs, log, command input)
│   └── plugin_manager.cpp/.h  ← Loads DLLs from plugins/ folder
├── examples/
│   └── hello-plugin/          ← Example plugin to copy from
├── CMakeLists.txt             ← Build definition (fetches ImGui automatically)
└── README.md
```

---

## How it stays EAC-safe

| Technique | Why it matters |
|---|---|
| Separate process | Never injected into `RocketLeague.exe` |
| Transparent overlay window | Uses Win32 `WS_EX_LAYERED` — standard Windows API |
| `RegisterHotKey` | System-level hotkey, no keyboard hook in the game |
| DirectX 11 on its own window | Completely separate DX context from the game |

---

## Requirements (runtime)

- Windows 10 / 11
- DirectX 11 (built into Windows — no extra install needed)
- Visual C++ Redistributable 2022 x64 (usually already installed)

---

## License

MIT — see `LICENSE` for details. Educational use only.
