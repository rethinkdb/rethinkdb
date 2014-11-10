// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/jobs/backend.hpp"

#include <set>

#include "clustering/administration/datum_adapter.hpp"
#include "clustering/administration/jobs/manager.hpp"
#include "clustering/administration/main/watchable_fields.hpp"
#include "concurrency/cross_thread_signal.hpp"

/* class jobs_response_record_t {
public:
    explicit jobs_response_record_t(
        mailbox_manager_t *mailbox_manager)
        : response_mailbox(mailbox_manager,
            [this](signal_t *, const std::vector<job_wire_entry_t> &entries) {
                promise.pulse(entries);
            }) {
    }

    promise_t<std::vector<job_wire_entry_t> > promise;
    mailbox_t<void(std::vector<job_wire_entry_t>)> response_mailbox;
};

// Extends the `job_report_t` with a `std::set<server_id_t>`.
class extended_job_wire_entry_t : public job_report_t {
public:
    extended_job_wire_entry_t(job_wire_entry_t entry) : job_wire_entry_t(entry) { }

    std::vector<server_id_t> servers;
}; */

jobs_artificial_table_backend_t::jobs_artificial_table_backend_t(
        mailbox_manager_t *_mailbox_manager,
        const clone_ptr_t<watchable_t<change_tracking_map_t<
            peer_id_t, cluster_directory_metadata_t> > > &_directory_view)
    : mailbox_manager(_mailbox_manager),
      directory_view(_directory_view) {
}

std::string jobs_artificial_table_backend_t::get_primary_key_name() {
    return "id";
}

bool jobs_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        UNUSED std::string *error_out) {
    cross_thread_signal_t ct_interruptor(interruptor, home_thread());
    on_thread_t rethreader(home_thread());
    rows_out->clear();

    std::map<server_id_t, std::vector<job_report_t> > server_reports;

    typedef std::map<peer_id_t, cluster_directory_metadata_t> peers_t;
    peers_t peers = directory_view->get().get_inner();
    pmap(peers.begin(), peers.end(), [&](peers_t::value_type const &peer) {
        cond_t got_report;
        disconnect_watcher_t disconnect_watcher(mailbox_manager, peer.first);

        mailbox_t<void(std::vector<job_report_t>)> return_mailbox(
            mailbox_manager,
            [&](UNUSED signal_t *, std::vector<job_report_t> const &reports) {
                server_reports[peer.second.server_id] = std::move(reports);
                got_report.pulse();
            });
        send(mailbox_manager,
             peer.second.jobs_mailbox.get_job_reports_mailbox_address,
             return_mailbox.get_address());

        wait_any_t waiter(&got_report, &disconnect_watcher);
        wait_interruptible(&waiter, &ct_interruptor);
    });

    for (auto const &server : server_reports) {
        for (auto const &report : server.second) {
            ql::datum_object_builder_t builder;
            builder.overwrite("id", convert_uuid_to_datum(report.id));
            builder.overwrite("type", convert_string_to_datum(report.type));
            builder.overwrite("duration", ql::datum_t(report.duration));
            rows_out->push_back(std::move(builder).to_datum());
        }
    }

    return true;

    /* std::map<server_id_t, scoped_ptr_t<jobs_response_record_t> > responses;

    std::map<peer_id_t, cluster_directory_metadata_t> peers =
        directory_view->get().get_inner();
    for (auto const &peer : peers) {
        server_id_t machine = peer.second.server_id;
        jobs_response_record_t *record = new jobs_response_record_t(mailbox_manager);
        responses.insert(
            std::make_pair(machine, scoped_ptr_t<jobs_response_record_t>(record)));
        send(mailbox_manager,
             peer.second.jobs_mailbox.get_job_wire_entries_mailbox_address,
             record->response_mailbox.get_address());
    }

    std::map<uuid_u, extended_job_wire_entry_t> entries;
    for (auto const &response : responses) {
        const signal_t *ready = response.second->promise.get_ready_signal();
        wait_any_t waiter(* &timer, * ready, &ct_interruptor);
        waiter.wait();

        if (ready->is_pulsed()) {
            const std::vector<job_wire_entry_t> jobs = response.second->promise.wait();
            for (auto const &job : jobs) {
                * auto entries_it = entries.find(job.id);
                if (entries_it == entries.end()) {
                    entries_it = entries.insert(std::make_pair(job.id, extended_job_wire_entry_t(job))).first;
                }
                entries_it->second.servers.push_back(response.first); *

                auto entries_pair = entries.insert(std::make_pair(job.id, job));
                entries_pair.first->second.servers.push_back(response.first);

                // entries[job.id] = job;
            }
        } else if (interruptor->is_pulsed()) {
            throw interrupted_exc_t();
        }
    }

    for (auto const &entry : entries) {
        ql::datum_object_builder_t builder;
        builder.overwrite("id", convert_uuid_to_datum(entry.first));
        builder.overwrite("servers",
            convert_vector_to_datum<server_id_t>(convert_uuid_to_datum, entry.second.servers));
        builder.overwrite("type",
            convert_string_to_datum(job_type_t_to_string(entry.second.type)));
        builder.overwrite("duration", ql::datum_t(entry.second.duration));
        rows_out->push_back(std::move(builder).to_datum());
    }

    return true; */
}

bool jobs_artificial_table_backend_t::read_row(UNUSED ql::datum_t primary_key,
                                               UNUSED signal_t *interruptor,
                                               ql::datum_t *row_out,
                                               UNUSED std::string *error_out) {
    on_thread_t rethreader(home_thread());
    *row_out = ql::datum_t();

    return true;
}

bool jobs_artificial_table_backend_t::write_row(UNUSED ql::datum_t primary_key,
                                                UNUSED bool pkey_was_autogenerated,
                                                UNUSED ql::datum_t *new_value_inout,
                                                UNUSED signal_t *interruptor,
                                                std::string *error_out) {
    error_out->assign("It's illegal to write to the `rethinkdb.jobs` table.");
    return false;
}

