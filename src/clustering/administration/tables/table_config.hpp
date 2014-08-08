// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_TABLE_CONFIG_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_TABLE_CONFIG_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "containers/uuid.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rpc/semilattice/view.hpp"

class server_name_client_t;

class table_config_artificial_table_backend_t :
    public artificial_table_backend_t
{
public:
    table_config_artificial_table_backend_t(
            machine_id_t _my_machine_id,
            boost::shared_ptr< semilattice_readwrite_view_t<
                cow_ptr_t<namespaces_semilattice_metadata_t> > > _table_sl_view,
            boost::shared_ptr< semilattice_readwrite_view_t<
                databases_semilattice_metadata_t> > _database_sl_view,
            server_name_client_t *_name_client) :
        my_machine_id(_my_machine_id),
        table_sl_view(_table_sl_view),
        database_sl_view(_database_sl_view),
        name_client(_name_client) { }

    std::string get_primary_key_name();
    bool read_all_primary_keys(
            signal_t *interruptor,
            std::vector<counted_t<const ql::datum_t> > *keys_out,
            std::string *error_out);
    bool read_row(
            counted_t<const ql::datum_t> primary_key,
            signal_t *interruptor,
            counted_t<const ql::datum_t> *row_out,
            std::string *error_out);
    bool write_row(
            counted_t<const ql::datum_t> primary_key,
            counted_t<const ql::datum_t> new_value,
            signal_t *interruptor,
            std::string *error_out);

private:
    machine_id_t my_machine_id;
    boost::shared_ptr< semilattice_readwrite_view_t<
        cow_ptr_t<namespaces_semilattice_metadata_t> > > table_sl_view;
    boost::shared_ptr< semilattice_readwrite_view_t<
        databases_semilattice_metadata_t> > database_sl_view;
    server_name_client_t *name_client;
};

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_CONFIG_HPP_ */

