// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/job_manager.hpp"

#include <functional>

#include "concurrency/watchable.hpp"
#include "perfmon/archive.hpp"
#include "perfmon/collect.hpp"
#include "perfmon/filter.hpp"
#include "stl_utils.hpp"

job_manager_t::job_manager_t(mailbox_manager_t* mm) :
    mailbox_manager(mm),
    get_job_descs_mailbox(mailbox_manager,
                          std::bind(&job_manager_t::on_get_job_descs,
                                    this, ph::_1, drainer.lock()))
    { }

job_manager_business_card_t job_manager_t::get_business_card() {
    b_card_t b_card;
    b_card.get_job_descs_mailbox_address = get_job_descs_mailbox.get_address();
    return b_card;
}

void job_manager_t::on_get_job_descs(const b_card_t::return_mailbox_t::address_t& reply_address,
                                     auto_drainer_t::lock_t) {
    std::vector<job_desc_t> job_descs = get_job_descriptions();
    send(mailbox_manager, reply_address, job_descs);
}

/* void job_manager_t::perform_stats_request(const return_address_t& reply_address, const std::set<std::string>& requested_stats, auto_drainer_t::lock_t) {
    perfmon_filter_t request(requested_stats);
    scoped_ptr_t<perfmon_result_t> perfmon_result(perfmon_get_stats());
    request.filter(&perfmon_result);
    guarantee(perfmon_result.has());
    send(mailbox_manager, reply_address, *perfmon_result);
} */
