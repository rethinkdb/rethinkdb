#ifndef __CONCURRENCY_TASK_HPP__
#define __CONCURRENCY_TASK_HPP__

#include "arch/arch.hpp"
#include "concurrency/cond_var.hpp"

/* We need two versions of task_t, one of which is specialized for void */

template<typename var_t>
struct task_t {
public:   // But not really
    task_t() { }
    promise_t<var_t> cond;
    template<class callable_t>
    void run(callable_t *f) {
        cond.pulse((*f)());
        delete f;
    }
public:
    var_t join() {
        var_t value = cond.wait();
        delete this;
        return value;
    }
private:
    DISABLE_COPYING(task_t);
};

template<typename callable_t, typename var_t>
void run_task(task_t<var_t> *t, callable_t *f) {
    t->cond.pulse((*f)());
    delete f;
}

template<>
struct task_t<void> {
public:   // But not really
    task_t() { }
    cond_t cond;
    template<class callable_t>
    void run(callable_t *f) {
        (*f)();
        cond.pulse();
        delete f;
    }
public:
    void join() {
        cond.wait();
        delete this;
    }
private:
    DISABLE_COPYING(task_t);
};

template<typename callable_t>
void run_task(task_t<void> *t, callable_t *f) {
    (*f)();
    t->cond.pulse();
    delete f;
}

/* Many variations of the task() function for convenience */

template<typename return_t, typename callable_t>
task_t<return_t> *task(callable_t fun) {
    task_t<return_t> *t = new task_t<return_t>();
    void (task_t<return_t>::*f2)(callable_t*) = &task_t<return_t>::run;
    coro_t::spawn(boost::bind(f2, t, new callable_t(fun)));
    return t;
}

template<typename return_t, typename callable_t, typename arg_t>
task_t<return_t> *task(callable_t fun, arg_t arg) {
    return task<return_t>(boost::bind(fun, arg));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t>
task_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2) {
    return task<return_t>(boost::bind(fun, arg1, arg2));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t>
task_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3) {
    return task<return_t>(boost::bind(fun, arg1, arg2, arg3));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t>
task_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4) {
    return task<return_t>(boost::bind(fun, arg1, arg2, arg3, arg4));
}

template<typename return_t, typename callable_t, typename arg1_t, typename arg2_t, typename arg3_t, typename arg4_t, typename arg5_t>
task_t<return_t> *task(callable_t fun, arg1_t arg1, arg2_t arg2, arg3_t arg3, arg4_t arg4, arg5_t arg5) {
    return task<return_t>(boost::bind(fun, arg1, arg2, arg3, arg4, arg5));
}

#endif /* __CONCURRENCY_TASK_HPP__ */
