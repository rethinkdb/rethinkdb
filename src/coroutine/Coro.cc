/*
 Credits

    Originally based on Edgar Toernig's Minimalistic cooperative multitasking
    http://www.goron.de/~froese/
    reorg by Steve Dekorte and Chis Double
    Symbian and Cygwin support by Chis Double
    Linux/PCC, Linux/Opteron, Irix and FreeBSD/Alpha, ucontext support by Austin Kurahone
    FreeBSD/Intel support by Faried Nawaz
    Mingw support by Pit Capitain
    Visual C support by Daniel Vollmer
    Solaris support by Manpreet Singh
    Fibers support by Jonas Eschenburg
    Ucontext arg support by Olivier Ansaldi
    Ucontext x86-64 support by James Burgess and Jonathan Wright
    Russ Cox for the newer portable ucontext implementions.
    Daniel Ehrenberg for deleting almost all of that stuff

 Notes

    This is the system dependent coro code.
    Setup a jmp_buf so when we longjmp, it will invoke 'func' using 'stack'.
    Important: 'func' must not return!

    Usually done by setting the program counter and stack pointer of a new, empty stack.
    If you're adding a new platform, look in the setjmp.h for PC and SP members
    of the stack structure

    If you don't see those members, Kentaro suggests writting a simple
    test app that calls setjmp and dumps out the contents of the jmp_buf.
    (The PC and SP should be in jmp_buf->__jmpbuf).

    Using something like GDB to be able to peek into register contents right
    before the setjmp occurs would be helpful also.
 */

#include "Coro.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>
#include <sys/mman.h>
#include "arch/arch.hpp"
#include "coroutine/coroutines.hpp"

#ifdef VALGRIND
#include <valgrind/valgrind.h>
#define STACK_REGISTER(coro) \
{ \
    Coro *c = (coro); \
        c->valgrindStackId = VALGRIND_STACK_REGISTER( \
                                             Coro_stack(c), \
                                             (size_t)Coro_stack(c) + Coro_stackSize(c)); \
}

#define STACK_DEREGISTER(coro) \
VALGRIND_STACK_DEREGISTER((coro)->valgrindStackId)

#else
#define STACK_REGISTER(coro)
#define STACK_DEREGISTER(coro)
#endif

extern "C" {
    extern int lightweight_swapcontext(ucontext_t *oucp, const ucontext_t *ucp);
}

void *Coro_stack(Coro *self)
{
    return self->stack;
}

size_t Coro_stackSize(Coro *self)
{
    return self->stack_size - 16;
}

Coro *Coro_new(size_t stack_size)
{
    Coro *self = (Coro *)calloc(1, sizeof(Coro));
    self->stack_size = stack_size;

#ifdef USE_FIBERS
    self->fiber = NULL; //Allocate a stack here?
#else
    if (stack_size > 0) {
        self->stack = valloc(stack_size);
        mprotect(self->stack, getpagesize(), PROT_NONE);
    }
#endif
    STACK_REGISTER(self);
    return self;
}

void Coro_free(Coro *self)
{
#ifdef USE_FIBERS
    // If this coro has a fiber, delete it.
    // Don't delete the main fiber. We don't want to commit suicide.
    if (self->fiber && !self->isMain)
    {
        DeleteFiber(self->fiber);
    }
#else
    mprotect(self->stack, getpagesize(), PROT_READ|PROT_WRITE); //Undo protections changes
    free(self->stack);
    STACK_DEREGISTER(self);
#endif
    free(self);
}


void Coro_initializeMainCoro(Coro *self)
{
    self->isMain = 1;
#ifdef USE_FIBERS
    // We must convert the current thread into a fiber if it hasn't already been done.
    if ((LPVOID) 0x1e00 == GetCurrentFiber()) // value returned when not a fiber
    {
        // Make this thread a fiber and set its data field to the main coro's address
        ConvertThreadToFiber(self);
    }
    // Make the main coro represent the current fiber
    self->fiber = GetCurrentFiber();
#endif
}

void Coro_switchTo_(Coro *self, Coro *next)
{
#if defined(USE_FIBERS)
    SwitchToFiber(next->fiber);
#elif defined(USE_UCONTEXT)
    lightweight_swapcontext(&self->env, &next->env);
#endif
}


#if defined(USE_UCONTEXT)
void Coro_setup(Coro *self)
{
    ucontext_t *ucp = (ucontext_t *) &self->env;

    getcontext(ucp);

    ucp->uc_stack.ss_sp    = Coro_stack(self);
    ucp->uc_stack.ss_size  = Coro_stackSize(self);
#if !defined(__APPLE__)
    ucp->uc_stack.ss_flags = 0;
    ucp->uc_link = NULL;
#endif
    makecontext(ucp, &coro_t::run_coroutine, 0);
}

#elif defined(USE_FIBERS)

void Coro_setup(Coro *self)
{
    // If this coro was recycled and already has a fiber, delete it.
    // Don't delete the main fiber. We don't want to commit suicide.

    if (self->fiber && !self->isMain)
    {
        DeleteFiber(self->fiber);
    }

    self->fiber = CreateFiber(Coro_stackSize(self),
                              (LPFIBER_START_ROUTINE)&coro_t::run_coroutine);
    if (!self->fiber) {
        DWORD err = GetLastError();
        exit(err);
    }
}
#endif
