#include "failover.hpp"
#include "logger.hpp"


void Failover_t::add_callback(failover_callback_t *cb) {
    callbacks.push_back(cb);
}

void Failover_t::on_failure() {
    for (intrusive_list_t<failover_callback_t>::iterator it = callbacks.begin(); it != callbacks.end(); it++)
        (*it).on_failure();
}
