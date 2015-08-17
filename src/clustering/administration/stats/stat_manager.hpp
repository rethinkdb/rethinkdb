// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_STATS_STAT_MANAGER_HPP_
#define CLUSTERING_ADMINISTRATION_STATS_STAT_MANAGER_HPP_

#include <map>
#include <set>
#include <string>

#include "perfmon/types.hpp"
#include "rpc/mailbox/typed.hpp"

struct admin_err_t;

class stat_manager_t {
public:
    typedef std::string stat_id_t;
    typedef mailbox_addr_t<void(ql::datum_t)> return_address_t;
    typedef mailbox_t<void(return_address_t, std::set<std::vector<stat_id_t> >)> get_stats_mailbox_t;
    typedef get_stats_mailbox_t::address_t get_stats_mailbox_address_t;

    explicit stat_manager_t(mailbox_manager_t* mailbox_manager,
                            server_id_t _own_server_id);

    get_stats_mailbox_address_t get_address();

private:
    void on_stats_request(
        signal_t *interruptor,
        const return_address_t& reply_address,
        const std::set<std::vector<stat_id_t> >& requested_stats);

    server_id_t own_server_id;
    mailbox_manager_t *mailbox_manager;
    get_stats_mailbox_t get_stats_mailbox;

    DISABLE_COPYING(stat_manager_t);
};

typedef stat_manager_t::get_stats_mailbox_t::address_t get_stats_mailbox_address_t;

bool fetch_stats_from_server(
        mailbox_manager_t *mailbox_manager,
        const get_stats_mailbox_address_t &request_addr,
        const std::set<std::vector<stat_manager_t::stat_id_t> > &filter,
        signal_t *interruptor,
        ql::datum_t *stats_out,
        admin_err_t *error_out);

#endif /* CLUSTERING_ADMINISTRATION_STATS_STAT_MANAGER_HPP_ */

