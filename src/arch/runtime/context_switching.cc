// Copyright 2010-2014 RethinkDB, all rights reserved.

#ifdef _WIN32

#include <tuple>

#include "windows.hpp"
#include "errors.hpp"
#include "arch/runtime/context_switching.hpp"
#include "arch/compiler.hpp"
#include "logger.hpp"

// Some declarations used to measure the stack size
// from http://undocumented.ntinternals.net/

#include <Ntsecapi.h>

typedef struct {
    PVOID UniqueProcess;
    PVOID UniqueThread;
} CLIENT_ID;

typedef LONG KPRIORITY;

typedef struct {
    NTSTATUS ExitStatus;
    PVOID TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION;

typedef NTSYSAPI NTSTATUS
(NTAPI *NtReadVirtualMemory_t)(IN HANDLE               ProcessHandle,
                               IN PVOID                BaseAddress,
                               OUT PVOID               Buffer,
                               IN ULONG                NumberOfBytesToRead,
                               OUT PULONG              NumberOfBytesReaded);

enum THREADINFOCLASS { ThreadBasicInformation = 0 };

typedef NTSTATUS
(WINAPI *NtQueryInformationThread_t)(HANDLE          ThreadHandle,
                                     THREADINFOCLASS ThreadInformationClass,
                                     PVOID           ThreadInformation,
                                     ULONG           ThreadInformationLength,
                                     PULONG          ReturnLength);

fiber_context_ref_t::~fiber_context_ref_t() {
    rassert(fiber == nullptr, "leaking a fiber");
}

THREAD_LOCAL void* thread_initial_fiber = nullptr;

void coro_initialize_for_thread() {
    if (thread_initial_fiber == nullptr) {
        thread_initial_fiber = ConvertThreadToFiber(nullptr);
        guarantee_winerr(thread_initial_fiber != nullptr, "ConvertThreadToFiber failed");
    }
}

void save_stack_info(fiber_stack_t *stack) {
    static HMODULE ntdll = LoadLibrary("ntdll.dll");
    static NtQueryInformationThread_t NtQueryInformationThread =
        reinterpret_cast<NtQueryInformationThread_t>(
            GetProcAddress(ntdll, "NtQueryInformationThread"));
    static NtReadVirtualMemory_t NtReadVirtualMemory =
        reinterpret_cast<NtReadVirtualMemory_t>(
            GetProcAddress(ntdll, "NtReadVirtualMemory"));
    static bool warned = false;
    if (NtQueryInformationThread == nullptr ||
        NtReadVirtualMemory == nullptr) {
        if (!warned) {
            logWRN("save_stack_info: GetProcAddress failed");
            warned = true;
        }
        stack->has_stack_info = false;
        return;
    }
    THREAD_BASIC_INFORMATION info;
    NTSTATUS res = NtQueryInformationThread(GetCurrentThread(),
                                            ThreadBasicInformation,
                                            &info,
                                            sizeof(info),
                                            nullptr);
    if (res != 0) {
        logWRN("NtQueryInformationThread failed: %s",
               winerr_string(LsaNtStatusToWinError(res)).c_str());
        stack->has_stack_info = false;
        return;
    }
    NT_TIB tib;
    res = NtReadVirtualMemory(GetCurrentProcess(),
                              info.TebBaseAddress,
                              &tib,
                              sizeof(tib),
                              nullptr);
    if (res != 0) {
        logWRN("NtReadVirtualMemory failed: %s",
               winerr_string(LsaNtStatusToWinError(res)).c_str());
        stack->has_stack_info = false;
        return;
    }
    stack->stack_base = reinterpret_cast<char*>(tib.StackBase);
    stack->stack_limit = reinterpret_cast<char*>(tib.StackLimit);
    stack->has_stack_info = true;
}

fiber_stack_t::fiber_stack_t(void(*initial_fun)(void), size_t stack_size) {
    auto tuple = std::make_tuple(initial_fun, this, GetCurrentFiber());
    typedef decltype(tuple) data_t;
    context.fiber = CreateFiberEx(
        stack_size,
        stack_size,
        0, // don't switch floating-point state
        [](void* data) {
            void (*initial_fun)();
            fiber_stack_t *self;
            void *parent_fiber;
            std::tie(initial_fun, self, parent_fiber) = *reinterpret_cast<data_t*>(data);
            save_stack_info(self);
            SwitchToFiber(parent_fiber);
            initial_fun();
        },
        reinterpret_cast<void*>(&tuple));
    guarantee_winerr(context.fiber != nullptr, "CreateFiber failed");
    SwitchToFiber(context.fiber);
}

fiber_stack_t::~fiber_stack_t() {
    DeleteFiber(context.fiber);
    context.fiber = nullptr;
}

bool fiber_stack_t::address_in_stack(const void *addr) const {
    if (!has_stack_info) {
        rassert(has_stack_info, "could not determine stack limits");
        return true;
    }
    const char *a = reinterpret_cast<const char*>(addr);
    return stack_limit < a && a <= stack_base;
}

bool fiber_stack_t::address_is_stack_overflow(const void *addr) const {
    if (!has_stack_info) {
        rassert(has_stack_info, "could not determine stack limits");
        return false;
    }
    return reinterpret_cast<const char*>(addr) <= stack_limit;
}

size_t fiber_stack_t::free_space_below(const void *addr) const {
    guarantee(address_in_stack(addr));
    return reinterpret_cast<const char*>(addr) - stack_limit;
}

void context_switch(fiber_context_ref_t *curr_context_out, fiber_context_ref_t *dest_context_in) {
    rassert(curr_context_out->fiber == nullptr, "switching from non-null context: %p", curr_context_out->fiber);
    rassert(dest_context_in->fiber != nullptr);
    curr_context_out->fiber = GetCurrentFiber();
    void *dest_context = dest_context_in->fiber;
    dest_context_in->fiber = nullptr;
    SwitchToFiber(dest_context);
}

#else

#include "arch/runtime/context_switching.hpp"

#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

#ifndef NDEBUG
#include <cxxabi.h>   // For __cxa_current_exception_type (see below)
#endif

#ifdef VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "arch/runtime/thread_pool.hpp"
#include "arch/runtime/coroutines.hpp"
#include "arch/io/concurrency.hpp"
#include "containers/scoped.hpp"
#include "errors.hpp"
#include "math.hpp"
#include "utils.hpp"

/* We have a custom implementation of `swapcontext()` that doesn't swap the
floating-point registers, the SSE registers, or the signal mask. This is for
performance reasons.

Our custom context-switching code is derived from GLibC, which is covered by the
LGPL. */

artificial_stack_context_ref_t::artificial_stack_context_ref_t() : pointer(nullptr) { }

artificial_stack_context_ref_t::~artificial_stack_context_ref_t() {
    rassert(is_nil(), "You're leaking a context.");
}

bool artificial_stack_context_ref_t::is_nil() {
    return pointer == nullptr;
}

artificial_stack_t::artificial_stack_t(void (*initial_fun)(void), size_t _stack_size)
    : stack(_stack_size), stack_size(_stack_size), overflow_protection_enabled(false) {

    // Tell the operating system that it can unmap the stack space
    // (except for the first page, which we are definitely going to need).
    // This is an optimization to keep memory consumption in check.
    guarantee(stack_size >= static_cast<size_t>(getpagesize()));
    madvise(stack.get(), stack_size - getpagesize(), MADV_DONTNEED);

    // Register our stack with Valgrind so that it understands what's going on
    // and doesn't create spurious errors.
#ifdef VALGRIND
    valgrind_stack_id = VALGRIND_STACK_REGISTER(stack.get(), (intptr_t)stack.get() + stack_size);
#endif

    // Setup the new stack (grows downwards).
    //
    // +---------------+ <- (1) initial sp.
    // |               |
    // +---------------+ <- (2) sp aligned as needed.
    // |               |
    // +---------------+ <- (3) sp after allocating (fake) caller stack frame;
    // |  initial_fun  |    will be the stack pointer at entry to initial_fun.
    // +---------------+ <- (4) sp after pushing the return address, initial_fun.
    // |               |
    // |  callee-save  |
    // |   registers   |
    // |               |
    // +---------------+ <- (5) sp after allocating space for callee-save registers.
    //
    // When swapcontext is called it will pop the callee-save registers off of the
    // stack (may be junk), pop initial_fun off the stack and then jump to initial_fun.
    uintptr_t *sp;

    // (1) initialize sp to point at the top of the stack.
    sp = reinterpret_cast<uintptr_t *>(uintptr_t(stack.get()) + stack_size);

    // (2) align sp to meet platform ABI requirements.
    // Note: not all platforms require 16-byte alignment, but it is easier to do it
    // everywhere.
    sp = reinterpret_cast<uintptr_t *>(uintptr_t(sp) & static_cast<uintptr_t>(-16L));

    // (3) allocate caller stack frame.
#if defined(__i386__) || defined(__x86_64__)
    // The x86-64 ABI requires the stack pointer to always be 16-byte-aligned at
    // function calls. That is, "(%rsp - 8) is always a multiple of 16 when control
    // is transferred to the function entry point".
    const size_t min_frame = 1;
#elif defined(__s390x__)
    // The s390x ABI requires a 160-byte caller allocated register save area.
    const size_t min_frame = 20;
#elif defined(__arm__)
    // This slot is used to store r12.
    const size_t min_frame = 1;
#elif defined(__powerpc64__)
    const size_t min_frame = 4;
#endif
    // Zero the caller stack frame. Prevents Valgrind complaining about uninitialized
    // value errors when throwing an uncaught exception.
    for (size_t i = 0; i < min_frame; i++) {
        sp--;
        *sp = 0;
    }

    // (4) write the return address to the stack.
    sp--;
    *sp = reinterpret_cast<uintptr_t>(initial_fun);

    // (5) allocate space for callee-save registers. These will be popped off the
    // stack by swapcontext but initial_fun should ignore their contents so we
    // don't initialize them to anything.
    // See swapcontext for more information about the way registers are restored.
#if defined(__i386__)
    sp -= 4; // esi, edi, ebx and ebp.
#elif defined(__x86_64__)
    sp -= 6; // r12-r15, rbx and rbp.
#elif defined(__arm__)
    // Note: r12 is also stored, in the 'caller frame' slot above the return
    // address.
    sp -= 8; // r4-r11.
#elif defined(__s390x__)
    sp -= 16; // r6-r13 and f8-f15.
#elif defined (__powerpc64__)
    // Considering the performance, non volatile floating point and vector
    // registers are saved and restored for powerpc.  There are no failures
    // seen during context switch on ubuntu/RHEL powerpc64le systems.
    sp -= 20; // r14-r31, toc, cr.
#else
#error "Unsupported architecture."
#endif

    // Save stack pointer.
    context.pointer = sp;

    // Our coroutines never return, so we don't put anything else on the stack.
}

artificial_stack_t::~artificial_stack_t() {

    /* `context` must now point to what it was when we were created. If it
    doesn't, that means we're deleting the stack but the corresponding context
    is still "out there" somewhere. */
    rassert(!context.is_nil(), "we never got our context back");
    rassert(address_in_stack(context.pointer), "we got the wrong context back");

    /* Set `context.pointer` to nil to avoid tripping an assertion in its
    destructor. */
    context.pointer = nullptr;

    /* Tell Valgrind the stack is no longer in use */
#ifdef VALGRIND
// Disable GCC diagnostic for VALGRIND_STACK_DEREGISTER.  The flag
// exists only in GCC 4.6 and greater.  See the "reenable" comment
// after the destructor.
#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
    VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
// Reenable GCC diagnostic for VALGRIND_STACK_DEREGISTER.
#if defined(__GNUC__) && (100 * __GNUC__ + __GNUC_MINOR__ >= 406)
#pragma GCC diagnostic pop
#endif
#endif

    /* Undo protections changes */
    disable_overflow_protection();

    /* Return the memory to the operating system right away. This makes
    sense because we keep our own cache of coroutine stacks around and
    don't need to rely on the allocator to optimize for the case of
    us quickly re-allocating an object of the same size.
    On OS X we use MADV_FREE. On Linux MADV_FREE is not available,
    and we use MADV_DONTNEED instead. */
#ifdef __MACH__
    madvise(stack.get(), stack_size, MADV_FREE);
#else
    madvise(stack.get(), stack_size, MADV_DONTNEED);
#endif
}

/* Wrapper around `mprotect` that checks the return code. */
void checked_mprotect_page(void *page_addr, int prot) {
    int res = mprotect(page_addr, getpagesize(), prot);
    if (res != 0) {
#ifdef __linux__
        if (get_errno() == ENOMEM) {
            crash("Failed to (un-)protect a coroutine stack (`mprotect` failed with "
                  "`ENOMEM`). Try increasing the value of `/proc/sys/vm/max_map_count`.");
        }
#endif
        crash("Failed to (un-)protect a coroutine stack. `mprotect` failed with error "
              "code %d.", get_errno());
    }
}

void artificial_stack_t::enable_overflow_protection() {
    /* Protect the end of the stack so that we crash when we get a stack
    overflow instead of corrupting memory. */
    if (overflow_protection_enabled) {
        return;
    }
    /* OS X Instruments hangs when running with mprotect and having object identification
    enabled. We don't need it for THREADED_COROUTINES anyway, so don't use it then. */
#ifndef THREADED_COROUTINES
    checked_mprotect_page(stack.get(), PROT_NONE);
    overflow_protection_enabled = true;
#endif
}

void artificial_stack_t::disable_overflow_protection() {
    if (!overflow_protection_enabled) {
        return;
    }
#ifndef THREADED_COROUTINES
    checked_mprotect_page(stack.get(), PROT_READ | PROT_WRITE);
    overflow_protection_enabled = false;
#endif
}

bool artificial_stack_t::address_in_stack(const void *addr) const {
    return reinterpret_cast<uintptr_t>(addr) >=
            reinterpret_cast<uintptr_t>(get_stack_bound())
        && reinterpret_cast<uintptr_t>(addr) <
            reinterpret_cast<uintptr_t>(get_stack_base());
}

bool artificial_stack_t::address_is_stack_overflow(const void *addr) const {
    void *addr_base =
        reinterpret_cast<void *>(floor_aligned(reinterpret_cast<uintptr_t>(addr),
                                               getpagesize()));
    return get_stack_bound() == addr_base;
}

size_t artificial_stack_t::free_space_below(const void *addr) const {
    rassert(address_in_stack(addr) && !address_is_stack_overflow(addr));

    // The bottom page is protected and used to detect stack overflows. Everything
    // above that is usable space.
    const uintptr_t lowest_valid_address =
        reinterpret_cast<uintptr_t>(get_stack_bound()) + getpagesize();

    // This check is pretty much equivalent to checking `address_is_stack_overflow`, but
    // we avoid the extra call to `getpagesize()` inside of `address_is_stack_overflow`
    // in release mode.
    guarantee(lowest_valid_address <= reinterpret_cast<uintptr_t>(addr));

    return reinterpret_cast<uintptr_t>(addr) - lowest_valid_address;
}

extern "C" {
// `lightweight_swapcontext` is defined in assembly further down.  If we didn't add the
// asm("_lightweight_swapcontext") here, we'd have to conditionally compile the symbol name in the
// assembly directive to include the underscore on OS X and certain other platforms, or use the
// -fleading-underscore which would be trickier and riskier.
extern void lightweight_swapcontext(void **current_pointer_out, void *dest_pointer)
    asm("_lightweight_swapcontext");
}

void context_switch(artificial_stack_context_ref_t *current_context_out, artificial_stack_context_ref_t *dest_context_in) {

    /* In the catch-clause of a C++ exception handler, executing a `throw` with
    no parameter will re-throw the exception that we caught. This works even if
    the parameter-less `throw` is in a function called by the exception handler.
    C++ accomplishes this by storing the current exception in a thread-local
    variable. We have no way to save and restore the value of this variable when
    switching contexts, so instead we just ban context switching from within a
    `catch`-block. We use the non-standard GCC-only interface "cxxabi.h" to
    figure out if we're in the catch clause of an exception handler. In C++0x we
    will be able to use std::current_exception() instead. */
    rassert(!abi::__cxa_current_exception_type(), "It's not safe to switch "
        "coroutine stacks in the catch-clause of an exception handler.");

    rassert(current_context_out->is_nil(), "that variable already holds a context");
    rassert(!dest_context_in->is_nil(), "cannot switch to nil context");

    /* `lightweight_swapcontext()` won't set `dest_context_in->pointer` to nullptr,
    so we have to do that ourselves. */
    void *dest_pointer = dest_context_in->pointer;
    dest_context_in->pointer = nullptr;

    lightweight_swapcontext(&current_context_out->pointer, dest_pointer);
}

asm(
#if defined(__i386__) || defined(__x86_64__) || defined(__arm__) || defined (__s390x__) || defined (__powerpc64__)
// We keep architecture-specific code interleaved in order to enforce commonality.
#if defined(__x86_64__)
#if defined(__LP64__) || defined(__LLP64__)
// Pointers are of the right size
#else
// Having non-native-sized pointers makes things very messy.
#error "Non-native pointer size."
#endif
#endif // defined(__x86_64__)
".text\n"
"_lightweight_swapcontext:\n"

#if defined(__i386__)
    /* `current_pointer_out` is in `4(%ebp)`. `dest_pointer` is in `8(%ebp)`. */
#elif defined(__x86_64__)
    /* `current_pointer_out` is in `%rdi`. `dest_pointer` is in `%rsi`. */
#elif defined(__arm__)
    /* `current_pointer_out` is in `r0`. `dest_pointer` is in `r1` */
#elif defined(__s390x_)
    /* `current_pointer_out` is in `%r2`. `dest_pointer` is in `%r3`. */
#elif defined(__powerpc64__)
    /* `current_pointer_out` is in `r3`. `dest_pointer` is in `r4` */
#endif

    // Save preserved registers.
#if defined(__i386__)
    // Preserve esi, edi, ebx, and ebp. The return address is already on the stack.
    "push %esi\n"
    "push %edi\n"
    "push %ebx\n"
    "push %ebp\n"
#elif defined(__x86_64__)
    // Preserve r12-r15, rbx, and rbp. The return address is already on the stack.
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbx\n"
    "pushq %rbp\n"
#elif defined(__arm__)
    // Preserve r4-r12 and the return address (r14). For consistency with x86 r12 is
    // pushed first, followed by r14 and then r4-r11.
    "push {r12}\n"
    "push {r14}\n"
    "push {r4-r11}\n"
#elif defined(__s390x__)
    // Preserve r6-r13, the return address (r14), and f8-f15.
    "aghi %r15, -136\n"
    "stmg %r6, %r14, 64(%r15)\n"
    "std %f8, 0(%r15)\n"
    "std %f9, 8(%r15)\n"
    "std %f10, 16(%r15)\n"
    "std %f11, 24(%r15)\n"
    "std %f12, 32(%r15)\n"
    "std %f13, 40(%r15)\n"
    "std %f14, 48(%r15)\n"
    "std %f15, 56(%r15)\n"
#elif defined(__powerpc64__)
    "addi 1, 1, -(21*8)\n"
    "std 2, (8*0)(1)\n"
    "std 14, (8*1)(1)\n"
    "std 15, (8*2)(1)\n"
    "std 16, (8*3)(1)\n"
    "std 17, (8*4)(1)\n"
    "std 18, (8*5)(1)\n"
    "std 19, (8*6)(1)\n"
    "std 20, (8*7)(1)\n"
    "std 21, (8*8)(1)\n"
    "std 22, (8*9)(1)\n"
    "std 23, (8*10)(1)\n"
    "std 24, (8*11)(1)\n"
    "std 25, (8*12)(1)\n"
    "std 26, (8*13)(1)\n"
    "std 27, (8*14)(1)\n"
    "std 28, (8*15)(1)\n"
    "std 29, (8*16)(1)\n"
    "std 30, (8*17)(1)\n"
    "std 31, (8*18)(1)\n"
    "mfcr 0\n"
    "std 0, (8*19)(1)\n"
    "mflr 0\n"
    "std 0, (8*20)(1)\n"
#endif

    /* Save old stack pointer. */
#if defined(__i386__)
    /* i386 passes arguments on the stack. We add ((number of things pushed)+1)*(sizeof(void*)) to esp in order to get the first argument. */
    "mov 20(%esp), %ecx\n"
    /* We then copy the stack pointer into the space indicated by the first argument. */
    "mov %esp, (%ecx)\n"
#elif defined(__x86_64__)
    /* On amd64, the first argument comes from rdi. */
    "movq %rsp, (%rdi)\n"
#elif defined(__arm__)
    /* On ARM, the first argument is in `r0`. `r13` is the stack pointer. */
    "str r13, [r0]\n"
#elif defined(__s390x__)
    /* On s390x, the first argument is in r2. r15 is the stack pointer. */
    "stg %r15, 0(%r2)\n"
#elif defined(__powerpc64__)
    "std  1, 0(3)\n"
#endif

    /* Load the new stack pointer and the preserved registers. */
#if defined(__i386__)
    /* i386 passes arguments on the stack. We add ((number of things pushed)+1)*(sizeof(void*)) to esp in order to get the first argument. */
    "mov 24(%esp), %esi\n"
    /* We then copy the second argument to be the new stack pointer. */
    "mov %esi, %esp\n"
#elif defined(__x86_64__)
    /* On amd64, the second argument comes from rsi. */
    "movq %rsi, %rsp\n"
#elif defined(__arm__)
    /* On ARM, the second argument is in `r1` */
    "mov r13, r1\n"
#elif defined(__s390x__)
    /* On s390x, the second argument is in r3 */
    "lgr %r15, %r3\n"
#elif defined(__powerpc64__)
    "mr 1, 4\n"
#endif

#if defined(__i386__)
    "pop %ebp\n"
    "pop %ebx\n"
    "pop %edi\n"
    "pop %esi\n"
#elif defined(__x86_64__)
    "popq %rbp\n"
    "popq %rbx\n"
    "popq %r15\n"
    "popq %r14\n"
    "popq %r13\n"
    "popq %r12\n"
#elif defined(__arm__)
    "pop {r4-r11}\n"
    "pop {r14}\n"
    "pop {r12}\n"
#elif defined(__s390x__)
    "lmg %r6, %r14, 64(%r15)\n"
    "ld %f8, 0(%r15)\n"
    "ld %f9, 8(%r15)\n"
    "ld %f10, 16(%r15)\n"
    "ld %f11, 24(%r15)\n"
    "ld %f12, 32(%r15)\n"
    "ld %f13, 40(%r15)\n"
    "ld %f14, 48(%r15)\n"
    "ld %f15, 56(%r15)\n"
    "aghi %r15, 136\n"
#elif defined(__powerpc64__)
    "ld 2, (8*0)(1)\n"
    "ld 14, (8*1)(1)\n"
    "ld 15, (8*2)(1)\n"
    "ld 16, (8*3)(1)\n"
    "ld 17, (8*4)(1)\n"
    "ld 18, (8*5)(1)\n"
    "ld 19, (8*6)(1)\n"
    "ld 20, (8*7)(1)\n"
    "ld 21, (8*8)(1)\n"
    "ld 22, (8*9)(1)\n"
    "ld 23, (8*10)(1)\n"
    "ld 24, (8*11)(1)\n"
    "ld 25, (8*12)(1)\n"
    "ld 26, (8*13)(1)\n"
    "ld 27, (8*14)(1)\n"
    "ld 28, (8*15)(1)\n"
    "ld 29, (8*16)(1)\n"
    "ld 30, (8*17)(1)\n"
    "ld 31, (8*18)(1)\n"
    "ld 0, (8*19)(1)\n"
    "mtcr 0\n"
    "ld 0, (8*20)(1)\n"
    "mtlr 0\n"
    "addi 1, 1, (8*21)\n"
#endif

#if defined(__i386__) || defined(__x86_64__)
    /* The following ret should return to the address set with
    `artificial_stack_t()` or with the previous `lightweight_swapcontext`. The
    instruction pointer is saved on the stack from the previous call (or
    initialized with `artificial_stack_t()`). */
    "ret\n"
#elif defined(__arm__)
    /* Above, we popped `LR` (`r14`) off the stack, so the bx instruction will
    jump to the correct return address. */
    "bx r14\n"
#elif defined(__s390x__)
    /* Above, we popped the return address (r14) off the stack. */
    "br %r14\n"
#elif defined(__powerpc64__)
    "blr\n"
#endif

#else
#error "Unsupported architecture."
#endif
);








/* Threaded version of context_switching */



static system_mutex_t virtual_thread_mutexes[MAX_THREADS];

void context_switch(threaded_context_ref_t *current_context,
                    threaded_context_ref_t *dest_context) {
    guarantee(current_context != nullptr);
    guarantee(dest_context != nullptr);

    bool is_scheduler = false;
    if (!current_context->lock.has()) {
        // This must be the scheduler. We need to acquire a lock.
        is_scheduler = true;
        current_context->lock.init(new system_mutex_t::lock_t(
            &virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]));
    }

    dest_context->rethread_to_current();
    dest_context->cond.signal();
    // ==== dest_context runs here ====
    current_context->wait();
    // ==== now back on current_context ====
    if (is_scheduler) {
        // We must not hold the lock, or we will get deadlocks because the scheduler
        // can end up waiting on a system event. If another thread then
        // wants to re-thread to our thread, it will be unable to do so.
        current_context->lock.reset();
    }
}

void threaded_context_ref_t::rethread_to_current() {
    int32_t old_thread = my_thread_id;
    store_virtual_thread();
    if (my_thread_id == old_thread) {
        return;
    }

    do_rethread = true;
}

void threaded_context_ref_t::wait() {
    cond.wait(&virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]);
    if (do_shutdown) {
        lock.reset();
        pthread_exit(nullptr);
    }
    if (do_rethread) {
        restore_virtual_thread();
        // Re-lock to a different thread mutex
        lock.reset();
        lock.init(new system_mutex_t::lock_t(
            &virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]));
        do_rethread = false;
    }
}

void threaded_context_ref_t::restore_virtual_thread() {
    // Fake our thread
    linux_thread_pool_t::set_thread_pool(my_thread_pool);
    linux_thread_pool_t::set_thread_id(my_thread_id);
    linux_thread_pool_t::set_thread(my_thread);
}

void threaded_context_ref_t::store_virtual_thread() {
    my_thread_pool = linux_thread_pool_t::get_thread_pool();
    my_thread_id = linux_thread_pool_t::get_thread_id();
    my_thread = linux_thread_pool_t::get_thread();
}

threaded_stack_t::threaded_stack_t(void (*initial_fun_)(void), size_t stack_size) :
    initial_fun(initial_fun_),
    dummy_stack(initial_fun, stack_size) {

    scoped_ptr_t<system_mutex_t::lock_t> possible_lock_acq;
    if (!coro_t::self()) {
        // If we are not in a coroutine, we have to acquire the lock first.
        // (coroutines would already hold the lock)
        possible_lock_acq.init(new system_mutex_t::lock_t(
            &virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]));
    }

    int result = pthread_create(&thread,
                                nullptr,
                                threaded_stack_t::internal_run,
                                reinterpret_cast<void *>(this));
    guarantee_xerr(result == 0, result, "Could not create thread: %i", result);
    // Wait for the thread to start
    launch_cond.wait(&virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]);
}

threaded_stack_t::~threaded_stack_t() {
    // Our coroutines never terminate. We have to tell the thread to shut down
    // before joining it.

    scoped_ptr_t<system_mutex_t::lock_t> possible_lock_acq;
    if (!coro_t::self()) {
        // If we are not in a coroutine, we have to acquire the lock first.
        // (coroutines would already hold the lock)
        possible_lock_acq.init(new system_mutex_t::lock_t(
            &virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]));
    }

    // Calling this destructor is safe only if we are on the same virtual thread as
    // the thread that we are destroying. This ensures that it is not currently
    // active and not holding a lock on the virtual thread mutex.
    guarantee(!context.do_rethread
              && context.my_thread_id == linux_thread_pool_t::get_thread_id());

    // Tell the thread to shut down.
    context.do_shutdown = true;
    context.cond.signal();

    // We now have to temporarily release our lock to let the thread
    // terminate.
    if (coro_t::self()) {
#ifdef THREADED_COROUTINES
        threaded_context_ref_t &our_context = static_cast<threaded_context_ref_t &>(
                coro_t::self()->get_stack()->context);
        our_context.lock.reset();
#else
        crash("Threaded stack destructed from a coroutine without "
              "THREADED_COROUTINES defined.");
#endif
    } else {
        possible_lock_acq.reset();
    }

    // Wait for the thread to shut down
    int result = pthread_join(thread, nullptr);
    guarantee_xerr(result == 0, result, "Could not join with thread: %i", result);

    // ... and re-acquire the lock, if we are in a coroutine.
    if (coro_t::self()) {
#ifdef THREADED_COROUTINES
        threaded_context_ref_t &our_context = static_cast<threaded_context_ref_t &>(
                coro_t::self()->get_stack()->context);
        our_context.lock.init(new system_mutex_t::lock_t(
            &virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]));
#endif
    }
}

void *threaded_stack_t::internal_run(void *p) {
    threaded_stack_t *parent = reinterpret_cast<threaded_stack_t *>(p);

    parent->context.restore_virtual_thread();

    parent->context.lock.init(new system_mutex_t::lock_t(
        &virtual_thread_mutexes[linux_thread_pool_t::get_thread_id()]));
    // Notify our parent that we have been created
    parent->launch_cond.signal();

    parent->context.wait();
    parent->initial_fun();
    return nullptr;
}

bool threaded_stack_t::address_in_stack(const void *addr) const {
    return reinterpret_cast<uintptr_t>(addr) >=
            reinterpret_cast<uintptr_t>(get_stack_bound())
        && reinterpret_cast<uintptr_t>(addr) <
            reinterpret_cast<uintptr_t>(get_stack_base());
}

bool threaded_stack_t::address_is_stack_overflow(const void *addr) const {
    void *addr_base =
        reinterpret_cast<void *>(floor_aligned(reinterpret_cast<uintptr_t>(addr),
                                               getpagesize()));
    return get_stack_bound() == addr_base;
}

void *threaded_stack_t::get_stack_base() const {
    void *stackaddr;
    size_t stacksize;
    get_stack_addr_size(&stackaddr, &stacksize);
    uintptr_t base = reinterpret_cast<uintptr_t>(stackaddr)
                     + static_cast<uintptr_t>(stacksize);
    return reinterpret_cast<void *>(base);
}

void *threaded_stack_t::get_stack_bound() const {
    void *stackaddr;
    size_t stacksize;
    get_stack_addr_size(&stackaddr, &stacksize);
    return stackaddr;
}

size_t threaded_stack_t::free_space_below(const void *addr) const {
    guarantee(address_in_stack(addr) && !address_is_stack_overflow(addr));
    // The bottom page is protected and used to detect stack overflows. Everything
    // above that is usable space.
    return reinterpret_cast<uintptr_t>(addr)
            - (reinterpret_cast<uintptr_t>(get_stack_bound()) + getpagesize());
}

void threaded_stack_t::get_stack_addr_size(void **stackaddr_out,
                                           size_t *stacksize_out) const {
#ifdef __MACH__
    // Implementation for OS X
    *stacksize_out = pthread_get_stacksize_np(thread);
    *stackaddr_out = reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np(thread))
        - static_cast<uintptr_t>(*stacksize_out));
#else
    // Implementation for Linux
    pthread_attr_t attr;
    int res = pthread_getattr_np(thread, &attr);
    guarantee_xerr(res == 0, res, "Unable to get pthread attributes");
    res = pthread_attr_getstack(&attr, stackaddr_out, stacksize_out);
    guarantee_xerr(res == 0, res, "Unable to get pthread stack attribute");
    res = pthread_attr_destroy(&attr);
    guarantee_xerr(res == 0, res, "Unable to destroy pthread attributes");
#endif
}

/* ^^^^ Threaded version of context_switching ^^^^ */

#endif // _WIN32
