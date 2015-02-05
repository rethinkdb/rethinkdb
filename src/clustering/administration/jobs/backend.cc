// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/backend.hpp"

#include <set>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/jobs/manager.hpp"
#include "clustering/administration/jobs/report.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "concurrency/cross_thread_signal.hpp"

jobs_artificial_table_backend_t::jobs_artificial_table_backend_t(
        mailbox_manager_t *_mailbox_manager,
        boost::shared_ptr< semilattice_readwrite_view_t<
            cluster_semilattice_metadata_t> > _semilattice_view,
        const clone_ptr_t<watchable_t<change_tracking_map_t<
            peer_id_t, cluster_directory_metadata_t> > > &_directory_view,
        server_config_client_t *_server_config_client,
        admin_identifier_format_t _identifier_format)
    : mailbox_manager(_mailbox_manager),
      semilattice_view(_semilattice_view),
      directory_view(_directory_view),
      server_config_client(_server_config_client),
      identifier_format(_identifier_format) {
}

jobs_artificial_table_backend_t::~jobs_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

std::string jobs_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

void jobs_artificial_table_backend_t::get_all_job_reports(
        signal_t *interruptor,
        std::map<uuid_u, query_job_report_t> *query_jobs_out,
        std::map<uuid_u, disk_compaction_job_report_t> *disk_compaction_jobs_out,
        std::map<uuid_u, index_construction_job_report_t> *index_construction_jobs_out,
        std::map<uuid_u, backfill_job_report_t> *backfill_jobs_out) {
    assert_thread();  // Accessing `directory_view`

    typedef std::map<peer_id_t, cluster_directory_metadata_t> peers_t;
    peers_t peers = directory_view->get().get_inner();
    pmap(peers.begin(), peers.end(), [&](peers_t::value_type const &peer) {
        cond_t returned_job_reports;
        disconnect_watcher_t disconnect_watcher(mailbox_manager, peer.first);

        jobs_manager_business_card_t::return_mailbox_t return_mailbox(
            mailbox_manager,
            [&](UNUSED signal_t *,
                std::vector<query_job_report_t> const & query_jobs,
                std::vector<disk_compaction_job_report_t> const &disk_compaction_jobs,
                std::vector<index_construction_job_report_t> const &index_construction_jobs,
                std::vector<backfill_job_report_t> const &backfill_jobs) {
                for (auto const &query_job : query_jobs) {
                    auto result = query_jobs_out->insert(
                        std::make_pair(query_job.id, query_job));
                    if (result.second == false) {
                        result.first->second.merge(query_job);
                    }
                }
                for (auto const &disk_compaction_job : disk_compaction_jobs) {
                    auto result = disk_compaction_jobs_out->insert(
                        std::make_pair(disk_compaction_job.id, disk_compaction_job));
                    if (result.second == false) {
                        result.first->second.merge(disk_compaction_job);
                    }
                }
                for (auto const &index_construction_job : index_construction_jobs) {
                    auto result = index_construction_jobs_out->insert(
                        std::make_pair(index_construction_job.id,
                                       index_construction_job));
                    if (result.second == false) {
                        result.first->second.merge(index_construction_job);
                    }
                }
                for (auto const &backfill_job : backfill_jobs) {
                    auto result = backfill_jobs_out->insert(
                        std::make_pair(backfill_job.id, backfill_job));
                    if (result.second == false) {
                        result.first->second.merge(backfill_job);
                    }
                }

                returned_job_reports.pulse();
            });
        send(mailbox_manager,
             peer.second.jobs_mailbox.get_job_reports_mailbox_address,
             return_mailbox.get_address());

        wait_any_t waiter(&returned_job_reports, &disconnect_watcher, interruptor);
        waiter.wait();
    });

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

bool jobs_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        UNUSED std::string *error_out) {
    rows_out->clear();

    cross_thread_signal_t ct_interruptor(interruptor, home_thread());
    on_thread_t rethreader(home_thread());

    std::map<uuid_u, query_job_report_t> query_jobs;
    std::map<uuid_u, disk_compaction_job_report_t> disk_compaction_jobs;
    std::map<uuid_u, index_construction_job_report_t> index_construction_jobs;
    std::map<uuid_u, backfill_job_report_t> backfill_jobs;

    get_all_job_reports(
        &ct_interruptor,
        &query_jobs,
        &disk_compaction_jobs,
        &index_construction_jobs,
        &backfill_jobs);

    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    ql::datum_t row;
    for (auto const &query_job : query_jobs) {
        if (query_job.second.to_datum(
                identifier_format, server_config_client, metadata, &row)) {
            rows_out->push_back(row);
        }
    }
    for (auto const &disk_compaction_job : disk_compaction_jobs) {
        if (disk_compaction_job.second.to_datum(
                identifier_format, server_config_client, metadata, &row)) {
            rows_out->push_back(row);
        }
    }
    for (auto const &index_construction_job : index_construction_jobs) {
        if (index_construction_job.second.to_datum(
                identifier_format, server_config_client, metadata, &row)) {
            rows_out->push_back(row);
        }
    }
    for (auto const &backfill_job : backfill_jobs) {
        if (backfill_job.second.to_datum(
                identifier_format, server_config_client, metadata, &row)) {
            rows_out->push_back(row);
        }
    }

    return true;
}

bool jobs_artificial_table_backend_t::read_row(ql::datum_t primary_key,
                                               signal_t *interruptor,
                                               ql::datum_t *row_out,
                                               UNUSED std::string *error_out) {
    *row_out = ql::datum_t();

    cross_thread_signal_t ct_interruptor(interruptor, home_thread());
    on_thread_t rethreader(home_thread());

    std::string job_type;
    uuid_u job_id;
    if (convert_job_type_and_id_from_datum(primary_key, &job_type, &job_id)) {
        std::map<uuid_u, query_job_report_t> query_jobs;
        std::map<uuid_u, disk_compaction_job_report_t> disk_compaction_jobs;
        std::map<uuid_u, index_construction_job_report_t> index_construction_jobs;
        std::map<uuid_u, backfill_job_report_t> backfill_jobs;

        get_all_job_reports(
            &ct_interruptor,
            &query_jobs,
            &disk_compaction_jobs,
            &index_construction_jobs,
            &backfill_jobs);

        cluster_semilattice_metadata_t metadata = semilattice_view->get();
        ql::datum_t row;
        if (job_type == "query") {
            auto const iterator = query_jobs.find(job_id);
            if (iterator != query_jobs.end() && iterator->second.to_datum(
                   identifier_format, server_config_client, metadata, &row)) {
                *row_out = std::move(row);
            }
        } else if (job_type == "disk_compaction") {
            auto const iterator = disk_compaction_jobs.find(job_id);
            if (iterator != disk_compaction_jobs.end() && iterator->second.to_datum(
                   identifier_format, server_config_client, metadata, &row)) {
                *row_out = std::move(row);
            }
        } else if (job_type == "index_construction") {
            auto const iterator = index_construction_jobs.find(job_id);
            if (iterator != index_construction_jobs.end() && iterator->second.to_datum(
                   identifier_format, server_config_client, metadata, &row)) {
                *row_out = std::move(row);
            }
        } else if (job_type == "backfill") {
            auto const iterator = backfill_jobs.find(job_id);
            if (iterator != backfill_jobs.end() && iterator->second.to_datum(
                   identifier_format, server_config_client, metadata, &row)) {
                *row_out = std::move(row);
            }
        }
    }

    return true;
}

bool jobs_artificial_table_backend_t::write_row(ql::datum_t primary_key,
                                                bool pkey_was_autogenerated,
                                                ql::datum_t *new_value_inout,
                                                UNUSED signal_t *interruptor,
                                                std::string *error_out) {
    on_thread_t rethreader(home_thread());

    if (new_value_inout->has()) {
        error_out->assign("The `rethinkdb.jobs` system table only allows deletions, "
                          "not inserts or updates.");
        return false;
    }
    guarantee(!pkey_was_autogenerated);

    std::string type;
    uuid_u id;
    if (convert_job_type_and_id_from_datum(primary_key, &type, &id)) {
        if (type != "query") {
            *error_out = strprintf("Jobs of type `%s` cannot be interrupted.",
                                   type.c_str());
            return false;
        }

        typedef std::map<peer_id_t, cluster_directory_metadata_t> peers_t;
        peers_t peers = directory_view->get().get_inner();
        pmap(peers.begin(), peers.end(), [&](peers_t::value_type const &peer) {
            send(mailbox_manager,
                 peer.second.jobs_mailbox.job_interrupt_mailbox_address,
                 id);
        });
    }

    return true;
}
