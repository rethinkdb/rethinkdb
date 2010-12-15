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

    template<typename var_t>
    struct cond_var_t {
    private:
        bool filled;
        union {
            var_t var;
            void *nothing;
        };
        std::vector<coro_t*> waiters;
        DISABLE_COPYING(cond_var_t);
    public:
        cond_var_t() : filled(false), nothing(NULL), waiters() {}
        var_t join() {
            if (!filled) {
                waiters.push_back(self());
                wait();
            }
            assert(filled);
            return var;
        }
        void fill(var_t value) {
            assert(!filled);
            var = value;
            filled = true;
            for (std::vector<coro_t*>::iterator it = waiters.begin(); it != waiters.end(); ++it) {
                (*it)->notify();
            }
        }
    };

    struct multi_wait_t {
    private:
        coro_t *waiter;
        unsigned int notifications_left;
        DISABLE_COPYING(multi_wait_t);
    public:
        multi_wait_t(unsigned int notifications_left)
            : waiter(self()), notifications_left(notifications_left) {
            assert(notifications_left > 0);
        }
        void notify() {
            notifications_left--;
            if (notifications_left == 0) {
                waiter->notify();
                delete this;
            }
        }
    };

};

#endif // __COROUTINES_HPP__
