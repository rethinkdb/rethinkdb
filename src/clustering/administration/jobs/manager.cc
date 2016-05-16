// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/manager.hpp"

#include <functional>
#include <iterator>

#include "concurrency/watchable.hpp"
#include "pprint/js_pprint.hpp"
#include "rdb_protocol/context.hpp"
#include "rdb_protocol/query_cache.hpp"

const size_t jobs_manager_t::printed_query_columns = 89;

const uuid_u jobs_manager_t::base_sindex_id =
    str_to_uuid("74d855a5-0c40-4930-a451-d1ce508ef2d2");

const uuid_u jobs_manager_t::base_disk_compaction_id =
    str_to_uuid("b8766ece-d15c-4f96-bee5-c0edacf10c9c");

const uuid_u jobs_manager_t::base_backfill_id =
    str_to_uuid("a5e1b38d-c712-42d7-ab4c-f177a3fb0d20");

jobs_manager_t::jobs_manager_t(mailbox_manager_t *_mailbox_manager,
                               server_id_t const &_server_id,
                               rdb_context_t *_rdb_context,
                               real_table_persistence_interface_t
                                   *_table_persistence_interface,
                               multi_table_manager_t *_multi_table_manager) :
    mailbox_manager(_mailbox_manager),
    server_id(_server_id),
    rdb_context(_rdb_context),
    table_persistence_interface(_table_persistence_interface),
    multi_table_manager(_multi_table_manager),
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

void jobs_manager_t::on_get_job_reports(
        signal_t *interruptor,
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

    pmap(get_num_threads(), [&](int32_t threadnum) {
        // Here we need to store `query_job_report_t` locally to prevent multiple threads
        // from inserting into the outer `job_reports`.
        std::vector<query_job_report_t> query_job_reports_inner;
        {
            on_thread_t thread((threadnum_t(threadnum)));

            for (const auto &query_cache
                    : *rdb_context->get_query_caches_for_this_thread()) {
                for (const auto &pair : *query_cache) {
                    if (pair.second->persistent_interruptor.is_pulsed()) {
                        continue;
                    }

                    auto render = pprint::render_as_javascript(
                        pair.second->term_storage->root_term());

                    query_job_reports_inner.emplace_back(
                        pair.second->job_id,
                        time - std::min(pair.second->start_time, time),
                        server_id,
                        query_cache->get_client_addr_port(),
                        pretty_print(printed_query_columns, render),
                        query_cache->get_user_context());
                }
            }
        }
        query_job_reports.insert(
            query_job_reports.end(),
            std::make_move_iterator(query_job_reports_inner.begin()),
            std::make_move_iterator(query_job_reports_inner.end()));
    });

    if (table_persistence_interface != nullptr &&
            table_persistence_interface->is_gc_active()) {
        // Note that "disk_compaction" jobs do not have a duration.
        disk_compaction_job_reports.emplace_back(
            uuid_u::from_hash(base_disk_compaction_id, uuid_to_str(server_id.get_uuid())),
            -1,
            server_id);
    }

    try {
        multi_table_manager->visit_tables(interruptor, access_t::read,
        [&](const namespace_id_t &table_id,
                UNUSED multistore_ptr_t *multistore_ptr,
                table_manager_t *table_manager) {
            std::map<std::string, std::pair<sindex_config_t, sindex_status_t> > statuses =
                table_manager->get_sindex_manager().get_status(interruptor);
            for (const auto &status : statuses) {
                uuid_u id = uuid_u::from_hash(
                    base_sindex_id, uuid_to_str(table_id) + status.first);

                /* Note that we only calculate the duration if the index construction is
                   still in progress. */
                double duration = status.second.second.ready
                    ? 0.0
                    : time - std::min<double>(status.second.second.start_time, time);

                index_construction_job_reports.emplace_back(
                    id,
                    duration,
                    server_id,
                    table_id,
                    status.first,
                    status.second.second.ready,
                    status.second.second.progress_numerator,
                    status.second.second.progress_denominator);
            }

            std::map<region_t, backfill_progress_tracker_t::progress_tracker_t> backfills =
                table_manager->get_backfill_progress_tracker().get_progress_trackers();
            std::string base_str = uuid_to_str(server_id.get_uuid()) + uuid_to_str(table_id);
            for (const auto &backfill : backfills) {
                uuid_u id = uuid_u::from_hash(
                    base_backfill_id,
                    base_str + uuid_to_str(backfill.second.source_server_id.get_uuid()));

                /* As above we only calculate the duration if the backfill is still in
                   progress. */
                double duration = backfill.second.is_ready
                    ? 0.0
                    : time - std::min<double>(backfill.second.start_time, time);

                backfill_job_reports.emplace_back(
                    id,
                    duration,
                    server_id,
                    table_id,
                    backfill.second.is_ready,
                    backfill.second.progress,
                    backfill.second.source_server_id,
                    server_id);
            }
        });

        send(mailbox_manager,
             reply_address,
             query_job_reports,
             disk_compaction_job_reports,
             index_construction_job_reports,
             backfill_job_reports);
    } catch (const interrupted_exc_t &) {
        // Do nothing
    }
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
                        query_cache->terminate_internal(pair.second.get());
                        pair.second->interrupt_reason =
                            ql::query_cache_t::interrupt_reason_t::DELETE;
                        return;
                    }
                }
            }
        }
    });
}
