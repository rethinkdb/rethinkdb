// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/stat_manager.hpp"

#include <functional>

#include "concurrency/watchable.hpp"
#include "perfmon/archive.hpp"
#include "perfmon/collect.hpp"
#include "perfmon/filter.hpp"
#include "stl_utils.hpp"

stat_manager_t::stat_manager_t(mailbox_manager_t* mm) :
    mailbox_manager(mm),
    get_stats_mailbox(mailbox_manager,
                      std::bind(&stat_manager_t::on_stats_request,
                                this, ph::_1, ph::_2))
    { }

get_stats_mailbox_address_t stat_manager_t::get_address() {
    return get_stats_mailbox.get_address();
}

void stat_manager_t::on_stats_request(const return_address_t& reply_address, const std::set<stat_id_t>& requested_stats) {
    coro_t::spawn_sometime(std::bind(&stat_manager_t::perform_stats_request, this, reply_address, requested_stats, auto_drainer_t::lock_t(&drainer)));
}

void stat_manager_t::perform_stats_request(const return_address_t& reply_address, const std::set<std::string>& requested_stats, auto_drainer_t::lock_t) {
    perfmon_filter_t request(requested_stats);
    scoped_ptr_t<perfmon_result_t> perfmon_result(perfmon_get_stats());
    request.filter(&perfmon_result);
    guarantee(perfmon_result.has());
    send(mailbox_manager, reply_address, *perfmon_result);
}
