// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/manager.hpp"

#include <functional>
#include <iterator>

#include "clustering/administration/reactor_driver.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/context.hpp"

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(jobs_manager_business_card_t,
                                    get_job_reports_mailbox_address,
                                    job_interrupt_mailbox_address);

const uuid_u jobs_manager_t::base_sindex_id =
    str_to_uuid("74d855a5-0c40-4930-a451-d1ce508ef2d2");

const uuid_u jobs_manager_t::base_disk_compaction_id =
    str_to_uuid("b8766ece-d15c-4f96-bee5-c0edacf10c9c");

const uuid_u jobs_manager_t::base_backfill_id =
    str_to_uuid("a5e1b38d-c712-42d7-ab4c-f177a3fb0d20");

jobs_manager_t::jobs_manager_t(mailbox_manager_t *_mailbox_manager,
                               server_id_t const &_server_id) :
    mailbox_manager(_mailbox_manager),
    server_id(_server_id),
    rdb_context(nullptr),
    reactor_driver(nullptr),
    get_job_reports_mailbox(_mailbox_manager,
                            std::bind(&jobs_manager_t::on_get_job_reports,
                                      this, ph::_1, ph::_2)),
    job_interrupt_mailbox(_mailbox_manager,
                          std::bind(&jobs_manager_t::on_job_interrupt,
                                    this, ph::_1, ph::_2)) { }

jobs_manager_business_card_t jobs_manager_t::get_business_card() {
    business_card_t business_card;
    business_card.get_job_reports_mailbox_address =
        get_job_reports_mailbox.get_address();
    business_card.job_interrupt_mailbox_address = job_interrupt_mailbox.get_address();
    return business_card;
}

void jobs_manager_t::set_rdb_context(rdb_context_t *_rdb_context) {
    rdb_context = _rdb_context;
}

void jobs_manager_t::set_reactor_driver(reactor_driver_t *_reactor_driver) {
    reactor_driver = _reactor_driver;
}

void jobs_manager_t::unset_rdb_context_and_reactor_driver() {
    drainer.drain();

    rdb_context = nullptr;
    reactor_driver = nullptr;
}

void jobs_manager_t::on_get_job_reports(
        UNUSED signal_t *interruptor,
        const business_card_t::return_mailbox_t::address_t &reply_address) {
    std::vector<job_report_t> job_reports;

    if (drainer.is_draining()) {
        // We're shutting down, send an empty reponse since we can't acquire a `drainer`
        // lock any more. Note the `mailbox_manager` is destructed after we are, thus
        // the pointer remains valid.
        send(mailbox_manager, reply_address, job_reports);
        return;
    }

    auto lock = drainer.lock();

    // Note, as `time` is retrieved here a job may actually report to be started after
    // fetching the time, leading to a negative duration which we round to zero.
    microtime_t time = current_microtime();

    pmap(get_num_threads(), [&](int32_t threadnum) {
        // Here we need to store `job_report_t` locally to prevent multiple threads from
        // inserting into the outer `job_reports`.
        std::vector<job_report_t> job_reports_inner;
        {
            on_thread_t thread((threadnum_t(threadnum)));

            if (rdb_context != nullptr) {
                for (auto const &query
                        : *rdb_context->get_query_jobs_for_this_thread()) {
                    job_reports_inner.emplace_back(
                        query.first,
                        "query",
                        time - std::min(query.second.start_time, time),
                        query.second.client_addr_port);
                }
            }
        }
        job_reports.insert(job_reports.end(),
                           std::make_move_iterator(job_reports_inner.begin()),
                           std::make_move_iterator(job_reports_inner.end()));
    });

    if (reactor_driver != nullptr) {
        for (auto const &sindex_job : reactor_driver->get_sindex_jobs()) {
            uuid_u id = uuid_u::from_hash(
                base_sindex_id,
                uuid_to_str(sindex_job.first.first) + sindex_job.first.second);

            // Only report the duration of index construction still in progress.
            double duration = sindex_job.second.is_ready
                ? 0
                : time - std::min(sindex_job.second.start_time, time);

            job_reports.emplace_back(
                id,
                "index_construction",
                duration,
                sindex_job.first.first,
                sindex_job.first.second,
                sindex_job.second.is_ready,
                sindex_job.second.progress);
        }

        if (reactor_driver->is_gc_active()) {
            uuid_u id = uuid_u::from_hash(base_disk_compaction_id, uuid_to_str(server_id));

            // `disk_compaction` jobs do not have a duration, it's set to -1 to prevent
            // it being displayed later.
            job_reports.emplace_back(id, "disk_compaction", -1);
        }

        for (auto const &report : reactor_driver->get_backfill_progress()) {
            // Only report the duration of backfills still in progress.
            double duration = report.second.is_ready
                ? 0
                : time - std::min(report.second.start_time, time);

            std::string base_str =
                uuid_to_str(server_id) + uuid_to_str(report.first.first);
            for (auto const &backfill : report.second.backfills) {
                uuid_u id = uuid_u::from_hash(
                    base_backfill_id, base_str + uuid_to_str(backfill.first.get_uuid()));

                job_reports.emplace_back(
                    id,
                    "backfill",
                    duration,
                    report.first.first,
                    report.second.is_ready,
                    backfill.second,
                    backfill.first,
                    server_id);
            }
        }
    }

    send(mailbox_manager, reply_address, job_reports);
}

void jobs_manager_t::on_job_interrupt(
        UNUSED signal_t *interruptor, uuid_u const &id) {
    if (drainer.is_draining()) {
        // We're shutting down, the query job will be interrupted regardless.
        return;
    }

    auto lock = drainer.lock();

    pmap(get_num_threads(), [&](int32_t threadnum) {
        on_thread_t thread((threadnum_t(threadnum)));

        if (rdb_context != nullptr) {
            rdb_context_t::query_jobs_t *query_jobs =
                rdb_context->get_query_jobs_for_this_thread();
            rdb_context_t::query_jobs_t::const_iterator iterator = query_jobs->find(id);
            if (iterator != query_jobs->end()) {
                iterator->second.interruptor->pulse_if_not_already_pulsed();
            }
        }
    });
}
