// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_TABLE_COMMON_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_TABLE_COMMON_HPP_

#include <string>
#include <vector>

#include "errors.hpp"
#include <boost/shared_ptr.hpp>

#include "clustering/administration/admin_op_exc.hpp"
#include "clustering/administration/metadata.hpp"
#include "rdb_protocol/artificial_table/caching_cfeed_backend.hpp"
#include "rpc/semilattice/view.hpp"

/* This is a base class for the `rethinkdb.table_config` and `rethinkdb.table_status`
pseudo-tables. Subclasses should implement `format_row()` and `write_row()`. */

class common_table_artificial_table_backend_t :
    public timer_cfeed_artificial_table_backend_t
{
public:
    common_table_artificial_table_backend_t(
            boost::shared_ptr< semilattice_readwrite_view_t<
                cluster_semilattice_metadata_t> > _semilattice_view,
            table_meta_client_t *_table_meta_client,
            admin_identifier_format_t _identifier_format);

    std::string get_primary_key_name();

    bool read_all_rows_as_vector(
            signal_t *interruptor_on_caller,
            std::vector<ql::datum_t> *rows_out,
            std::string *error_out);

    bool read_row(
            ql::datum_t primary_key,
            signal_t *interruptor_on_caller,
            ql::datum_t *row_out,
            std::string *error_out);

protected:
    /* This will always be called on the home thread */
    virtual void format_row(
            const namespace_id_t &table_id,
            const table_config_and_shards_t &config,
            /* In theory `db_name_or_uuid` can be computed from `config`, but the
            computation is non-trivial, so it's easier if `table_common` does it for all
            the subclasses. */
            const ql::datum_t &db_name_or_uuid,
            signal_t *interruptor_on_home,
            ql::datum_t *row_out)
        THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t) = 0;

    ql::datum_t make_error_row(
            const namespace_id_t &table_id,
            const ql::datum_t &db_name_or_uuid,
            const name_string_t &table_name);

    boost::shared_ptr< semilattice_readwrite_view_t<
        cluster_semilattice_metadata_t> > semilattice_view;
    table_meta_client_t *table_meta_client;
    admin_identifier_format_t identifier_format;
};

#endif /* CLUSTERING_ADMINISTRATION_TABLES_TABLE_COMMON_HPP_ */

