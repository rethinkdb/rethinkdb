// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/stats/stat_manager.hpp"

#include <functional>

#include "concurrency/watchable.hpp"
#include "perfmon/collect.hpp"
#include "perfmon/filter.hpp"
#include "stl_utils.hpp"

stat_manager_t::stat_manager_t(mailbox_manager_t* mm) :
    mailbox_manager(mm),
    get_stats_mailbox(mailbox_manager,
                      std::bind(&stat_manager_t::on_stats_request,
                                this, ph::_1, ph::_2, ph::_3))
    { }

get_stats_mailbox_address_t stat_manager_t::get_address() {
    return get_stats_mailbox.get_address();
}

void stat_manager_t::on_stats_request(
        UNUSED signal_t *interruptor,
        const return_address_t& reply_address,
        const std::set<std::vector<stat_id_t> >& requested_stats) {
    perfmon_filter_t request(requested_stats);
    ql::datum_t perfmon_result(perfmon_get_stats());
    perfmon_result = request.filter(perfmon_result);
    send(mailbox_manager, reply_address, perfmon_result);
}

bool fetch_stats_from_server(
        mailbox_manager_t *mailbox_manager,
        const get_stats_mailbox_address_t &request_addr,
        const std::set<std::vector<stat_manager_t::stat_id_t> > &filter,
        signal_t *interruptor,
        ql::datum_t *stats_out,
        std::string *error_out) {
    cond_t done;
    mailbox_t<void(ql::datum_t)> return_mailbox(mailbox_manager,
        [&](signal_t *, ql::datum_t s) {
            *stats_out = s;
            done.pulse();
        });

    disconnect_watcher_t disconnect_watcher(mailbox_manager, request_addr.get_peer());

    send(mailbox_manager, request_addr, return_mailbox.get_address(), filter);

    signal_timer_t timeout;
    timeout.start(5000);

    wait_any_t waiter(&done, &disconnect_watcher, &timeout);
    wait_interruptible(&waiter, interruptor);

    if (disconnect_watcher.is_pulsed()) {
        *error_out = "Server disconnected.";
        return false;
    }

    if (timeout.is_pulsed()) {
        *error_out = "Stats request timed out.";
    }

    guarantee(done.is_pulsed());
    return true;
}

