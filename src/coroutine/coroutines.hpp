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
    coro_t(void (*fn)(void *arg), void *arg); //Creates and notifies a coroutine
    explicit coro_t(Coro *underlying) : underlying(underlying), dead(false)
#ifndef NDEBUG
    , notified(false)
#endif
    { }
    void notify(); //Wakes up the coroutine, allowing the scheduler to trigger it to continue
    static void move_to_cpu(int cpu); //Wait and notify self on the CPU (avoiding race conditions)
    ~coro_t();
    virtual void on_cpu_switch();

public:
    void *initialize(void (*fn)(void *), void *arg);

private:
    void switch_to(coro_t *next);

    Coro *underlying;
    bool dead;
    int home_cpu; //not a home_cpu_mixin_t because this is set by initialize

public:
    static void wait(); //Pauses the current coroutine until it's notified
    static coro_t *self(); //Returns the current coroutine
    static void run();
    static void destroy();
    static void yield();

private:
    static void suicide();
    static void run_coroutine(void *);

    static __thread coro_t *current_coro;
    static __thread coro_t *scheduler; //Main execution of program

    DISABLE_COPYING(coro_t);

#ifndef NDEBUG
    bool notified;
#endif
//Unary typed coroutines
    template<typename T>
    coro_t(void (*fn)(T), T arg) {
        initialize((void (*)(void *))fn, (void *)arg);
    }

//Binary typed coroutines
protected:
    template<typename A, typename B>
    struct binary {
        void (*fn)(A, B);
        A first;
        B second;
        binary(void (*fn)(A, B), A first, B second)
            : fn(fn), first(first), second(second) { }
    };

    template<typename A, typename B>
    static void start_binary(void *arg) {
        binary<A, B> argument = *(binary<A, B> *)arg;
        delete (binary<A, B> *)arg;
        (*argument.fn)(argument.first, argument.second);
    }

public:
    template<typename A, typename B>
    coro_t(void (*fn)(A, B), A first, B second) {
        initialize(start_binary<A, B>, (void *)new binary<A, B>(fn, first, second));
    }

//Ternary typed coroutines
protected:
    template<typename A, typename B, typename C>
    struct ternary {
        void (*fn)(A, B, C);
        A first;
        B second;
        C third;
        ternary(void (*fn)(A, B, C), A first, B second, C third)
            : fn(fn), first(first), second(second), third(third) { }
    };

    template<typename A, typename B, typename C>
    static void start_ternary(void *arg) {
        ternary<A, B, C> argument = *(ternary<A, B, C> *)arg;
        delete (ternary<A, B, C> *)arg;
        (*argument.fn)(argument.first, argument.second, argument.third);
    }

public:
    template<typename A, typename B, typename C>
    coro_t(void (*fn)(A, B, C), A first, B second, C third) {
        initialize(start_ternary<A, B, C>, (void *)new ternary<A, B, C>(fn, first, second, third));
    }

//Quaternary typed coroutines
protected:
    template<typename A, typename B, typename C, typename D>
    struct quaternary {
        void (*fn)(A, B, C, D);
        A first;
        B second;
        C third;
        D fourth;
        quaternary(void (*fn)(A, B, C, D), A first, B second, C third, D fourth)
            : fn(fn), first(first), second(second), third(third), fourth(fourth) { }
    };

    template<typename A, typename B, typename C, typename D>
    static void start_quaternary(void *arg) {
        quaternary<A, B, C, D> argument = *(quaternary<A, B, C, D> *)arg;
        delete (quaternary<A, B, C, D> *)arg;
        (*argument.fn)(argument.first, argument.second, argument.third, argument.fourth);
    }

public:
    template<typename A, typename B, typename C, typename D>
    coro_t(void (*fn)(A, B, C, D), A first, B second, C third, D fourth) {
        initialize(start_quaternary<A, B, C, D>, (void *)new quaternary<A, B, C, D>(fn, first, second, third, fourth));
    }

//Fiveary (?) typed coroutines
protected:
    template<typename A, typename B, typename C, typename D, typename E>
    struct fiveary {
        void (*fn)(A, B, C, D, E);
        A first;
        B second;
        C third;
        D fourth;
        E fifth;
        fiveary(void (*fn)(A, B, C, D, E), A first, B second, C third, D fourth, E fifth)
            : fn(fn), first(first), second(second), third(third), fourth(fourth), fifth(fifth) { }
    };

    template<typename A, typename B, typename C, typename D, typename E>
    static void start_fiveary(void *arg) {
        fiveary<A, B, C, D, E> argument = *(fiveary<A, B, C, D, E> *)arg;
        delete (fiveary<A, B, C, D, E> *)arg;
        (*argument.fn)(argument.first, argument.second, argument.third, argument.fourth, argument.fifth);
    }

public:
    template<typename A, typename B, typename C, typename D, typename E>
    coro_t(void (*fn)(A, B, C, D, E), A first, B second, C third, D fourth, E fifth) {
        initialize(start_fiveary<A, B, C, D, E>, (void *)new fiveary<A, B, C, D, E>(fn, first, second, third, fourth, fifth));
    }

protected:
    coro_t() { }
};

struct task_callback_t {
    virtual void on_task_return(void *value) = 0;
};

/* A task represents an action with a return value that can be blocked on */
struct task_t {
    task_t(void *(*fn)(void *), void *arg); //Creates and notifies a task to be joined
    void *join(); //Blocks the current coroutine until the task finishes, returning the result
    //Join should only be called once, or you can add a completion callback:
    void callback(task_callback_t *cb);
    static void run_task(void *(*)(void *), void *, task_t *);
    void notify();

private:
    coro_t *coroutine;
    bool done;
    void *result;
    std::vector<coro_t*> waiters; //There should always be 0 or 1 waiters

    static void run_callback(task_callback_t *cb, task_t *task);

    DISABLE_COPYING(task_t);
};

//TODO: Convenient constructors for task_t, similar to coro_t
//I'll write this when I have a place to use it; otherwise it'd be annoying to test

//maybe make more things like these, maybe using boost::bind

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


#endif // __COROUTINES_HPP__
