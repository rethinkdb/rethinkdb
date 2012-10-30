// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_STAT_MANAGER_HPP_
#define CLUSTERING_ADMINISTRATION_STAT_MANAGER_HPP_

#include <string>
#include <map>
#include <set>

#include "perfmon/types.hpp"
#include "rpc/mailbox/typed.hpp"

class stat_manager_t {
public:
    typedef std::string stat_id_t;
    typedef mailbox_addr_t<void(perfmon_result_t)> return_address_t;
    typedef mailbox_t<void(return_address_t, std::set<stat_id_t>)> get_stats_mailbox_t;
    typedef get_stats_mailbox_t::address_t get_stats_mailbox_address_t;

    explicit stat_manager_t(mailbox_manager_t* mailbox_manager);

    get_stats_mailbox_address_t get_address();

private:
    void on_stats_request(const return_address_t& reply_address, const std::set<stat_id_t>& requested_stats);
    void perform_stats_request(const return_address_t& reply_address, const std::set<stat_id_t>& requested_stats, auto_drainer_t::lock_t);

    mailbox_manager_t *mailbox_manager;
    get_stats_mailbox_t get_stats_mailbox;

    auto_drainer_t drainer;

    DISABLE_COPYING(stat_manager_t);
};

typedef stat_manager_t::get_stats_mailbox_t::address_t get_stats_mailbox_address_t;

#endif /* CLUSTERING_ADMINISTRATION_STAT_MANAGER_HPP_ */

