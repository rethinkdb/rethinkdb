#include "coroutine/coroutines.hpp"
#include <stdio.h>

__thread coro_t *coro_t::current_coro = NULL;
__thread coro_t *coro_t::scheduler = NULL;

coro_t *coro_t::self() {
    return current_coro;
}

void coro_t::wait() {
    current_coro->switch_to(scheduler);
}

void coro_t::switch_to(coro_t *next) {
    current_coro = next;
    Coro_switchTo_(this->underlying, next->underlying);
}

void coro_t::notify() {
#ifndef NDEBUG
    assert(!notified);
    notified = true;
#endif
    call_later_on_this_cpu(this);
}

void coro_t::on_cpu_switch() {
#ifndef NDEBUG
    assert(notified);
    notified = false;
#endif
    scheduler->switch_to(this);
    if (dead) delete this;
}

void coro_t::suicide() {
    self()->dead = true;
    coro_t::wait();
}

struct coro_initialization_data {
    void *arg;
    void (*fn)(void *);
    coro_t *coroutine;
    coro_t *parent;
};

void coro_t::run_coroutine(void *data) {
    coro_initialization_data information = *(coro_initialization_data *)data;
    information.coroutine->notify();
    information.coroutine->switch_to(information.parent);
#ifndef NDEBUG
    information.coroutine->notified = false;
#endif
    (information.fn)(information.arg);
    coro_t::suicide();
}

coro_t::coro_t(void (*fn)(void *arg), void *arg) {
    initialize(fn, arg);
}

void coro_t::initialize(void (*fn)(void *arg), void *arg) {
    underlying = Coro_new();
    dead = false;
#ifndef NDEBUG
    notified = false;
#endif
    coro_initialization_data data;
    data.fn = fn;
    data.arg = arg;
    coro_t *previous_coro = data.parent = current_coro;
    data.coroutine = current_coro = this;
    Coro_startCoro_(previous_coro->underlying, underlying, &data, run_coroutine);
}

coro_t::~coro_t() {
    Coro_free(underlying);
}

void execute(void (*arg)()) {
    arg();
}

void coro_t::run() {
    Coro *mainCoro = Coro_new();
    Coro_initializeMainCoro(mainCoro);
    scheduler = current_coro = new coro_t(mainCoro);
}

void coro_t::destroy() {
    delete scheduler;
    scheduler = current_coro = NULL;
}

void *task_t::join() {
    assert(waiters.size() == 0);
    if (!done) {
        waiters.push_back(coro_t::self());
        coro_t::wait();
        assert(done);
    }
    void *value = result;
    delete this;
    return value;
}

void task_t::run_callback(task_callback_t *cb, task_t *task) {
    cb->on_task_return(task->join());
}

void task_t::callback(task_callback_t *cb) {
    new coro_t(run_callback, cb, this);
}

void task_t::run_task(void *(*fn)(void *), void *arg, task_t *task) {
    task->result = fn(arg);
    task->done = 1;
    for (std::vector<coro_t*>::iterator iter = task->waiters.begin();
         iter != task->waiters.end();
         iter++) {
        (*iter)->notify();
    }
}

task_t::task_t(void *(*fn)(void *), void *arg)
  : coroutine(NULL), done(0), result(NULL), waiters() {
    coroutine = new coro_t(run_task, fn, arg, this);
}

void task_t::notify() {
    coroutine->notify();
}
