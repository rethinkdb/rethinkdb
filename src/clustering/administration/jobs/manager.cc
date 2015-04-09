// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/manager.hpp"

#include <functional>
#include <iterator>

#include "concurrency/watchable.hpp"
#include "pprint/js_pprint.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/query_cache.hpp"

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(jobs_manager_business_card_t,
                                    get_job_reports_mailbox_address,
                                    job_interrupt_mailbox_address);

const size_t jobs_manager_t::printed_query_columns = 89;

const uuid_u jobs_manager_t::base_sindex_id =
    str_to_uuid("74d855a5-0c40-4930-a451-d1ce508ef2d2");

const uuid_u jobs_manager_t::base_disk_compaction_id =
    str_to_uuid("b8766ece-d15c-4f96-bee5-c0edacf10c9c");

const uuid_u jobs_manager_t::base_backfill_id =
    str_to_uuid("a5e1b38d-c712-42d7-ab4c-f177a3fb0d20");

jobs_manager_t::jobs_manager_t(mailbox_manager_t *_mailbox_manager,
                               server_id_t const &_server_id,
                               server_config_client_t *_server_config_client) :
    mailbox_manager(_mailbox_manager),
    server_id(_server_id),
    server_config_client(_server_config_client),
    rdb_context(nullptr),
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

// RSI(raft): Reimplement this once Raft supports table IO
#if 0
void jobs_manager_t::set_reactor_driver(reactor_driver_t *_reactor_driver) {
    reactor_driver = _reactor_driver;
}
#endif

void jobs_manager_t::unset_rdb_context_and_reactor_driver() {
    drainer.drain();

    rdb_context = nullptr;
// RSI(raft): Reimplement this once Raft supports table IO
#if 0
    reactor_driver = nullptr;
#endif
}

void jobs_manager_t::on_get_job_reports(
        UNUSED signal_t *interruptor,
        const business_card_t::return_mailbox_t::address_t &reply_address) {
    std::vector<query_job_report_t> query_job_reports;
    std::vector<disk_compaction_job_report_t> disk_compaction_job_reports;
    std::vector<index_construction_job_report_t> index_construction_job_reports;
    std::vector<backfill_job_report_t> backfill_job_reports;

    if (drainer.is_draining()) {
        // We're shutting down, send an empty reponse since we can't acquire a `drainer`
        // lock any more. Note the `mailbox_manager` is destructed after we are, thus
        // the pointer remains valid.
        send(mailbox_manager,
             reply_address,
             query_job_reports,
             disk_compaction_job_reports,
             index_construction_job_reports,
             backfill_job_reports);
        return;
    }

    auto lock = drainer.lock();

    // Note, as `time` is retrieved here a job may actually report to be started after
    // fetching the time, leading to a negative duration which we round to zero.
    microtime_t time = current_microtime();

    if (rdb_context != nullptr) {
        pmap(get_num_threads(), [&](int32_t threadnum) {
            // Here we need to store `query_job_report_t` locally to prevent multiple threads
            // from inserting into the outer `job_reports`.
            std::vector<query_job_report_t> query_job_reports_inner;
            {
                on_thread_t thread((threadnum_t(threadnum)));

                for (auto const &query_cache
                        : *rdb_context->get_query_caches_for_this_thread()) {
                    for (auto const &pair : *query_cache) {
                        if (pair.second->persistent_interruptor.is_pulsed()) {
                            continue;
                        }

                        auto render = pprint::render_as_javascript(
                            pair.second->original_query->query());

                        query_job_reports_inner.emplace_back(
                            pair.second->job_id,
                            time - std::min(pair.second->start_time, time),
                            server_id,
                            query_cache->get_client_addr_port(),
                            pretty_print(printed_query_columns, render));
                    }
                }
            }
            query_job_reports.insert(
                query_job_reports.end(),
                std::make_move_iterator(query_job_reports_inner.begin()),
                std::make_move_iterator(query_job_reports_inner.end()));
        });
    }

    // RSI(raft): Reimplement this once Raft supports table IO
    (void)server_config_client;
#if 0
    if (reactor_driver != nullptr) {
        for (auto const &job : reactor_driver->get_sindex_jobs()) {
            uuid_u id = uuid_u::from_hash(
                base_sindex_id,
                uuid_to_str(job.first.first) + job.first.second);

            // Only report the duration of index constructions still in progress.
            double duration = job.second.is_ready
                ? 0
                : time - std::min(job.second.start_time, time);

            index_construction_job_reports.emplace_back(
                id,
                duration,
                server_id,
                job.first.first,
                job.first.second,
                job.second.is_ready,
                job.second.progress);
        }

        if (reactor_driver->is_gc_active()) {
            disk_compaction_job_reports.emplace_back(
                uuid_u::from_hash(base_disk_compaction_id, uuid_to_str(server_id)),
                -1.0,           // Note that `"disk_compaction"` jobs do not have a duration.
                server_id);
        }

        for (auto const &backfills : reactor_driver->get_backfill_progress()) {
            std::string base_str =
                uuid_to_str(server_id) + uuid_to_str(backfills.first.first);

            // Only report the duration of backfills still in progress.
            double duration = backfills.second.is_ready
                ? 0
                : time - std::min(backfills.second.start_time, time);

            for (auto const &backfill : backfills.second.backfills) {
                uuid_u id = uuid_u::from_hash(
                    base_backfill_id, base_str + uuid_to_str(backfill.first.get_uuid()));

                boost::optional<server_id_t> source_server_id =
                    server_config_client->get_server_id_for_peer_id(backfill.first);
                if (static_cast<bool>(source_server_id) == false) {
                    continue;
                }

                backfill_job_reports.emplace_back(
                    id,
                    duration,
                    server_id,
                    backfills.first.first,
                    backfills.second.is_ready,
                    backfill.second,
                    source_server_id.get(),
                    server_id);
            }
        }
    }
#endif

    send(mailbox_manager,
         reply_address,
         query_job_reports,
         disk_compaction_job_reports,
         index_construction_job_reports,
         backfill_job_reports);
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
            for (auto &&query_cache : *rdb_context->get_query_caches_for_this_thread()) {
                for (auto &&pair : *query_cache) {
                    if (pair.second->job_id == id) {
                        pair.second->persistent_interruptor.pulse_if_not_already_pulsed();
                        return;
                    }
                }
            }
        }
    });
}
