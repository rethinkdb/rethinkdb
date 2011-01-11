#include "coroutine/coroutines.hpp"
#include "config/args.hpp"
#include "arch/arch.hpp"
#include <stdio.h>
#include <vector>

perfmon_counter_t pm_active_coroutines("active_coroutines"),
                  pm_allocated_coroutines("allocated_coroutines");

std::vector<coro_t*> coro_t::all_coros[MAX_THREADS];

__thread coro_t *coro_t::current_coro = NULL;
__thread coro_t *coro_t::scheduler = NULL;
__thread std::vector<coro_t*> *coro_t::free_coros = NULL;

size_t coro_t::stack_size = COROUTINE_STACK_SIZE; //Default, setable by command-line parameter

coro_t *coro_t::self() {
    return current_coro;
}

void coro_t::wait() {
    assert(current_coro != scheduler);
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
}

void coro_t::run_coroutine() {
    coro_t *self = current_coro;
    while (true) {
        pm_active_coroutines++;
#ifndef NDEBUG
        self->active = true;
#endif
        (*self->deed)();
        pm_active_coroutines--;

        free_coros->push_back(self);
#ifndef NDEBUG
        self->active = false;
#endif
        wait();
    }
}

coro_t *coro_t::get_free_coro() {
    if (free_coros->size() == 0) {
        coro_t *coro = new coro_t(Coro_new(stack_size));
        pm_allocated_coroutines++;
        all_coros[get_thread_id()].push_back(coro);
        Coro_setup(coro->underlying); //calls run_coroutine
        return coro;
    } else {
        assert(free_coros->size() > 0);
        coro_t *coro = free_coros->back();
        free_coros->pop_back();
        return coro;
    }
}

int coro_t::in_coro_from_cpu(void *addr) {
    size_t base = (size_t)addr - ((size_t)addr % getpagesize());
    for (int i = 0; i < MAX_THREADS; i++) {
        for (std::vector<coro_t*>::iterator p = all_coros[i].begin(); p != all_coros[i].end(); p++) {
            if (base == (size_t)((*p)->underlying->stack)) {
                return i;
            }
        }
    }
    return -1;
}

void coro_t::do_deed(deed_t *deed) {
    assert(!active);
    this->deed = deed;
    home_thread = get_thread_id();
    notify();
}

coro_t::~coro_t() {
    Coro_free(underlying);
}

void coro_t::run() {
    Coro *mainCoro = Coro_new(0);
    Coro_initializeMainCoro(mainCoro);
    scheduler = current_coro = new coro_t(mainCoro);
    free_coros = new std::vector<coro_t*>();
}

void coro_t::destroy() {
    assert(scheduler == current_coro);
    delete scheduler;
    for (std::vector<coro_t*>::iterator it = free_coros->begin(); it != free_coros->end(); it++) {
        delete *it;
    }
    delete free_coros;
    scheduler = current_coro = NULL;
    free_coros = NULL;
}

