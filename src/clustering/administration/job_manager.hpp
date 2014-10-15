// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_JOB_MANAGER_HPP_
#define CLUSTERING_ADMINISTRATION_JOB_MANAGER_HPP_

#include <string>
#include <map>
#include <set>

#include "job_control.hpp"
#include "rpc/mailbox/typed.hpp"

class job_manager_business_card_t {
public:
    typedef mailbox_t<void(std::vector<job_desc_t>)> return_mailbox_t;
    typedef mailbox_t<void(return_mailbox_t::address_t)> get_job_descs_mailbox_t;

    get_job_descs_mailbox_t::address_t get_job_descs_mailbox_address;
};

class job_manager_t {
public:
    typedef job_manager_business_card_t b_card_t;

    explicit job_manager_t(mailbox_manager_t* mailbox_manager);

    job_manager_business_card_t get_business_card();

private:
    void on_get_job_descs(const b_card_t::return_mailbox_t::address_t& reply_address,
                          auto_drainer_t::lock_t);

    mailbox_manager_t *mailbox_manager;
    auto_drainer_t drainer;

    b_card_t::get_job_descs_mailbox_t get_job_descs_mailbox;

    DISABLE_COPYING(job_manager_t);
};

#endif /* CLUSTERING_ADMINISTRATION_JOB_MANAGER_HPP_ */

