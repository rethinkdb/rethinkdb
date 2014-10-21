// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_TABLE_COMMON_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_TABLE_COMMON_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/database_metadata.hpp"
#include "clustering/administration/namespace_metadata.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rpc/semilattice/view.hpp"

/* This is a base class for the `rethinkdb.table_config` and `rethinkdb.table_status`
pseudo-tables. Subclasses should implement `format_row()` and `write_row()`. */

class common_table_artificial_table_backend_t :
    public artificial_table_backend_t
{
public:
    common_table_artificial_table_backend_t(
            boost::shared_ptr< semilattice_readwrite_view_t<
                cow_ptr_t<namespaces_semilattice_metadata_t> > > _table_sl_view,
            boost::shared_ptr< semilattice_readwrite_view_t<
                databases_semilattice_metadata_t> > _database_sl_view) :
        table_sl_view(_table_sl_view),
        database_sl_view(_database_sl_view) {
        table_sl_view->assert_thread();
        database_sl_view->assert_thread();
    }

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
    /* This will always be called on the home thread */
    virtual bool format_row(
            namespace_id_t table_id,
            name_string_t table_name,
            name_string_t db_name,
            const namespace_semilattice_metadata_t &metadata,
            signal_t *interruptor,
            ql::datum_t *row_out,
            std::string *error_out) = 0;

    /* This should only be called on the home thread */
    name_string_t get_db_name(database_id_t db_id);

    /* This should only be called on the home thread. If the DB is not found or there is
    a name collision, returns `false` and sets `*error_out`. */
    bool get_db_id(name_string_t db_name, database_id_t *db_id_out,
        std::string *error_out);

    boost::shared_ptr< semilattice_readwrite_view_t<
        cow_ptr_t<namespaces_semilattice_metadata_t> > > table_sl_view;
    boost::shared_ptr< semilattice_readwrite_view_t<
        databases_semilattice_metadata_t> > database_sl_view;
};

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_COMMON_HPP_ */

