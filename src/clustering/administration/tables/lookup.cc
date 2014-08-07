// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/tables/lookup.hpp"

#include "clustering/administration/metadata.hpp"

namespace_id_t lookup_table_with_database(
        const name_string_t &database,
        const name_string_t &table,
        const databases_semilattice_metadata_t &database_md,
        const cow_ptr_t<namespaces_semilattice_metadata_t> &namespace_md) {
    const_metadata_searcher_t<database_semilattice_metadata_t> db_searcher(
        &database_md.databases);
    metadata_search_status_t db_search_status;
    auto it = db_searcher.find_uniq(database, &db_search_status);
    switch (db_search_status) {
        case METADATA_SUCCESS:
            break;
        case METADATA_ERR_NONE:
            return nil_uuid();
        case METADATA_ERR_MULTIPLE:
        case METADATA_CONFLICT:
            /* RSI(reql_admin): The reason why we don't handle this case yet is that this
            situation will probably become impossible. Vector clocks will likely go away
            (see #2784) and we'll probably implement some sort of automated resolution
            for database name collisions. */
            crash("There's a name collision or vector clock conflict and we don't "
                "handle this case yet."); 
        default: unreachable();
    }
    database_id_t db_id = it->first;
    const_metadata_searcher_t<namespace_semilattice_metadata_t> ns_searcher(
            &namespace_md.get()->namespaces);
    metadata_search_status_t ns_search_status;
    auto it2 = ns_searcher.find_uniq(namespace_predicate_t(&table, &db_id),
                                     &ns_search_status);
    switch (ns_search_status) {
        case METADATA_SUCCESS:
            break;
        case METADATA_ERR_NONE:
            return nil_uuid();
        case METADATA_ERR_MULTIPLE:
        case METADATA_CONFLICT:
            /* RSI(reql_admin): The reason why we don't handle this case yet is that this
            situation will probably become impossible. Vector clocks will likely go away
            and we'll probably implement some sort of automated resoution for table name
            collisions. */
            crash("There's a name collision or vector clock conflict and we don't "
                "handle this case yet.");
        default: unreachable();
    }
    return it2->first;
}

