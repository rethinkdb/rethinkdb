// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_WAIT_FOR_READINESS_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_WAIT_FOR_READINESS_HPP_

#include "protocol_api.hpp"

class namespace_repo_t;
class signal_t;
class table_meta_client_t;

/* Blocks until the given table is available at the given level of readiness. If the
table is deleted, throws `no_such_table_exc_t`. The return value is `true` if the table
was ready immediately, or `false` if it was necessary to wait. */
bool wait_for_table_readiness(
    const namespace_id_t &table_id,
    table_readiness_t readiness,
    namespace_repo_t *namespace_repo,
    table_meta_client_t *table_meta_client,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t, no_such_table_exc_t);

/* Blocks until all of the tables in the given set are either deleted or ready at the
given level of readiness. */
void wait_for_many_tables_readiness(
    const std::set<namespace_id_t> &tables,
    table_readiness_t readiness,
    namespace_repo_t *namespace_repo,
    table_meta_client_t *table_meta_client,
    signal_t *interruptor)
    THROWS_ONLY(interrupted_exc_t);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_WAIT_FOR_READINESS_HPP_ */

