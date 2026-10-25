#pragma once
// Hyprland's CFunctionHook patches x86-64 code only (HookSystem.cpp returns false on any other
// architecture), so on aarch64 the plugin hooks a function itself. Its first instruction becomes
// a direct branch to a page allocated within branch range, which holds two pieces of code:
//
//   launch:  ldr x16, #12; br x16; .quad destination   jumps on to the hook, anywhere in memory
//   land:    <first instruction>; b source + 4          the hook calls this as the original
//
// Only the one instruction moves, so it must not depend on where it runs: anything PC-relative is
// refused. x16 is the AAPCS64 intra-procedure scratch register that veneers clobber the same way.
//
// A function built with branch protection starts with a landing pad that indirect calls must
// land on, so the pad stays and the instruction after it moves instead. When the pad is paciasp
// or pacibsp, the return address is already signed by the time the branch is taken: launch
// authenticates it before entering the hook, and land signs it again for the original.
//
// Header-only with C headers so it can be built and run on its own for an aarch64 test.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000
#endif
#ifndef PROT_BTI
#define PROT_BTI 0x10
#endif

class Arm64Hook {
  public:
    // A branch's 26-bit word offset reaches 128 MB either way; the stub page stays well inside.
    static constexpr uintptr_t Reach = uintptr_t{120} << 20;

    static bool pcRelative(uint32_t instruction) {
        return (instruction & 0x1F000000u) == 0x10000000u    // adr, adrp
            || (instruction & 0x7C000000u) == 0x14000000u    // b, bl
            || (instruction & 0xFF000000u) == 0x54000000u    // b.cond, bc.cond
            || (instruction & 0x7E000000u) == 0x34000000u    // cbz, cbnz
            || (instruction & 0x7E000000u) == 0x36000000u    // tbz, tbnz
            || (instruction & 0x3B000000u) == 0x18000000u;   // ldr, ldrsw, prfm (literal)
    }

    static constexpr uint32_t Nop = 0xD503201Fu;
    static constexpr uint32_t BtiC = 0xD503245Fu, BtiJC = 0xD50324DFu;
    static constexpr uint32_t PacIASP = 0xD503233Fu, AutIASP = 0xD50323BFu;
    static constexpr uint32_t PacIBSP = 0xD503237Fu, AutIBSP = 0xD50323FFu;

    static uint32_t branch(uintptr_t from, uintptr_t to) {
        const intptr_t offset = static_cast<intptr_t>(to - from);
        return 0x14000000u | (static_cast<uint32_t>(offset >> 2) & 0x03FFFFFFu);
    }

    bool install(void* source, void* destination) {
        if (m_page) {
            m_error = "already installed";
            return false;
        }
        if (!source || !destination || reinterpret_cast<uintptr_t>(source) % 4) {
            m_error = "function address is not an instruction";
            return false;
        }
        auto* at = static_cast<uint32_t*>(source);
        uint32_t authenticate = Nop, sign = Nop;
        if (at[0] == PacIASP || at[0] == PacIBSP) {
            authenticate = at[0] == PacIASP ? AutIASP : AutIBSP;
            sign = at[0];
        }
        if (at[0] == BtiC || at[0] == BtiJC || at[0] == PacIASP || at[0] == PacIBSP)
            ++at;
        const uint32_t moved = *at;
        if (pcRelative(moved)) {
            m_error = "the instruction to move is PC-relative";
            return false;
        }
        const auto from = reinterpret_cast<uintptr_t>(at);
        const long pageSize = sysconf(_SC_PAGESIZE);
        auto* page = static_cast<uint32_t*>(allocateNear(from, pageSize));
        if (!page) {
            m_error = "no free page within branch range";
            return false;
        }
        const auto launch = reinterpret_cast<uintptr_t>(page);
        page[0] = authenticate;
        page[1] = 0x58000070u; // ldr x16, #12 (the address below, 8-byte aligned)
        page[2] = 0xD61F0200u; // br x16
        page[3] = Nop;
        const auto target = reinterpret_cast<uint64_t>(destination);
        memcpy(page + 4, &target, sizeof(target));
        page[6] = sign;
        page[7] = moved;
        page[8] = branch(launch + 8 * 4, from + 4);
        __builtin___clear_cache(reinterpret_cast<char*>(page), reinterpret_cast<char*>(page + 9));
        if (mprotect(page, pageSize, PROT_READ | PROT_EXEC) != 0 || !writeInstruction(at, branch(from, launch))) {
            munmap(page, pageSize);
            m_error = "could not make the function writable";
            return false;
        }
        m_source = at;
        m_first = moved;
        m_page = page;
        m_pageSize = pageSize;
        m_error = nullptr;
        return true;
    }

    void remove() {
        if (!m_page)
            return;
        // Should the instruction fail to go back, the branch still needs somewhere to land.
        if (writeInstruction(m_source, m_first))
            munmap(m_page, m_pageSize);
        m_page = nullptr;
    }

    void* original() const { return m_page ? m_page + 6 : nullptr; }
    const char* error() const { return m_error ? m_error : "unknown error"; }

    // Whether the page holding an address is BTI-guarded. Only /proc says so, and making the page
    // writable drops the guard, so it is put back afterwards.
    static bool guarded(const void* at) {
        FILE* smaps = fopen("/proc/self/smaps", "r");
        if (!smaps)
            return false;
        const auto address = reinterpret_cast<uintptr_t>(at);
        char line[4096];
        bool inside = false, result = false;
        while (fgets(line, sizeof(line), smaps)) {
            unsigned long start = 0, end = 0;
            if (sscanf(line, "%lx-%lx ", &start, &end) == 2) {
                inside = address >= start && address < end;
            } else if (inside && strncmp(line, "VmFlags:", 8) == 0) {
                result = strstr(line, " bt ") || strstr(line, " bt\n");
                break;
            }
        }
        fclose(smaps);
        return result;
    }

  private:
    static void* allocateNear(uintptr_t address, long pageSize) {
        const uintptr_t base = address & ~(static_cast<uintptr_t>(pageSize) - 1);
        constexpr uintptr_t step = uintptr_t{1} << 20;
        for (uintptr_t distance = step; distance < Reach; distance += step) {
            for (int below = 1; below >= 0; --below) {
                if (below && distance > base)
                    continue;
                const uintptr_t candidate = below ? base - distance : base + distance;
                void* page = mmap(reinterpret_cast<void*>(candidate), pageSize, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
                if (page == MAP_FAILED)
                    continue;
                const auto got = reinterpret_cast<uintptr_t>(page);
                // Kernels before 4.17 take the address as a hint only.
                if ((got > address ? got - address : address - got) < Reach)
                    return page;
                munmap(page, pageSize);
            }
        }
        return nullptr;
    }

    // The 4-byte store is single-copy atomic, and Hyprland renders on this thread anyway.
    static bool writeInstruction(void* at, uint32_t instruction) {
        const long pageSize = sysconf(_SC_PAGESIZE);
        void* page = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(at) & ~(static_cast<uintptr_t>(pageSize) - 1));
        const int executable = PROT_READ | PROT_EXEC | (guarded(at) ? PROT_BTI : 0);
        if (mprotect(page, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) != 0)
            return false;
        __atomic_store_n(static_cast<uint32_t*>(at), instruction, __ATOMIC_RELEASE);
        mprotect(page, pageSize, executable);
        __builtin___clear_cache(static_cast<char*>(at), static_cast<char*>(at) + 4);
        return true;
    }

    // The patched instruction, which is the first one unless a landing pad comes before it.
    void* m_source = nullptr;
    uint32_t m_first = 0;
    uint32_t* m_page = nullptr;
    long m_pageSize = 0;
    const char* m_error = nullptr;
};
