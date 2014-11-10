// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_JOBS_MANAGER_HPP_
#define CLUSTERING_ADMINISTRATION_JOBS_MANAGER_HPP_

#include <string>
#include <map>
#include <set>

#include "clustering/administration/jobs/report.hpp"
#include "concurrency/one_per_thread.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/uuid.hpp"
#include "rpc/mailbox/typed.hpp"
#include "rpc/serialize_macros.hpp"

class jobs_manager_business_card_t {
public:
    typedef mailbox_t<void(std::vector<job_report_t>)> return_mailbox_t;
    typedef mailbox_t<void(return_mailbox_t::address_t)> get_job_reports_mailbox_t;

    get_job_reports_mailbox_t::address_t get_job_reports_mailbox_address;
};
RDB_DECLARE_SERIALIZABLE(jobs_manager_business_card_t);

class jobs_manager_t {
public:
    explicit jobs_manager_t(mailbox_manager_t* mailbox_manager);

    typedef jobs_manager_business_card_t business_card_t;
    jobs_manager_business_card_t get_business_card();

    typedef std::map<uuid_u, query_job_t> queries_map_t;
    queries_map_t * get_queries_map();

    typedef std::multimap<std::pair<uuid_u, uuid_u>, sindex_job_t> sindexes_multimap_t;
    sindexes_multimap_t * get_sindexes_multimap();

private:
    void on_get_job_reports(
        UNUSED signal_t *interruptor,
        const business_card_t::return_mailbox_t::address_t& reply_address);

    mailbox_manager_t *mailbox_manager;
    business_card_t::get_job_reports_mailbox_t get_job_reports_mailbox;

    one_per_thread_t<queries_map_t> queries_map;
    one_per_thread_t<sindexes_multimap_t> sindexes_multimap;

    DISABLE_COPYING(jobs_manager_t);
};

#endif /* CLUSTERING_ADMINISTRATION_JOBS_MANAGER_HPP_ */
