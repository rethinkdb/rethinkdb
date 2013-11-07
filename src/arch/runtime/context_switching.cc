// Copyright 2010-2012 RethinkDB, all rights reserved.
#include "arch/runtime/context_switching.hpp"

#include <sys/mman.h>
#include <unistd.h>

#ifndef NDEBUG
#include <cxxabi.h>   // For __cxa_current_exception_type (see below)
#endif

#ifdef VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "utils.hpp"

/* We have a custom implementation of `swapcontext()` that doesn't swap the
floating-point registers, the SSE registers, or the signal mask. This is for
performance reasons.

Our custom context-switching code is derived from GLibC, which is covered by the
LGPL. */

context_ref_t::context_ref_t() : pointer(NULL) { }

context_ref_t::~context_ref_t() {
    rassert(is_nil(), "You're leaking a context.");
}

bool context_ref_t::is_nil() {
    return pointer == NULL;
}

artificial_stack_t::artificial_stack_t(void (*initial_fun)(void), size_t _stack_size)
    : stack_size(_stack_size) {
    /* Allocate the stack */
    stack = malloc_aligned(stack_size, getpagesize());

    /* Protect the end of the stack so that we crash when we get a stack
    overflow instead of corrupting memory. */
    mprotect(stack, getpagesize(), PROT_NONE);

    /* Register our stack with Valgrind so that it understands what's going on
    and doesn't create spurious errors */
#ifdef VALGRIND
    valgrind_stack_id = VALGRIND_STACK_REGISTER(stack, (intptr_t)stack + stack_size);
#endif

    /* Set up the stack... */

    uintptr_t *sp; /* A pointer into the stack. Note that uintptr_t is ideal since it points to something of the same size as the native word or pointer. */

    /* Start at the beginning. */
    sp = reinterpret_cast<uintptr_t *>(uintptr_t(stack) + stack_size);

    /* Align stack. The x86-64 ABI requires the stack pointer to always be
    16-byte-aligned at function calls. That is, "(%rsp - 8) is always a multiple
    of 16 when control is transferred to the function entry point". */
    sp = reinterpret_cast<uintptr_t *>(uintptr_t(sp) & static_cast<uintptr_t>(-16L));

    // Currently sp is 16-byte aligned.

    /* Set up the instruction pointer; this will be popped off the stack by ret (or popped
    explicitly, for ARM) in swapcontext once all the other registers have been "restored". */
    sp--;

    /* This seems to prevent Valgrind from complaining about uninitialized value
    errors when throwing an uncaught exception. My assembly-fu isn't strong
    enough to explain why, though. */
    /* Also, on ARM, this gets popped into `r12` */
    *sp = 0;

    sp--;

    // Subtracted 2*sizeof(uintptr_t), so sp is still double-word-size (16-byte for amd64) aligned.

    *sp = reinterpret_cast<uintptr_t>(initial_fun);

#if defined(__i386__)
    /* For i386, we are obligated (by the A.B.I. specification) to preserve esi, edi, ebx, ebp, and esp. We do not push esp onto the stack, though, since we will have needed to retrieve it anyway in order to get to the point on the stack from which we would pop. */
    sp -= 4;
#elif defined(__x86_64__)
    /* These registers (r12, r13, r14, r15, rbx, rbp) are going to be popped off
    the stack by swapcontext; they're callee-saved, so whatever happens to be in
    them will be ignored. */
    sp -= 6;
#elif defined(__arm__)
    /* We must preserve r4, r5, r6, r7, r8, r9, r10, and r11. Because we have to store the LR (r14) in swapcontext as well, we also store r12 in swapcontext to keep the stack double-word-aligned. However, we already accounted for both of those by decrementing sp twice above (once for r14 and once for r12, say). */
    sp -= 8;
#else
#error "Unsupported architecture."
#endif

    // Subtracted (multiple of 2)*sizeof(uintptr_t), so sp is still double-word-size (16-byte for amd64, 8-byte for i386 and ARM) aligned.

    /* Set up stack pointer. */
    context.pointer = sp;

    /* Our coroutines never return, so we don't put anything else on the stack.
    */
}

artificial_stack_t::~artificial_stack_t() {

    /* `context` must now point to what it was when we were created. If it
    doesn't, that means we're deleting the stack but the corresponding context
    is still "out there" somewhere. */
    rassert(!context.is_nil(), "we never got our context back");
    rassert(address_in_stack(context.pointer), "we got the wrong context back");

    /* Set `context.pointer` to nil to avoid tripping an assertion in its
    destructor. */
    context.pointer = NULL;

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
    mprotect(stack, getpagesize(), PROT_READ | PROT_WRITE);

    /* Release the stack we allocated */
    free(stack);
}

bool artificial_stack_t::address_in_stack(void *addr) {
    return (uintptr_t)addr >= (uintptr_t)stack &&
        (uintptr_t)addr < (uintptr_t)stack + stack_size;
}

bool artificial_stack_t::address_is_stack_overflow(void *addr) {
    void *base = reinterpret_cast<void *>(floor_aligned(uintptr_t(addr), getpagesize()));
    return stack == base;
}

extern "C" {
// `lightweight_swapcontext` is defined in assembly further down.  If we didn't add the
// asm("_lightweight_swapcontext") here, we'd have to conditionally compile the symbol name in the
// assembly directive to include the underscore on OS X and certain other platforms, or use the
// -fleading-underscore which would be trickier and riskier.
extern void lightweight_swapcontext(void **current_pointer_out, void *dest_pointer)
    asm("_lightweight_swapcontext");
}

void context_switch(context_ref_t *current_context_out, context_ref_t *dest_context_in) {

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

    /* `lightweight_swapcontext()` won't set `dest_context_in->pointer` to NULL,
    so we have to do that ourselves. */
    void *dest_pointer = dest_context_in->pointer;
    dest_context_in->pointer = NULL;

    lightweight_swapcontext(&current_context_out->pointer, dest_pointer);
}

asm(
#if defined(__i386__) || defined(__x86_64__) || defined(__arm__)
// We keep the i386, x86_64, and ARM stuff interleaved in order to enforce commonality.
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
#endif

    /* Save preserved registers (the return address is already on the stack). */
#if defined(__i386__)
    /* For i386, we must preserve esi, edi, ebx, ebp, and esp. */
    "push %esi\n"
    "push %edi\n"
    "push %ebx\n"
    "push %ebp\n"
#elif defined(__x86_64__)
    "pushq %r12\n"
    "pushq %r13\n"
    "pushq %r14\n"
    "pushq %r15\n"
    "pushq %rbx\n"
    "pushq %rbp\n"
#elif defined(__arm__)
    /* Note that we push `LR` (`r14`) since that's not implicitly done at a call on ARM. We include `r12` just to keep the stack double-word-aligned. The order here is really important, as it must match the way we set up the stack in artificial_stack_t::artificial_stack_t. For consistency with the other architectures, we push `r12` first, then `r14`, then the rest. */
    "push {r12}\n"
    "push {r14}\n"
    "push {r4-r11}\n"
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
#endif

#else
#error "Unsupported architecture."
#endif
);

