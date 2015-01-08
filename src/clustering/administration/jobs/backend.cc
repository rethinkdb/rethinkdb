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
        std::map<uuid_u, job_report_t> *job_reports_out) {
    assert_thread();  // Accessing `directory_view`

    typedef std::map<peer_id_t, cluster_directory_metadata_t> peers_t;
    peers_t peers = directory_view->get().get_inner();
    pmap(peers.begin(), peers.end(), [&](peers_t::value_type const &peer) {
        cond_t returned_job_reports;
        disconnect_watcher_t disconnect_watcher(mailbox_manager, peer.first);

        mailbox_t<void(std::vector<job_report_t>)> return_mailbox(
            mailbox_manager,
            [&](UNUSED signal_t *, std::vector<job_report_t> const &return_job_reports) {
                for (auto const &return_job_report : return_job_reports) {
                    auto result = job_reports_out->insert(
                        std::make_pair(return_job_report.id, return_job_report));

                    // Note `std::map.insert` returns a `std::pair<iterator, bool>`,
                    // here we update the `job_report_t` if it already existed.
                    if (result.second == false) {
                        result.first->second.duration = std::max(
                            result.first->second.duration, return_job_report.duration);
                        result.first->second.is_ready &= return_job_report.is_ready;
                        result.first->second.progress_numerator +=
                            return_job_report.progress_numerator;
                        result.first->second.progress_denominator +=
                            return_job_report.progress_denominator;
                    }
                    result.first->second.servers.insert(peer.second.server_id);
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

    std::map<uuid_u, job_report_t> job_reports;
    get_all_job_reports(&ct_interruptor, &job_reports);

    cluster_semilattice_metadata_t metadata = semilattice_view->get();
    for (auto const &job_report : job_reports) {
        ql::datum_t row;
        if (job_report.second.to_datum(
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

    std::string job_report_type;
    uuid_u job_report_id;
    if (convert_job_type_and_id_from_datum(
            primary_key, &job_report_type, &job_report_id)) {
        std::map<uuid_u, job_report_t> job_reports;
        get_all_job_reports(&ct_interruptor, &job_reports);

        std::map<uuid_u, job_report_t>::const_iterator iterator =
            job_reports.find(job_report_id);
        if (iterator != job_reports.end() && iterator->second.type == job_report_type) {
            cluster_semilattice_metadata_t metadata = semilattice_view->get();
            ql::datum_t row;
            if (iterator->second.to_datum(
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
