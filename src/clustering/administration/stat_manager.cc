#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>

#include "clustering/administration/stat_manager.hpp"
#include "concurrency/watchable.hpp"
#include "perfmon/perfmon.hpp"
#include "perfmon/perfmon_archive.hpp"
#include "stl_utils.hpp"

stat_manager_t::stat_manager_t(mailbox_manager_t* mailbox_manager) : get_stats_mailbox(mailbox_manager, boost::bind(&stat_manager_t::send_stats, mailbox_manager, _1, _2)) { }

void stat_manager_t::send_stats(mailbox_manager_t* mailbox_manager, return_address_t& reply_address, std::set<stat_id_t>&) {
    perfmon_result_t perfmon_result = perfmon_result_t::make_map();
    perfmon_get_stats(&perfmon_result);
    send(mailbox_manager, reply_address, perfmon_result);
}

get_stats_mailbox_address_t stat_manager_t::get_address() {
    return get_stats_mailbox.get_address();
}

