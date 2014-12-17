// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/stats/stats_backend.hpp"

#include "clustering/administration/stats/request.hpp"
#include "concurrency/cross_thread_signal.hpp"

stats_artificial_table_backend_t::stats_artificial_table_backend_t(
        const clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > >
                &_directory_view,
        boost::shared_ptr<semilattice_read_view_t<cluster_semilattice_metadata_t> >
            _cluster_sl_view,
        server_config_client_t *_server_config_client,
        mailbox_manager_t *_mailbox_manager,
        admin_identifier_format_t _admin_format) :
    directory_view(_directory_view),
    cluster_sl_view(_cluster_sl_view),
    server_config_client(_server_config_client),
    mailbox_manager(_mailbox_manager),
    admin_format(_admin_format) { }

stats_artificial_table_backend_t::~stats_artificial_table_backend_t() {
    begin_changefeed_destruction();
}

std::string stats_artificial_table_backend_t::get_primary_key_name() {
    return std::string("id");
}

// Since this is called through pmap, it will not throw an `interrupted_exc_t`. Instead,
// the behavior is undefined if `interruptor` is pulsed.
void stats_artificial_table_backend_t::get_peer_stats(
        const peer_id_t &peer,
        const std::set<std::vector<std::string> > &filter,
        ql::datum_t *result_out,
        signal_t *interruptor) {
    // Loop up peer in directory - find get stats mailbox
    get_stats_mailbox_address_t request_addr;
    directory_view->apply_read(
        [&](const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> *dir) {
            auto const peer_it = dir->get_inner().find(peer);
            if (peer_it != dir->get_inner().end()) {
                request_addr = peer_it->second.get_stats_mailbox_address;
            }
        });

    if (!request_addr.is_nil()) {
        try {
            std::string dummy_error;
            if (!fetch_stats_from_server(mailbox_manager, request_addr, filter, interruptor,
                    result_out, &dummy_error)) {
                /* Signal an error with an empty `datum_t` */
                *result_out = ql::datum_t();
            }
        } catch (const interrupted_exc_t &) {
            /* It doesn't matter what we return */
        }
    }
}

void stats_artificial_table_backend_t::perform_stats_request(
        const std::vector<peer_id_t> &peers,
        const std::set<std::vector<std::string> > &filter,
        std::vector<ql::datum_t> *results_out,
        signal_t *interruptor) {
    results_out->resize(peers.size());
    pmap(peers.size(),
        [&](int64_t index) {
            get_peer_stats(peers[index], filter, &results_out->at(index), interruptor);
        });
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
}

// A row is excluded if it fails to convert to a datum - which should only
// happen if the entity was deleted from the metadata
void maybe_append_result(const stats_request_t &request,
                         const parsed_stats_t &parsed_stats,
                         const cluster_semilattice_metadata_t &metadata,
                         server_config_client_t *server_config_client,
                         admin_identifier_format_t admin_format,
                         std::vector<ql::datum_t> *rows_out) {
    ql::datum_t row;
    if (request.to_datum(parsed_stats, metadata, server_config_client, admin_format,
            &row)) {
        rows_out->push_back(row);
    }
}

bool stats_artificial_table_backend_t::read_all_rows_as_vector(
        signal_t *interruptor,
        std::vector<ql::datum_t> *rows_out,
        UNUSED std::string *error_out) {
    cross_thread_signal_t ct_interruptor(interruptor, home_thread());
    on_thread_t rethreader(home_thread());
    rows_out->clear();

    std::set<std::vector<std::string> > filter = stats_request_t::global_stats_filter();
    std::vector<peer_id_t> peers =
        stats_request_t::all_peers(directory_view->get().get_inner());

    // Save the metadata from when we sent the requests to avoid race conditions
    cluster_semilattice_metadata_t metadata = cluster_sl_view->get();

    std::vector<ql::datum_t> results;
    perform_stats_request(peers, filter, &results, &ct_interruptor);
    parsed_stats_t parsed_stats(results);

    // Start building results
    rows_out->clear();
    maybe_append_result(cluster_stats_request_t(), parsed_stats, metadata,
        server_config_client, admin_format, rows_out);

    for (auto const &server_pair : metadata.servers.servers) {
        if (server_pair.second.is_deleted()) {
            continue;
        }
        maybe_append_result(server_stats_request_t(server_pair.first), parsed_stats,
            metadata, server_config_client, admin_format, rows_out);
    }

    for (auto const &table_pair : metadata.rdb_namespaces->namespaces) {
        if (table_pair.second.is_deleted()) {
            continue;
        }
        maybe_append_result(table_stats_request_t(table_pair.first), parsed_stats,
            metadata, server_config_client, admin_format, rows_out);
    }

    for (auto const &server_pair : metadata.servers.servers) {
        if (server_pair.second.is_deleted()) {
            continue;
        }
        for (auto const &table_pair : metadata.rdb_namespaces->namespaces) {
            if (table_pair.second.is_deleted()) {
                continue;
            }
            maybe_append_result(
                table_server_stats_request_t(table_pair.first, server_pair.first),
                parsed_stats, metadata, server_config_client, admin_format, rows_out);
        }
    }

    return true;
}

template <class T>
bool parse_stats_request(const ql::datum_t &info,
                         scoped_ptr_t<stats_request_t> *request_out) {
    if (info.get(0).as_str() == T::get_name()) {
        if (!T::parse(info, request_out)) {
            return false;
        }
    }
    return true;
}

bool stats_artificial_table_backend_t::read_row(
        ql::datum_t primary_key,
        signal_t *interruptor,
        ql::datum_t *row_out,
        UNUSED std::string *error_out) {
    cross_thread_signal_t ct_interruptor(interruptor, home_thread());
    on_thread_t rethreader(home_thread());

    // Check format - any incorrect format means the row doesn't exist
    if (primary_key.get_type() != ql::datum_t::R_ARRAY ||
        primary_key.arr_size() == 0 ||
        primary_key.get(0).get_type() != ql::datum_t::R_STR) {
        *row_out = ql::datum_t();
        return true;
    }

    scoped_ptr_t<stats_request_t> request;
    if (!parse_stats_request<cluster_stats_request_t>(primary_key, &request) ||
        !parse_stats_request<table_stats_request_t>(primary_key, &request) ||
        !parse_stats_request<server_stats_request_t>(primary_key, &request) ||
        !parse_stats_request<table_server_stats_request_t>(primary_key, &request)) {
        *row_out = ql::datum_t();
        return true;
    }

    // Save the metadata from when we sent the request to avoid race conditions
    cluster_semilattice_metadata_t metadata = cluster_sl_view->get();
    std::vector<ql::datum_t> results_map;

    if (!request.has() ||
        !request->check_existence(metadata)) {
        *row_out = ql::datum_t();
        return true;
    }

    std::vector<peer_id_t> peers = request->get_peers(directory_view->get().get_inner(),
                                                      server_config_client);
    perform_stats_request(peers, request->get_filter(), &results_map, &ct_interruptor);

    parsed_stats_t parsed_stats(results_map);
    bool to_datum_res = request->to_datum(parsed_stats, metadata, server_config_client,
                                          admin_format, row_out);

     // The stats request target should have been located earlier in `check_existence`
    r_sanity_check(to_datum_res);
    return true;
}

bool stats_artificial_table_backend_t::write_row(
        UNUSED ql::datum_t primary_key,
        UNUSED bool pkey_was_autogenerated,
        UNUSED ql::datum_t *new_value_inout,
        UNUSED signal_t *interruptor,
        std::string *error_out) {
    assert_thread();
    *error_out = "It's illegal to write to the `rethinkdb.stats` table.";
    return false;
}
