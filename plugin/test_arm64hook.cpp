// Runs Arm64Hook on functions with known first instructions: an ordinary prologue like
// Hyprland's renderLayer and saveBufferForMirror, the landing pads a build with branch protection
// starts functions with, and starts that cannot move because they are PC-relative.
#include "Arm64Hook.hpp"
#include <stdio.h>

extern "C" int plainTarget(int);
extern "C" int relativeTarget(int);
extern "C" int btiTarget(int);
extern "C" int pacTarget(int);
extern "C" int btiRelativeTarget(int);
extern "C" char btiTargetEnd[], pacTargetEnd[];

// Indirect calls into a BTI-guarded build must land on a pad, so the functions without one only
// exist when this test is built without branch protection.
#if !defined(__ARM_FEATURE_BTI_DEFAULT)
asm(R"(
    .text
    .p2align 2
    .globl plainTarget
    .type plainTarget, %function
plainTarget:
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    add w0, w0, #1
    ldp x29, x30, [sp], #16
    ret
    .size plainTarget, .-plainTarget

    .p2align 2
    .globl relativeTarget
    .type relativeTarget, %function
relativeTarget:
    adrp x1, relativeTarget
    add w0, w0, #2
    ret
    .size relativeTarget, .-relativeTarget
)");
#endif
asm(R"(
    .text
    .p2align 2
    .globl btiTarget
    .type btiTarget, %function
btiTarget:
    hint #34 // bti c
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    add w0, w0, #3
    ldp x29, x30, [sp], #16
    ret
    .globl btiTargetEnd
btiTargetEnd:
    .size btiTarget, .-btiTarget

    .p2align 2
    .globl pacTarget
    .type pacTarget, %function
pacTarget:
    hint #25 // paciasp
    stp x29, x30, [sp, #-16]!
    mov x29, sp
    add w0, w0, #4
    ldp x29, x30, [sp], #16
    hint #29 // autiasp
    ret
    .globl pacTargetEnd
pacTargetEnd:
    .size pacTarget, .-pacTarget

    .p2align 2
    .globl btiRelativeTarget
    .type btiRelativeTarget, %function
btiRelativeTarget:
    hint #34 // bti c
    adrp x1, btiRelativeTarget
    add w0, w0, #5
    ret
    .size btiRelativeTarget, .-btiRelativeTarget
)");

namespace {
int failures = 0;
Arm64Hook hook;
// qemu's /proc/self/smaps leaves out the bt flag, so the hook cannot see a guard there.
bool smapsShowsBti = false;

void expect(bool condition, const char* what) {
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

int hooked(int value) {
    return reinterpret_cast<int (*)(int)>(hook.original())(value) * 10;
}

// After a patch to a guarded page: the hook has kept the guard where /proc shows it, and
// elsewhere the test puts it back, so the calls that follow still land under BTI.
void stillGuarded(void* function) {
    const long pageSize = sysconf(_SC_PAGESIZE);
    if (smapsShowsBti)
        expect(Arm64Hook::guarded(function), "patching keeps the page guarded");
    else
        mprotect(function, pageSize, PROT_READ | PROT_EXEC | PROT_BTI);
}

// Through a volatile pointer, so every call goes to the patched code.
void hooksAndRestores(int (*target)(int), int added, const char* name, bool guardedPage = false) {
    int (*volatile call)(int) = target;
    expect(call(1) == 1 + added, name);
    if (!hook.install(reinterpret_cast<void*>(target), reinterpret_cast<void*>(hooked))) {
        fprintf(stderr, "FAIL: %s: %s\n", name, hook.error());
        ++failures;
        return;
    }
    if (guardedPage)
        stillGuarded(reinterpret_cast<void*>(target));
    expect(call(1) == (1 + added) * 10, name);
    expect(call(4) == (4 + added) * 10, name);
    expect(!hook.install(reinterpret_cast<void*>(target), reinterpret_cast<void*>(hooked)), "a second install is refused");
    hook.remove();
    if (guardedPage)
        stillGuarded(reinterpret_cast<void*>(target));
    expect(call(1) == 1 + added, name);
    expect(hook.original() == nullptr, "a removed hook has no original");
}

// A copy of a position-independent function on a page of its own, BTI-guarded when the CPU and
// kernel allow it, as a build with branch protection maps its code.
using Target = int (*)(int);
Target guardedCopy(Target function, const char* end, bool* isGuarded) {
    const long pageSize = sysconf(_SC_PAGESIZE);
    void* page = mmap(nullptr, pageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED)
        return nullptr;
    const auto* begin = reinterpret_cast<const char*>(function);
    memcpy(page, begin, end - begin);
    __builtin___clear_cache(static_cast<char*>(page), static_cast<char*>(page) + (end - begin));
    *isGuarded = mprotect(page, pageSize, PROT_READ | PROT_EXEC | PROT_BTI) == 0;
    if (!*isGuarded)
        mprotect(page, pageSize, PROT_READ | PROT_EXEC);
    return reinterpret_cast<Target>(page);
}

void refuses(int (*target)(int), int added, const char* name) {
    int (*volatile call)(int) = target;
    expect(!hook.install(reinterpret_cast<void*>(target), reinterpret_cast<void*>(hooked)), name);
    expect(call(1) == 1 + added, name);
}
}

int main() {
    expect(!Arm64Hook::pcRelative(0xA9BF7BFDu), "stp x29, x30, [sp, #-16]! moves");
    expect(!Arm64Hook::pcRelative(0x910003FDu), "mov x29, sp moves");
    expect(!Arm64Hook::pcRelative(Arm64Hook::PacIASP), "paciasp moves");
    expect(!Arm64Hook::pcRelative(Arm64Hook::BtiC), "bti c moves");
    expect(Arm64Hook::pcRelative(0x90000001u), "adrp is refused");
    expect(Arm64Hook::pcRelative(0x10000001u), "adr is refused");
    expect(Arm64Hook::pcRelative(0x14000010u), "b is refused");
    expect(Arm64Hook::pcRelative(0x94000010u), "bl is refused");
    expect(Arm64Hook::pcRelative(0x54000040u), "b.eq is refused");
    expect(Arm64Hook::pcRelative(0xB4000040u), "cbz is refused");
    expect(Arm64Hook::pcRelative(0x36000040u), "tbz is refused");
    expect(Arm64Hook::pcRelative(0x58000050u), "ldr literal is refused");
    expect(Arm64Hook::branch(0x1000, 0x2000) == 0x14000400u, "branch encodes forward offsets");
    expect(Arm64Hook::branch(0x2000, 0x1000) == 0x17FFFC00u, "branch encodes backward offsets");

#if !defined(__ARM_FEATURE_BTI_DEFAULT)
    hooksAndRestores(plainTarget, 1, "an ordinary prologue is hooked");
    refuses(relativeTarget, 2, "a PC-relative start is refused and left alone");
#endif
    hooksAndRestores(btiTarget, 3, "a bti c pad stays and the next instruction is hooked");
    hooksAndRestores(pacTarget, 4, "a paciasp pad stays and the return address is re-signed");
    refuses(btiRelativeTarget, 5, "a PC-relative instruction after the pad is refused");

    bool isGuarded = false;
    const Target guardedBti = guardedCopy(btiTarget, btiTargetEnd, &isGuarded);
    const Target guardedPac = guardedCopy(pacTarget, pacTargetEnd, &isGuarded);
    expect(guardedBti && guardedPac, "copies of the padded functions are mapped");
    if (!isGuarded) {
        printf("arm64hook: no BTI here, guarded pages not tested\n");
    } else if (guardedBti && guardedPac) {
        smapsShowsBti = Arm64Hook::guarded(reinterpret_cast<void*>(guardedBti));
        if (!smapsShowsBti)
            printf("arm64hook: /proc/self/smaps does not show BTI here (qemu), the test re-guards pages itself\n");
        expect(!Arm64Hook::guarded(reinterpret_cast<void*>(&hook)), "a data page reads as unguarded");
        hooksAndRestores(guardedBti, 3, "a function on a guarded page is hooked past its bti c", true);
        hooksAndRestores(guardedPac, 4, "a function on a guarded page is hooked past its paciasp", true);
    }

    if (failures == 0)
        printf("arm64hook: all checks passed\n");
    return failures == 0 ? 0 : 1;
}
