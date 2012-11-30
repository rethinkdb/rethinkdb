#include "arch/io/timer_provider.hpp"  // For RDB_USE_TIMER_ITIMER_PROVIDER

#if RDB_USE_TIMER_ITIMER_PROVIDER


#include <sys/time.h>
#include <string.h>

struct timer_itimer_data_t {
    // A list of providers whose on_alrm functions we should call.
    intrusive_list_t<timer_itimer_provider_t> callbacks;

    // Used to prevent force access to timer_itimer mechanisms to happen outside.
    bool in_alrm_handler;

    timer_itimer_data_t() : in_alrm_handler(false) { }
};

__thread timer_itimer_data_t *itimer_data = 0;

void add_to_itimer_callback_list(timer_itimer_provider_t *provider) {
    if (!itimer_data) {
        itimer_data = itimer_data = new timer_itimer_data_t;
    }

    guarantee(!itimer_data->in_alrm_handler);
    itimer_data->callbacks.push_back(provider);
}

void remove_from_itimer_callback_list(timer_itimer_provider_t *provider) {
    guarantee(itimer_data != NULL);
    guarantee(!itimer_data->in_alrm_handler);

    itimer_data->callbacks.remove(provider);
    if (itimer_data->callbacks.empty()) {
        delete itimer_data;
        itimer_data = NULL;
    }
}

void timer_itimer_forward_alrm() {
    guarantee(!itimer_data->in_alrm_handler);
    itimer_data->in_alrm_handler = true;
    if (itimer_data) {
        timer_itimer_provider_t *next = NULL;
        timer_itimer_provider_t *curr = itimer_data->callbacks.head();
        while (curr != NULL) {
            next = itimer_data->callbacks.next(curr);
            curr->on_alrm();
            curr = next;
        }
    }
    itimer_data->in_alrm_handler = false;
}

timer_itimer_provider_t::timer_itimer_provider_t(linux_event_queue_t *queue,
                                                 timer_provider_callback_t *callback,
                                                 UNUSED time_t secs, UNUSED int32_t nsecs)
    : queue_(queue), callback_(callback) {
    add_to_itimer_callback_list(this);
}

timer_itimer_provider_t::~timer_itimer_provider_t() {
    remove_from_itimer_callback_list(this);
}

void timer_itimer_provider_t::on_alrm() {
    callback_->on_timer(1);
}


#endif  // RDB_USE_TIMER_ITIMER_PROVIDER
