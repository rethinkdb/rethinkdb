#include "errors.hpp"
#include <boost/bind.hpp>

#include "clustering/administration/stat_manager.hpp"
#include "concurrency/watchable.hpp"
#include "perfmon/collect.hpp"
#include "perfmon/archive.hpp"
#include "stl_utils.hpp"

stat_manager_t::stat_manager_t(mailbox_manager_t* mm) :
    mailbox_manager(mm),
    get_stats_mailbox(mailbox_manager, boost::bind(&stat_manager_t::on_stats_request, this, _1, _2), mailbox_callback_mode_inline)
    { }

get_stats_mailbox_address_t stat_manager_t::get_address() {
    return get_stats_mailbox.get_address();
}

void stat_manager_t::on_stats_request(const return_address_t& reply_address, const std::set<stat_id_t>& requested_stats) {
    coro_t::spawn_sometime(boost::bind(&stat_manager_t::perform_stats_request, this, reply_address, requested_stats, auto_drainer_t::lock_t(&drainer)));
}

void stat_manager_t::perform_stats_request(const return_address_t& reply_address, const std::set<stat_id_t>& requested_stats, auto_drainer_t::lock_t) {
    perfmon_filter_t request(requested_stats);
    scoped_ptr_t<perfmon_result_t> perfmon_result(request.filter(perfmon_get_stats()));
    //scoped_ptr_t<perfmon_result_t> perfmon_result(perfmon_get_stats());
    send(mailbox_manager, reply_address, *perfmon_result.get());
}
