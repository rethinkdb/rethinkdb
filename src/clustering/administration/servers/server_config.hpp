// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_SERVERS_SERVER_CONFIG_HPP_
#define CLUSTERING_ADMINISTRATION_SERVERS_SERVER_CONFIG_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/servers/machine_metadata.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rpc/semilattice/view.hpp"

class server_name_client_t;

class server_config_artificial_table_backend_t :
    public artificial_table_backend_t
{
public:
    server_config_artificial_table_backend_t(
            boost::shared_ptr< semilattice_read_view_t<
                machines_semilattice_metadata_t> > _servers_sl_view,
            server_name_client_t *_name_client) :
        servers_sl_view(_servers_sl_view),
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
    bool lookup(ql::datum_t primary_key,
                machines_semilattice_metadata_t *machines,
                name_string_t *name_out,
                machine_id_t *machine_id_out,
                machine_semilattice_metadata_t **machine_out);

    boost::shared_ptr< semilattice_read_view_t<
        machines_semilattice_metadata_t> > servers_sl_view;
    server_name_client_t *name_client;
};

#endif /* CLUSTERING_ADMINISTRATION_SERVERS_SERVER_CONFIG_HPP_ */

