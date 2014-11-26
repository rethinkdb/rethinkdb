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

class rdb_context_t;
class reactor_driver_t;

class jobs_manager_t {
public:
    explicit jobs_manager_t(mailbox_manager_t *mailbox_manager,
                            server_id_t const &server_id);

    typedef jobs_manager_business_card_t business_card_t;
    jobs_manager_business_card_t get_business_card();

    // The `jobs_manager_t` object is constructed before either the `rdb_context_t` or
    // `reactor_driver_t` is constructed in `main/serve.cc`, thus the pointers to the
    // respective objects are set once they are constructed.
    void set_rdb_context(rdb_context_t *);
    void set_reactor_driver(reactor_driver_t *);

private:
    static const uuid_u base_sindex_id;
    static const uuid_u base_disk_compaction_id;

    void on_get_job_reports(
        UNUSED signal_t *interruptor,
        const business_card_t::return_mailbox_t::address_t& reply_address);

    mailbox_manager_t *mailbox_manager;
    business_card_t::get_job_reports_mailbox_t get_job_reports_mailbox;

    server_id_t server_id;

    rdb_context_t *rdb_context;
    reactor_driver_t *reactor_driver;

    DISABLE_COPYING(jobs_manager_t);
};

#endif /* CLUSTERING_ADMINISTRATION_JOBS_MANAGER_HPP_ */
