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
        table_meta_client_t *_table_meta_client,
        admin_identifier_format_t _identifier_format)
    : mailbox_manager(_mailbox_manager),
      semilattice_view(_semilattice_view),
      directory_view(_directory_view),
      server_config_client(_server_config_client),
      table_meta_client(_table_meta_client),
      identifier_format(_identifier_format) {
}

jobs_artificial_table_backend_t::~jobs_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

std::string jobs_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

template <typename T>
void insert_or_merge_jobs(std::vector<T> const &jobs, std::map<uuid_u, T> *jobs_out) {
    for (auto const &job : jobs) {
        auto result = jobs_out->insert(std::make_pair(job.id, job));
        if (result.second == false) {
            result.first->second.merge(job);
        }
    }
}

template <typename T>
void jobs_to_datums(std::map<uuid_u, T> const &jobs,
                    admin_identifier_format_t identifier_format,
                    server_config_client_t *server_config_client,
                    table_meta_client_t *table_meta_client,
                    cluster_semilattice_metadata_t const &metadata,
                    std::map<uuid_u, ql::datum_t> *jobs_out) {
    ql::datum_t job_out;
    for (auto const &job : jobs) {
        if (job.second.to_datum(identifier_format, server_config_client,
                table_meta_client, metadata, &job_out)) {
            jobs_out->insert(std::make_pair(job.first, std::move(job_out)));
        }
    }
}

void jobs_artificial_table_backend_t::get_all_job_reports(
        signal_t *interruptor,
        std::map<uuid_u, ql::datum_t> *jobs_out) {
    assert_thread();  // Accessing `directory_view`

    std::map<uuid_u, query_job_report_t> query_jobs_map;
    std::map<uuid_u, disk_compaction_job_report_t> disk_compaction_jobs_map;
    std::map<uuid_u, index_construction_job_report_t> index_construction_jobs_map;
    std::map<uuid_u, backfill_job_report_t> backfill_jobs_map;

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

                insert_or_merge_jobs(query_jobs, &query_jobs_map);
                insert_or_merge_jobs(disk_compaction_jobs, &disk_compaction_jobs_map);
                insert_or_merge_jobs(
                    index_construction_jobs, &index_construction_jobs_map);
                insert_or_merge_jobs(backfill_jobs, &backfill_jobs_map);

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

    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    jobs_to_datums(query_jobs_map, identifier_format, server_config_client,
        table_meta_client, metadata, jobs_out);
    jobs_to_datums(disk_compaction_jobs_map, identifier_format, server_config_client,
        table_meta_client, metadata, jobs_out);
    jobs_to_datums(index_construction_jobs_map, identifier_format, server_config_client,
        table_meta_client, metadata, jobs_out);
    jobs_to_datums(backfill_jobs_map, identifier_format, server_config_client,
        table_meta_client, metadata, jobs_out);
}

bool jobs_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor_on_caller,
        std::vector<ql::datum_t> *rows_out,
        UNUSED admin_err_t *error_out) {
    rows_out->clear();

    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t rethreader(home_thread());

    std::map<uuid_u, ql::datum_t> job_reports;
    get_all_job_reports(
        &interruptor_on_home,
        &job_reports);

    rows_out->reserve(job_reports.size());
    for (auto &&job_report : job_reports) {
        rows_out->push_back(std::move(job_report.second));
    }

    return true;
}

bool jobs_artificial_table_backend_t::read_row(ql::datum_t primary_key,
                                               signal_t *interruptor_on_caller,
                                               ql::datum_t *row_out,
                                               UNUSED admin_err_t *error_out) {
    *row_out = ql::datum_t();

    cross_thread_signal_t interruptor_on_home(interruptor_on_caller, home_thread());
    on_thread_t rethreader(home_thread());

    std::string job_type;
    uuid_u job_id;
    if (convert_job_type_and_id_from_datum(primary_key, &job_type, &job_id)) {
        std::map<uuid_u, ql::datum_t> job_reports;
        get_all_job_reports(
            &interruptor_on_home,
            &job_reports);

        auto const iterator = job_reports.find(job_id);
        if (iterator != job_reports.end() &&
            iterator->second.get_field("type").as_str().to_std() == job_type) {
            *row_out = std::move(iterator->second);
        }
    }

    return true;
}

bool jobs_artificial_table_backend_t::write_row(ql::datum_t primary_key,
                                                bool pkey_was_autogenerated,
                                                ql::datum_t *new_value_inout,
                                                UNUSED signal_t *interruptor_on_caller,
                                                admin_err_t *error_out) {
    on_thread_t rethreader(home_thread());

    if (new_value_inout->has()) {
        *error_out = admin_err_t{
            "The `rethinkdb.jobs` system table only allows deletions, "
            "not inserts or updates.",
            query_state_t::FAILED};
        return false;
    }
    guarantee(!pkey_was_autogenerated);

    std::string type;
    uuid_u id;
    if (convert_job_type_and_id_from_datum(primary_key, &type, &id)) {
        if (type != "query") {
            *error_out = admin_err_t{
                strprintf("Jobs of type `%s` cannot be interrupted.", type.c_str()),
                query_state_t::FAILED};
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
