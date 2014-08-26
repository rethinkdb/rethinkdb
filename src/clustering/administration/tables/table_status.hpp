// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_TABLE_STATUS_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_TABLE_STATUS_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "concurrency/watchable.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rpc/semilattice/view.hpp"

class server_name_client_t;

class table_status_artificial_table_backend_t :
    public artificial_table_backend_t
{
public:
    table_status_artificial_table_backend_t(
            boost::shared_ptr< semilattice_readwrite_view_t<
                cow_ptr_t<namespaces_semilattice_metadata_t> > > _table_sl_view,
            boost::shared_ptr< semilattice_readwrite_view_t<
                databases_semilattice_metadata_t> > _database_sl_view,
            clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
                namespaces_directory_metadata_t> > > _directory_view,
            server_name_client_t *_name_client) :
        table_sl_view(_table_sl_view),
        database_sl_view(_database_sl_view),
        directory_view(_directory_view),
        name_client(_name_client) { }

    std::string get_primary_key_name();
    bool read_all_primary_keys(
            signal_t *interruptor,
            std::vector<ql::datum_t> *keys_out,
            std::string *error_out);
    bool read_row(
            ql::datum_t primary_key,
            signal_t *interruptor,
            ql::datum_t *row_out,
            std::string *error_out);
    bool write_row(
            ql::datum_t primary_key,
            ql::datum_t new_value,
            signal_t *interruptor,
            std::string *error_out);

private:
    boost::shared_ptr< semilattice_readwrite_view_t<
        cow_ptr_t<namespaces_semilattice_metadata_t> > > table_sl_view;
    boost::shared_ptr< semilattice_readwrite_view_t<
        databases_semilattice_metadata_t> > database_sl_view;
    clone_ptr_t< watchable_t< change_tracking_map_t<peer_id_t,
        namespaces_directory_metadata_t> > > directory_view;
    server_name_client_t *name_client;
};

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_STATUS_HPP_ */

