// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_SERVER_COMMON_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_SERVER_COMMON_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/tables/database_metadata.hpp"
#include "clustering/administration/tables/table_metadata.hpp"
#include "clustering/administration/servers/config_client.hpp"
#include "rdb_protocol/artificial_table/caching_cfeed_backend.hpp"
#include "rdb_protocol/datum.hpp"
#include "rpc/semilattice/view.hpp"

/* This is a base class for the `rethinkdb.server_config` and `rethinkdb.server_status`
pseudo-tables. Subclasses should implement `format_row()` and `write_row()`, in terms of
`lookup()`. */

class common_server_artificial_table_backend_t :
    public caching_cfeed_artificial_table_backend_t {
public:
    common_server_artificial_table_backend_t(
            server_config_client_t *_server_config_client,
            watchable_map_t<peer_id_t, cluster_directory_metadata_t> *_directory);

    std::string get_primary_key_name();

    bool read_all_rows_as_vector(
            signal_t *interruptor,
            std::vector<ql::datum_t> *rows_out,
            std::string *error_out);

    bool read_row(
            ql::datum_t primary_key,
            signal_t *interruptor,
            ql::datum_t *row_out,
            std::string *error_out);

protected:
    virtual bool format_row(
            server_id_t const & server_id,
            peer_id_t const & peer_id,
            cluster_directory_metadata_t const & directory_entry,
            ql::datum_t *row_out,
            std::string *error_out) = 0;

    /* `lookup()` returns `true` if it finds a row corresponding to the given
    `primary_key` and `false` if it does not find a row. It never produces an error. It
    should only be called on the home thread. */
    bool lookup(
            ql::datum_t primary_key,
            server_id_t *server_id_out,
            peer_id_t *peer_id_out,
            cluster_directory_metadata_t *metadata_out);

    watchable_map_t<peer_id_t, cluster_directory_metadata_t> *directory;
    server_config_client_t *server_config_client;

    /* We use `directory_subs` to notify the `caching_cfeed_..._backend_t` that a row has
    changed. */
    watchable_map_t<peer_id_t, cluster_directory_metadata_t>::all_subs_t directory_subs;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_SERVER_COMMON_HPP_ */

