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
    : private thread_message_t
{
public:
    static void wait(); //Pauses the current coroutine until it's notified
    static coro_t *self(); //Returns the current coroutine
    static void run();
    static void destroy();
    void notify(); //Wakes up the coroutine, allowing the scheduler to trigger it to continue
    static void move_to_thread(int thread); //Wait and notify self on the CPU (avoiding race conditions)

private:
    ~coro_t();
    virtual void on_thread_switch();
    void start();
    
    virtual void action() { }

    coro_t() : underlying(NULL), dead(false), home_thread(get_thread_id())
#ifndef NDEBUG
    , notified(false)
#endif
    { }

    explicit coro_t(Coro *underlying) : underlying(underlying), dead(false), home_thread(get_thread_id())
#ifndef NDEBUG
    , notified(false)
#endif
    { }

    void switch_to(coro_t *next);

    Coro *underlying;
    bool dead;
    int home_thread; //not a home_thread_mixin_t because this is set by initialize

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

    struct on_thread_t {
        int home_thread;
        on_thread_t(int thread) {
            home_thread = get_thread_id();
            coro_t::move_to_thread(thread);
        }
        ~on_thread_t() {
            coro_t::move_to_thread(home_thread);
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
            var_t value = var;
            delete this; //would we ever have a condition variable that multiple things can wait on?
                         //if so, we shouldn't delete this; the caller should delete it
                         //if not, we shouldn't bother using a vector for waiters
            return value;
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

//TODO: Why am I using malloc, memcpy and free here? I can't get stuff to work! This is disgusting.
private:
    template<typename return_t, typename callable_t>
    static void run_task(callable_t *f, cond_var_t<return_t> *cond_var) {
        return_t value = (*f)();
        free(f);
        cond_var->fill(value);
    }

public:
    template<typename return_t, typename callable_t>
    static cond_var_t<return_t> *task(callable_t fun) {
        cond_var_t<return_t> *cond_var = new cond_var_t<return_t>();
        callable_t *f = (callable_t *)malloc(sizeof(fun));
        memcpy(f, &fun, sizeof(fun));
        spawn(&run_task<return_t, callable_t>, f, cond_var);
        return cond_var;
    }

    template<typename return_t, typename callable_t, typename arg_t>
    static cond_var_t<return_t> *task(callable_t fun, arg_t arg) {
        return task<return_t>(boost::bind(fun, arg));
    }

    template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t>
    static cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2) {
        return task<return_t>(boost::bind(fun, arg1, arg2));
    }

    template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t>
    static cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3) {
        return task<return_t>(boost::bind(fun, arg1, arg2, arg3));
    }

    template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t>
    static cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4) {
        return task<return_t>(boost::bind(fun, arg1, arg2, arg3, arg4));
    }

    template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t, typename arg5_t>
    static cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4, arg5_t arg5) {
        return task<return_t>(boost::bind(fun, arg1, arg2, arg3, arg4, arg5));
    }

};

#endif // __COROUTINES_HPP__
