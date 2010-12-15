#ifndef __COROUTINES_HPP__
#define __COROUTINES_HPP__
#include "coroutine/Coro.hpp"
#include "arch/arch.hpp"
#include "utils.hpp"
#include <list>
#include <vector>
#include <boost/bind.hpp>

extern perfmon_counter_t pm_active_coroutines;

/* A coroutine represents an action with no return value */
struct coro_t
    : private cpu_message_t
{
public:
    static void wait(); //Pauses the current coroutine until it's notified
    static coro_t *self(); //Returns the current coroutine
    static void run();
    static void destroy();
    void notify(); //Wakes up the coroutine, allowing the scheduler to trigger it to continue
    static void move_to_cpu(int cpu); //Wait and notify self on the CPU (avoiding race conditions)

private:
    ~coro_t();
    virtual void on_cpu_switch();
    void start();
    
    virtual void action() { }

    coro_t() : underlying(NULL), dead(false), home_cpu(get_cpu_id())
#ifndef NDEBUG
    , notified(false)
#endif
    { }

    explicit coro_t(Coro *underlying) : underlying(underlying), dead(false), home_cpu(get_cpu_id())
#ifndef NDEBUG
    , notified(false)
#endif
    { }

    void switch_to(coro_t *next);

    Coro *underlying;
    bool dead;
    int home_cpu; //not a home_cpu_mixin_t because this is set by initialize

#ifndef NDEBUG
    bool notified;
#endif

    static void suicide();
    static void run_coroutine(void *);

    static __thread coro_t *current_coro;
    static __thread coro_t *scheduler; //Epoll loop--main execution of program

    DISABLE_COPYING(coro_t);

public:
    template<typename callable_t>
    static void spawn(callable_t fun) {
        struct fun_runner_t : public coro_t {
            callable_t fun;
            fun_runner_t(callable_t fun) : fun(fun) { }
            virtual void action() {
                fun();
            }
        } *coroutine = new fun_runner_t(fun);
        coroutine->start();
    }

    template<typename callable_t, typename arg_t>
    static void spawn(callable_t fun, arg_t arg) {
        spawn(boost::bind(fun, arg));
    }

    template<typename callable_t, typename arg1_t, typename arg2_t>
    static void spawn(callable_t fun, arg1_t arg1, arg2_t arg2) {
        spawn(boost::bind(fun, arg1, arg2));
    }

    template<typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t>
    static void spawn(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3) {
        spawn(boost::bind(fun, arg1, arg2, arg3));
    }

    template<typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t>
    static void spawn(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4) {
        spawn(boost::bind(fun, arg1, arg2, arg3, arg4));
    }

    template<typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t, typename arg5_t>
    static void spawn(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4, arg5_t arg5) {
        spawn(boost::bind(fun, arg1, arg2, arg3, arg4, arg5));
    }

    struct on_cpu_t {
        int home_cpu;
        on_cpu_t(int cpu) {
            home_cpu = get_cpu_id();
            coro_t::move_to_cpu(cpu);
        }
        ~on_cpu_t() {
            coro_t::move_to_cpu(home_cpu);
        }
    };
};

/*
//Are tasks actually useful at all?
struct task_callback_t {
    virtual void on_task_return(void *value) = 0;
};

// A task represents an action with a return value that can be blocked on
struct task_t {
    task_t(void *(*fn)(void *), void *arg); //Creates and notifies a task to be joined
    void *join(); //Blocks the current coroutine until the task finishes, returning the result
    //Join should only be called once, or you can add a completion callback:
    void callback(task_callback_t *cb);
    void run_task(void *(*)(void *), void *);

private:
    bool done;
    void *result;
    std::vector<coro_t*> waiters; //There should always be 0 or 1 waiters

    void run_callback(task_callback_t *cb);

    DISABLE_COPYING(task_t);
};
*/


#endif // __COROUTINES_HPP__
