#include "coroutine/coroutines.hpp"
#include "arch/arch.hpp"
#include <stdio.h>

perfmon_counter_t pm_active_coroutines("active_coroutines");
perfmon_duration_sampler_t pm_move_thread("move_thread", secs_to_ticks(1));
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
    if (continue_on_thread(home_thread, this)) {
        call_later_on_this_thread(this);
    }
}

void coro_t::move_to_thread(int thread) {
    block_pm_duration timer(&pm_move_thread);
    self()->home_thread = thread;
    self()->notify();
    wait();
}

void coro_t::on_thread_switch() {
#ifndef NDEBUG
    assert(notified);
    notified = false;
#endif
    assert(scheduler == current_coro);
    scheduler->switch_to(this);
    assert(scheduler == current_coro);
    if (dead) delete this;
}

void coro_t::suicide() {
    self()->dead = true;
    wait();
}

struct coro_initialization_data {
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
    information.coroutine->action();
    coro_t::suicide();
}

void coro_t::start() {
    pm_active_coroutines++;
    underlying = Coro_new();
    coro_initialization_data data;
    coro_t *previous_coro = data.parent = current_coro;
    data.coroutine = current_coro = this;
    Coro_startCoro_(previous_coro->underlying, underlying, &data, run_coroutine);
}

coro_t::~coro_t() {
    pm_active_coroutines--;
    Coro_free(underlying);
}

void coro_t::run() {
    Coro *mainCoro = Coro_new();
    Coro_initializeMainCoro(mainCoro);
    scheduler = current_coro = new coro_t(mainCoro);
}

void coro_t::destroy() {
    assert(scheduler == current_coro);
    delete scheduler;
    scheduler = current_coro = NULL;
}

