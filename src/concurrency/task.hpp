#ifndef __CONCURRENCY_TASK_HPP__
#define __CONCURRENCY_TASK_HPP__

#include "arch/arch.hpp"

template<typename var_t>
struct cond_var_t {

private:
    bool filled;
    union {   // What is this union for?
        var_t var;
        void *nothing;
    };
    std::vector<coro_t*> waiters;
    DISABLE_COPYING(cond_var_t);

public:
    cond_var_t() : filled(false), nothing(NULL), waiters() {}
    var_t join() {
        if (!filled) {
            waiters.push_back(coro_t::self());
            coro_t::wait();
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

template<typename return_t, typename callable_t>
void run_task(callable_t *f, cond_var_t<return_t> *cond_var) {
    return_t value = (*f)();
    free(f);
    cond_var->fill(value);
}

template<typename return_t, typename callable_t>
cond_var_t<return_t> *task(callable_t fun) {
    cond_var_t<return_t> *cond_var = new cond_var_t<return_t>();
    //TODO: Why am I using malloc, memcpy and free here? I can't get stuff to work! This is disgusting.
    callable_t *f = (callable_t *)malloc(sizeof(fun));
    memcpy(f, &fun, sizeof(fun));
    coro_t::spawn(&run_task<return_t, callable_t>, f, cond_var);
    return cond_var;
}

template<typename return_t, typename callable_t, typename arg_t>
cond_var_t<return_t> *task(callable_t fun, arg_t arg) {
    return task<return_t>(boost::bind(fun, arg));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t>
cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2) {
    return task<return_t>(boost::bind(fun, arg1, arg2));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t>
cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3) {
    return task<return_t>(boost::bind(fun, arg1, arg2, arg3));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t>
cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4) {
    return task<return_t>(boost::bind(fun, arg1, arg2, arg3, arg4));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t, typename arg5_t>
cond_var_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4, arg5_t arg5) {
    return task<return_t>(boost::bind(fun, arg1, arg2, arg3, arg4, arg5));
}

#endif /* __CONCURRENCY_TASK_HPP__ */
