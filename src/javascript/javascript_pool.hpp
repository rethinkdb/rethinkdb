#ifndef __JAVASCRIPT_JAVASCRIPT_POOL_HPP__
#define __JAVASCRIPT_JAVASCRIPT_POOL_HPP__

#include <vector>
#include "arch/runtime/event_queue.hpp"
#include "arch/io/blocker_pool.hpp"
#include "arch/runtime/thread_pool.hpp"
#include "concurrency/cond_var.hpp"

#include <JavaScriptCore/JavaScript.h>

namespace JS {

/* Read this before doing anything important with JavaScriptCore embedding: */

/* This file exists partly for legitimate, reasonable reasons (running
 * potentially-long-running JavaScript functions using the OS's preëmptive
 * threads so that they don't block our coöperative coroutines), and partly to
 * account for the behavior of JavaScriptCore, the interpreter we're currently
 * using.
 *
 * The main issue is that JavaScriptCore, a C library, will garbage-collect on
 * machine stacks -- i.e., when you call a JavaScript function, it will
 * occasionally stop, scan your thread's stack[1], find unreachable objects,
 * and so on. This is so that you don't have to free objects that you allocated
 * by yourself -- which makes sense, because you *are* writing C code, after
 * all, and you wouldn't want to have to do all that. It's very convenient and
 * time-saving.
 *
 * But wait, there's more! JavaScriptCore supports threading, right? Which
 * means that you must be able to access objects from any thread, and not have
 * the garbage collector dispose of them for you. Therefore, any time you touch
 * any JSC object that belongs to a ContextGroup from a thread, you get added
 * to that ContextGroup's list of machine threads. When it GCs, it'll send you
 * a SIGUSR2 -- you're expected to suspend -- and then it'll scan *your* stack
 * and your registers and so on.
 *
 * This means that there are virtually no JSC API functions that are safe to
 * call from our main thread pool -- not even seemingly-innocuous ones like
 * JSValueIsNull(). So just about any JSC API call must be called from the
 * ContextGroup's associated blocker thread[2]. The current API asserts that,
 * but is not completely safe, so be careful.
 *
 *
 * More on ContextGroups: JavaScriptCore has a concept of "context groups"
 * (internally called "global data"), in which JavaScript evaluation contexts
 * may be created. To quote the API documentation:
 *
 *   Contexts in the same group may share and exchange JavaScript objects.
 *   Sharing and/or exchanging JavaScript objects between contexts in different
 *   groups will produce undefined behavior. When objects from the same context
 *   group are used in multiple threads, explicit synchronization is required.
 *
 * At the moment, we have one blocker thread associated with each context
 * group.
 *
 *
 * [1] Naturally, it isn't aware of our coroutines' heap-allocated stacks, so
 * it immediately considers any operation to be out of stack bounds. However,
 * simply modifying JSC to be aware of custom stack bounds isn't sufficient;
 * see <https://bugs.webkit.org/show_bug.cgi?id=65399>.
 *
 * [2] As far as I can tell, the only exceptions are calls that don't have a
 * context group associated with them, like JSString*(); note that these also
 * typically have manual release functions like JSStringRelease().
 */


/* A ctx_group_t is a JavaScriptCore ContextGroupRef along with a blocker
 * thread (implemented as a one-thread blocker_pool_t). */

struct ctx_group_t {
    explicit ctx_group_t(linux_event_queue_t *queue);
    ~ctx_group_t();

    JSGlobalContextRef create_JSGlobalContext();
    void               release_JSGlobalContext(JSGlobalContextRef jsc_context_ref);

    inline void incr_ctxcount() { ctxcount++; }
    inline void decr_ctxcount() { ctxcount--; }

    bool am_on_blocker_thread();

    // Run a blocking operation in the blocker thread.
    template <typename res_t>
    res_t run_blocking(boost::function<res_t()> f);

private:
    blocker_pool_t blocker;
    int ctxcount;
    JSContextGroupRef jsc_context_group_ref;

private:
    DISABLE_COPYING(ctx_group_t);
};


/* A javascript_pool_t is a pool of threads that can run JavaScript jobs. The
 * constructor starts n threads and initializes context groups on them; to get
 * an actual context, pass the js_pool a JS::ctx_t's constructor. */

struct javascript_pool_t {
    javascript_pool_t(int nthreads, linux_event_queue_t *queue);
    ~javascript_pool_t();

    /* Get one of the context groups in the pool. All operations on this
     * context group must be executed in its associated blocker thread. */
    ctx_group_t *get_a_ctx_group();

private:
    std::vector<ctx_group_t *> ctx_groups;
    int current_ctx_group_index;

private:
    DISABLE_COPYING(javascript_pool_t);
};


/* Template implementations. */

template <typename res_t>
res_t ctx_group_t::run_blocking(boost::function<res_t()> f) {
    rassert(!am_on_blocker_thread());
    rassert(coro_t::self());

    struct action_t : public cond_t, public blocker_pool_t::job_t {
        explicit action_t(boost::function<res_t()> f) : f_(f) {}

        res_t res; // This is typically a (typedefed) pointer, e.g. JSValueRef.
        boost::function<res_t()> f_;

        void run() { res = f_(); }
        void done() { pulse(); }
    };

    action_t a(f);
    blocker.do_job(&a);
    a.wait();
    return a.res;
}

template <>
inline void ctx_group_t::run_blocking<void>(boost::function<void()> f) {
    rassert(!am_on_blocker_thread());
    rassert(coro_t::self());

    struct action_t : public cond_t, public blocker_pool_t::job_t {
        explicit action_t(boost::function<void()> f) : f_(f) {}

        boost::function<void()> f_;

        void run() { f_(); }
        void done() { pulse(); }
    };

    action_t a(f);
    blocker.do_job(&a);
    a.wait();
}

} // namespace JS

#endif /* __JAVASCRIPT_JAVASCRIPT_POOL_HPP__ */
