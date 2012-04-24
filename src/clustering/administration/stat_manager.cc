#include "errors.hpp"
#include <boost/function.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/map.hpp>

#include "clustering/administration/stat_manager.hpp"
#include "concurrency/watchable.hpp"
#include "perfmon.hpp"
#include "stl_utils.hpp"

stat_manager_t::stat_manager_t(mailbox_manager_t* mailbox_manager) : get_stats_mailbox(mailbox_manager, boost::bind(&stat_manager_t::send_stats, mailbox_manager, _1, _2)) { }

void stat_manager_t::send_stats(mailbox_manager_t* mailbox_manager, return_address_t& reply_address, std::set<stat_id_t>& requested_stats) {
    stats_t stats;

    perfmon_stats_t perfmon_stats;
    perfmon_get_stats(&perfmon_stats, true);

    for (perfmon_stats_t::iterator it = perfmon_stats.begin(); it != perfmon_stats.end(); ++it) {
        if (requested_stats.empty() || std_contains(requested_stats, it->first)) {
            stats[it->first] = it->second;
        }
    }
    send(mailbox_manager, reply_address, stats);
}

get_stats_mailbox_address_t stat_manager_t::get_address() {
    return get_stats_mailbox.get_address();
}

