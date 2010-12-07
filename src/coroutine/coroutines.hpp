#ifndef __COROUTINES_HPP__
#define __COROUTINES_HPP__
#include "coroutine/Coro.hpp"
#include "arch/arch.hpp"
#include <list>
#include <vector>

/* A coroutine represents an action with no return value */
struct coro_t : public cpu_message_t {
    coro_t(void (*fn)(void *arg), void *arg); //Creates and notifies a coroutine
    explicit coro_t(Coro *underlying) : underlying(underlying), dead(false)
#ifndef NDEBUG
    , notified(false)
#endif
    { }
    void notify(); //Wakes up the coroutine, allowing the scheduler to trigger it to continue
    ~coro_t();
    virtual void on_cpu_switch();

private:
    void switch_to(coro_t *next);

    Coro *underlying;
    bool dead;

public:
    static void wait(); //Pauses the current coroutine until it's notified
    static coro_t *self(); //Returns the current coroutine
    static void run();

private:
    static void suicide();
    static void run_coroutine(void *);

    static __thread coro_t *current_coro;
    static __thread coro_t *scheduler; //Main execution of program

    DISABLE_COPYING(coro_t);

#ifndef NDEBUG
    bool notified;
#endif
};

struct task_callback_t {
    virtual void on_task_return(void *value) = 0;
};

/* A task represents an action with a return value that can be blocked on */
struct task_t {
    task_t(void *(*)(void *), void *arg); //Creates and notifies a task to be joined
    void *join(); //Blocks the current coroutine until the task finishes, returning the result
    //Join should only be called once, or you can add a completion callback:
    void callback(task_callback_t *cb);
    static void run_task(void*);
    void notify();

private:
    coro_t *coroutine;
    bool done;
    void *result;
    std::vector<coro_t*> waiters; //There should always be 0 or 1 watier

    static void run_callback(void *);

    DISABLE_COPYING(task_t);
};

#endif // __COROUTINES_HPP__


/* Add error checking to make sure nothing is notified or joined more than once? */

