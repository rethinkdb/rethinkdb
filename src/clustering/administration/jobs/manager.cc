// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/manager.hpp"

#include <functional>
#include <iterator>

#include "concurrency/watchable.hpp"

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(jobs_manager_business_card_t,
                                    get_job_reports_mailbox_address);

jobs_manager_t::jobs_manager_t(mailbox_manager_t* _mailbox_manager) :
    mailbox_manager(_mailbox_manager),
    get_job_reports_mailbox(_mailbox_manager,
                            std::bind(&jobs_manager_t::on_get_job_reports,
                                      this, ph::_1, ph::_2))
    { }

jobs_manager_business_card_t jobs_manager_t::get_business_card() {
    business_card_t business_card;
    business_card.get_job_reports_mailbox_address =
        get_job_reports_mailbox.get_address();
    return business_card;
}

jobs_manager_t::queries_map_t * jobs_manager_t::get_queries_map() {
    return queries_map.get();
}

jobs_manager_t::sindexes_multimap_t * jobs_manager_t::get_sindexes_multimap() {
    return sindexes_multimap.get();
}

void jobs_manager_t::on_get_job_reports(
        UNUSED signal_t *interruptor,
        const business_card_t::return_mailbox_t::address_t& reply_address) {
    std::vector<job_report_t> job_reports;
    pmap(get_num_threads(), [&](int32_t threadnum) {
        microtime_t time = current_microtime();

        // Here we need to store `job_report_t` locally to prevent multiple threads from
        // inserting into the outer `job_reports`.
        std::vector<job_report_t> job_reports_inner;
        {
            on_thread_t thread((threadnum_t(threadnum)));

            for (auto const &query : *queries_map.get()) {
                job_reports_inner.push_back(job_report_t(
                    "query",
                    query.first,
                    (time - query.second.start_time) / 1000000.0));
            }

            for (auto const &sindex : *sindexes_multimap.get()) {
                job_reports_inner.push_back(job_report_t(
                    "sindex",
                    generate_uuid(),
                    (time - sindex.second.start_time) / 1000000.0));
            }
        }
        job_reports.insert(job_reports.end(),
                           std::make_move_iterator(job_reports_inner.begin()),
                           std::make_move_iterator(job_reports_inner.end()));
    });
    send(mailbox_manager, reply_address, job_reports);
}
