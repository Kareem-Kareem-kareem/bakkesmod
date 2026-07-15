#include "game_memory.h"
#include "console.h"
#include <tlhelp32.h>
#include <sstream>
#include <iomanip>

// ─────────────────────────────────────────────────────────────────────────────
GameMemory& GameMemory::Get() {
    static GameMemory instance;
    return instance;
}

// ─────────────────────────────────────────────────────────────────────────────
bool GameMemory::Attach(const std::string& processName) {
    Detach();

    // Convert to wide string for Toolhelp32
    std::wstring wName(processName.begin(), processName.end());

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    bool found = false;
    if (Process32FirstW(snap, &entry)) {
        do {
            if (wName == entry.szExeFile) {
                m_pid = entry.th32ProcessID;
                found = true;
                break;
            }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);

    if (!found) {
        Console::Get().Log("WARN: Process not found: " + processName);
        return false;
    }

    m_handle = OpenProcess(
        PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
        FALSE, m_pid);

    if (!m_handle) {
        Console::Get().Log("ERROR: OpenProcess failed (error " +
            std::to_string(GetLastError()) + "). Try running RLMod as Administrator.");
        m_pid = 0;
        return false;
    }

    // Find the base address of the main module
    HANDLE modSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
    if (modSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W mod{};
        mod.dwSize = sizeof(mod);
        if (Module32FirstW(modSnap, &mod)) {
            m_baseAddr = reinterpret_cast<uintptr_t>(mod.modBaseAddr);
        }
        CloseHandle(modSnap);
    }

    std::ostringstream ss;
    ss << "Attached to " << processName
       << " (PID " << m_pid << ", base 0x"
       << std::hex << std::uppercase << m_baseAddr << ")";
    Console::Get().Log(ss.str());
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
bool GameMemory::ReadBytes(uintptr_t address, void* out, size_t size) const {
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(m_handle,
        reinterpret_cast<LPCVOID>(address),
        out, size, &bytesRead) && bytesRead == size;
}

bool GameMemory::WriteBytes(uintptr_t address, const void* in, size_t size) const {
    SIZE_T written = 0;
    return WriteProcessMemory(m_handle,
        reinterpret_cast<LPVOID>(address),
        in, size, &written) && written == size;
}

// ─────────────────────────────────────────────────────────────────────────────
void GameMemory::Detach() {
    if (m_handle) {
        CloseHandle(m_handle);
        m_handle   = nullptr;
        m_pid      = 0;
        m_baseAddr = 0;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
uintptr_t GameMemory::Follow(const std::vector<uintptr_t>& offsets) const {
    if (!m_handle || offsets.empty()) return 0;

    uintptr_t addr = m_baseAddr + offsets[0];
    for (size_t i = 1; i < offsets.size(); ++i) {
        addr = Read<uintptr_t>(addr);
        if (!addr) return 0;
        addr += offsets[i];
    }
    return addr;
}

// ─────────────────────────────────────────────────────────────────────────────
std::vector<GameMemory::PatternByte>
GameMemory::ParsePattern(const std::string& pattern) {
    std::vector<PatternByte> result;
    std::istringstream ss(pattern);
    std::string token;
    while (ss >> token) {
        if (token == "??" || token == "?") {
            result.push_back({ 0, true });
        } else {
            uint8_t byte = static_cast<uint8_t>(std::stoul(token, nullptr, 16));
            result.push_back({ byte, false });
        }
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
bool GameMemory::ScanChunk(const uint8_t* chunk, size_t chunkSize,
                            uintptr_t chunkBase,
                            const std::vector<PatternByte>& parsed,
                            uintptr_t& result) const {
    if (parsed.empty() || chunkSize < parsed.size()) return false;

    for (size_t i = 0; i <= chunkSize - parsed.size(); ++i) {
        bool match = true;
        for (size_t j = 0; j < parsed.size(); ++j) {
            if (!parsed[j].wildcard && chunk[i + j] != parsed[j].value) {
                match = false;
                break;
            }
        }
        if (match) {
            result = chunkBase + i;
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
uintptr_t GameMemory::PatternScan(const std::string& pattern) const {
    if (!m_handle) return 0;

    // Walk all committed, readable memory regions of the target process
    MEMORY_BASIC_INFORMATION mbi{};
    uintptr_t addr = 0;
    auto parsed = ParsePattern(pattern);

    constexpr size_t CHUNK = 0x10000; // 64 KB read buffer
    std::vector<uint8_t> buf(CHUNK);

    while (VirtualQueryEx(m_handle, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi))) {
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                            PAGE_READONLY | PAGE_READWRITE)) &&
            !(mbi.Protect & PAGE_GUARD)) {

            uintptr_t regionBase = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
            size_t    remaining  = mbi.RegionSize;

            while (remaining > 0) {
                size_t toRead = min(remaining, CHUNK);
                SIZE_T bytesRead = 0;
                if (ReadProcessMemory(m_handle,
                        reinterpret_cast<LPCVOID>(regionBase),
                        buf.data(), toRead, &bytesRead) && bytesRead > 0) {
                    uintptr_t found = 0;
                    if (ScanChunk(buf.data(), bytesRead, regionBase, parsed, found))
                        return found;
                }
                regionBase += toRead;
                remaining  -= toRead;
            }
        }
        addr += mbi.RegionSize;
        if (addr == 0) break; // wrapped around
    }
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
uintptr_t GameMemory::PatternScanRange(uintptr_t start, size_t length,
                                        const std::string& pattern) const {
    if (!m_handle) return 0;
    auto parsed = ParsePattern(pattern);
    std::vector<uint8_t> buf(length);
    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(start),
                           buf.data(), length, &bytesRead))
        return 0;
    uintptr_t found = 0;
    ScanChunk(buf.data(), bytesRead, start, parsed, found);
    return found;
}
