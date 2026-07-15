#pragma once
#ifndef RLMOD_GAME_MEMORY_H
#define RLMOD_GAME_MEMORY_H
#include <windows.h>
#include <string>
#include <vector>
#include <cstdint>
#include "plugin_api.h"

/**
 * GameMemory
 * ──────────
 * Implements IGameMemory. Attaches to a running process by name and provides
 * safe read/write helpers plus a pattern (signature) scanner that works across
 * game patches.
 *
 * Usage:
 *   GameMemory::Get().Attach("RocketLeague.exe");
 *   uintptr_t addr = GameMemory::Get().PatternScan("48 8B 05 ? ? ? ?");
 *   float val = GameMemory::Get().Read<float>(addr + offset);
 *   GameMemory::Get().Write<float>(addr + offset, 10.f);
 */
class GameMemory : public IGameMemory {
public:
    static GameMemory& Get();

    // Open a handle to the target process. Returns true if successful.
    bool Attach(const std::string& processName);

    // Release the process handle.
    void Detach();

    // ── IGameMemory ───────────────────────────────────────────────────────────
    bool      IsAttached()      const override { return m_handle != nullptr; }
    uintptr_t GetBaseAddress()  const override { return m_baseAddr; }

    bool ReadBytes (uintptr_t address, void* out,        size_t size) const override;
    bool WriteBytes(uintptr_t address, const void* in,   size_t size) const override;

    uintptr_t Follow     (const std::vector<uintptr_t>& offsets) const override;
    uintptr_t PatternScan(const std::string& pattern)            const override;

    // Scan within a specific address range instead of the whole module.
    uintptr_t PatternScanRange(uintptr_t start, size_t length,
                               const std::string& pattern) const;

    // ── Process info ─────────────────────────────────────────────────────────
    DWORD GetProcessId() const { return m_pid; }

private:
    GameMemory() = default;

    struct PatternByte {
        uint8_t value;
        bool    wildcard;
    };

    static std::vector<PatternByte> ParsePattern(const std::string& pattern);
    bool ScanChunk(const uint8_t* chunk, size_t chunkSize,
                   uintptr_t chunkBase,
                   const std::vector<PatternByte>& parsed,
                   uintptr_t& result) const;

    HANDLE    m_handle   = nullptr;
    DWORD     m_pid      = 0;
    uintptr_t m_baseAddr = 0;
};
#endif // RLMOD_GAME_MEMORY_H
