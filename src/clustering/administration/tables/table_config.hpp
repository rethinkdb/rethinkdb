// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_TABLE_CONFIG_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_TABLE_CONFIG_HPP_

#include "rdb_protocol/artificial_table/backend.hpp"

class table_config_artificial_table_backend_t :
    public artificial_table_backend_t
{
public:
    table_config_artificial_table_backend_t(
            machine_id_t _my_machine_id,
            boost::shared_ptr< semilattice_readwrite_view_t<
                namespace_semilattice_metadata_t> > _semilattice_view) :
        my_machine_id(_my_machine_id),
        semilattice_view(_semilattice_view) { }

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
    publisher_t<std::function<void(counted_t<const ql::datum_t>)> > *get_publisher();

private:
    machine_id_t my_machine_id;
    boost::shared_ptr< semilattice_readwrite_view_t<
            namespace_semilattice_metadata_t> > semilattice_view;
};

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_CONFIG_HPP_ */

