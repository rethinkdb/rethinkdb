#include "arch/io/timer_provider.hpp"  // For RDB_USE_TIMER_ITIMER_PROVIDER

#if RDB_USE_TIMER_ITIMER_PROVIDER


#include <sys/time.h>
#include <string.h>

// TODO(OSX) We never free this list (just its elements).  Make it part of the thread pool.
__thread intrusive_list_t<timer_itimer_provider_t> *itimer_callback_list = 0;

void add_to_itimer_callback_list(timer_itimer_provider_t *provider) {
    if (!itimer_callback_list) {
        itimer_callback_list = new intrusive_list_t<timer_itimer_provider_t>;
    }
    itimer_callback_list->push_back(provider);
}

void remove_from_itimer_callback_list(timer_itimer_provider_t *provider) {
    guarantee(itimer_callback_list);
    itimer_callback_list->remove(provider);
}

void timer_itimer_forward_alrm() {
    if (itimer_callback_list) {
        timer_itimer_provider_t *next = NULL;
        timer_itimer_provider_t *curr = itimer_callback_list->head();
        while (curr != NULL) {
            next = itimer_callback_list->next(curr);
            curr->on_alrm();
            curr = next;
        }
    }
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
