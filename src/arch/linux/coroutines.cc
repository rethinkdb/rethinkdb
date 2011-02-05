#include "arch/linux/coroutines.hpp"
#include "arch/linux/thread_pool.hpp"
#include "config/args.hpp"
#include <stdio.h>
#include <sys/mman.h>
#include <ucontext.h>

#ifdef VALGRIND
#include <valgrind/valgrind.h>
#endif

#ifndef NDEBUG
#include <cxxabi.h>   // For __cxa_current_exception_type (see below)
#endif

perfmon_counter_t pm_active_coroutines("active_coroutines"),
                  pm_allocated_coroutines("allocated_coroutines");

size_t coro_stack_size = COROUTINE_STACK_SIZE; //Default, setable by command-line parameter

/* We have a custom implementation of swapcontext() that doesn't swap the floating-point
registers, the SSE registers, or the signal mask. This is for performance reasons. */

extern "C" {
    extern int lightweight_swapcontext(ucontext_t *oucp, const ucontext_t *ucp);
}

/* The register definitions and the definition of lightweight_swapcontext itself are
derived from GLibC, which is covered under the LGPL. */

/* These are the byte offsets of various fields in the GLibC ucontext_t type. To be
more portable, maybe we should define our own makecontext()/getcontext()/setcontext()
as well; right now we're tied tightly to GLibC. */
#define oRBP "120"
#define oRSP "160"
#define oRBX "128"
#define oR12 "72"
#define oR13 "80"
#define oR14 "88"
#define oR15 "96"
#define oRIP "168"

asm(
    ".section .text\n"
    ".globl lightweight_swapcontext\n"
    "lightweight_swapcontext:\n"

    /* Save the preserved registers and the return address */
    "\tmovq\t%rbx, "oRBX"(%rdi)\n"
    "\tmovq\t%rbp, "oRBP"(%rdi)\n"
    "\tmovq\t%r12, "oR12"(%rdi)\n"
    "\tmovq\t%r13, "oR13"(%rdi)\n"
    "\tmovq\t%r14, "oR14"(%rdi)\n"
    "\tmovq\t%r15, "oR15"(%rdi)\n"
    "\tmovq\t(%rsp), %rcx\n"
    "\tmovq\t%rcx, "oRIP"(%rdi)\n"
    "\tleaq\t8(%rsp), %rcx\n"        /* Exclude the return address.  */
    "\tmovq\t%rcx, "oRSP"(%rdi)\n"

    /* Load the new stack pointer and the preserved registers.  */
    "\tmovq\t"oRSP"(%rsi), %rsp\n"
    "\tmovq\t"oRBX"(%rsi), %rbx\n"
    "\tmovq\t"oRBP"(%rsi), %rbp\n"
    "\tmovq\t"oR12"(%rsi), %r12\n"
    "\tmovq\t"oR13"(%rsi), %r13\n"
    "\tmovq\t"oR14"(%rsi), %r14\n"
    "\tmovq\t"oR15"(%rsi), %r15\n"

    /* The following ret should return to the address set with
    getcontext.  Therefore push the address on the stack.  */
    "\tmovq\t"oRIP"(%rsi), %rcx\n"
    "\tpushq\t%rcx\n"
    "\txorl\t%eax, %eax\n"
    "\tret\n"
);

/* coro_context_t is only used internally within the coroutine logic. For performance reasons,
we recycle stacks and ucontexts; the coro_context_t represents a stack and a ucontext. */

struct coro_context_t :
    public intrusive_list_node_t<coro_context_t>
{
    coro_context_t();

    /* The run() function is at the bottom of every coro_context_t's call stack. It repeatedly
    waits for a coroutine to run and then calls that coroutine's run() method. */
    static void run();

    ~coro_context_t();

    /* The guts of the coro_context_t */
    void *stack;
    ucontext_t env;
#ifdef VALGRIND
    unsigned int valgrind_stack_id;
#endif
};

/* The coroutine we're currently in, if any. NULL if we are in the main context. */
static __thread coro_t *current_coro = NULL;

/* The main context. */
static __thread ucontext_t scheduler;

/* A list of coro_context_t objects that are not in use. */
static __thread intrusive_list_t<coro_context_t> *free_contexts = NULL;

/* coro_globals_t */

coro_globals_t::coro_globals_t()
{
    rassert(!current_coro);

    rassert(free_contexts == NULL);
    free_contexts = new intrusive_list_t<coro_context_t>;
}

coro_globals_t::~coro_globals_t()
{
    rassert(!current_coro);

    /* Destroy remaining coroutines */
    while (coro_context_t *s = free_contexts->head()) {
        free_contexts->remove(s);
        delete s;
    }

    delete free_contexts;
    free_contexts = NULL;
}

/* coro_context_t */

coro_context_t::coro_context_t()
{
    pm_allocated_coroutines++;

    stack = malloc_aligned(coro_stack_size, getpagesize());

    /* Protect the end of the stack so that we crash when we get a stack overflow instead of
    corrupting memory. */
    mprotect(stack, getpagesize(), PROT_NONE);

    /* Register our stack with Valgrind so that it understands what's going on and doesn't
    create spurious errors */
#ifdef VALGRIND
    valgrind_stack_id = VALGRIND_STACK_REGISTER(stack, (intptr_t)stack + coro_stack_size);
#endif

    getcontext(&env);   /* Why do we call getcontext() here? */

    env.uc_stack.ss_sp = stack;
    env.uc_stack.ss_size = coro_stack_size;
    env.uc_stack.ss_flags = 0;
    env.uc_link = NULL;

    /* run() is the main worker loop for a coroutine. */
    makecontext(&env, &coro_context_t::run, 0);
}

void coro_context_t::run() {

    coro_context_t *self = current_coro->context;

    /* Make sure we're on the right stack. */
#ifndef NDEBUG
    char dummy;
    rassert(&dummy >= (char*)self->stack);
    rassert(&dummy < (char*)self->stack + coro_stack_size);
#endif

    while (true) {

        current_coro->run();

        current_coro = NULL;
        lightweight_swapcontext(&self->env, &scheduler);
    }
}

coro_context_t::~coro_context_t() {

#ifdef VALGRIND
    VALGRIND_STACK_DEREGISTER(valgrind_stack_id);
#endif

    mprotect(stack, getpagesize(), PROT_READ|PROT_WRITE); //Undo protections changes

    free(stack);

    pm_allocated_coroutines--;
}

/* coro_t */


coro_t::coro_t(const boost::function<void()>& deed, int thread) :
    deed_(deed),
    current_thread_(thread),
    notified_(false)
{

    pm_active_coroutines++;

    /* Find us a stack */
    if (free_contexts->size() == 0) {
        context = new coro_context_t();
    } else {
        context = free_contexts->tail();
        free_contexts->remove(context);
    }

    /* Eventually our run() method will be called within the coroutine. */
    notify();
}

coro_t::~coro_t() {

    /* Return the context to the free-contexts list */
    free_contexts->push_back(context);

    pm_active_coroutines--;
}

void coro_t::run() {

    rassert(current_coro == this);

    deed_();

    delete this;
}

coro_t *coro_t::self() {   /* class method */
    return current_coro;
}

void coro_t::wait() {   /* class method */

#ifndef NDEBUG
    /* It's not safe to wait() in a catch clause of an exception handler. We use the non-standard
    GCC-only interface "cxxabi.h" to figure out if we're in the catch clause of an exception
    handler. In C++0x we will be able to use std::current_exception() instead. */
    rassert(!abi::__cxa_current_exception_type());
#endif

    rassert(current_coro != NULL);
    coro_context_t *context = current_coro->context;
    current_coro = NULL;
    lightweight_swapcontext(&context->env, &scheduler);
    rassert(current_coro != NULL);
}

void coro_t::notify() {

    rassert(!notified_);
    notified_ = true;

    /* notify() doesn't switch to the coroutine immediately; instead, it just pushes
    the coroutine onto the event queue. */

    /* current_thread is the thread that the coroutine lives on, which may or may not be the
    same as get_thread_id(). */
    linux_thread_pool_t::thread->message_hub.store_message(current_thread_, this);
}

void coro_t::move_to_thread(int thread) {   /* class method */
    if (thread == self()->current_thread_) {
        // If we're trying to switch to the thread we're currently on, do nothing.
        return;
    }
    self()->current_thread_ = thread;
    self()->notify();
    wait();
}

void coro_t::on_thread_switch() {

    rassert(notified_);
    notified_ = false;

    rassert(current_coro == NULL);
    current_coro = this;
    lightweight_swapcontext(&scheduler, &context->env);
    rassert(current_coro == NULL);
}

void coro_t::set_coroutine_stack_size(size_t size) {
    coro_stack_size = size;
}

/* Called by SIGSEGV handler to identify segfaults that come from overflowing a coroutine's
stack. Could also in theory be used by a function to check if it's about to overflow
the stack. */

bool is_coroutine_stack_overflow(void *addr) {
    void *base = (void *)floor_aligned((intptr_t)addr, getpagesize());
    return current_coro && current_coro->context->stack == base;
}

void coro_t::spawn(const boost::function<void()>& deed) {
    spawn_on_thread(linux_thread_pool_t::thread_id, deed);
}
