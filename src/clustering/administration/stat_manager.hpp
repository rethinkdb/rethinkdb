#ifndef CLUSTERING_ADMINISTRATION_STAT_MANAGER_HPP_
#define CLUSTERING_ADMINISTRATION_STAT_MANAGER_HPP_

#include <string>
#include <map>
#include <set>
#include "rpc/mailbox/typed.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "perfmon/perfmon_types.hpp"

class stat_manager_t {
public:
    typedef std::string stat_id_t;
    typedef mailbox_addr_t<void(perfmon_result_t)> return_address_t;
    typedef mailbox_t<void(return_address_t, std::set<stat_id_t>)> get_stats_mailbox_t;
    typedef get_stats_mailbox_t::address_t get_stats_mailbox_address_t;

    explicit stat_manager_t(mailbox_manager_t* mailbox_manager);

    get_stats_mailbox_address_t get_address();

private:
    static void send_stats(mailbox_manager_t* mailbox_manager, return_address_t& reply_address, std::set<stat_id_t>& requested_stats);
    get_stats_mailbox_t get_stats_mailbox;

    DISABLE_COPYING(stat_manager_t);
};

typedef stat_manager_t::get_stats_mailbox_t::address_t get_stats_mailbox_address_t;

#endif /* CLUSTERING_ADMINISTRATION_STAT_MANAGER_HPP_ */

