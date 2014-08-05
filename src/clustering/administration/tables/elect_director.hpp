// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_ELECT_DIRECTOR_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_ELECT_DIRECTOR_HPP_

#include <vector>

#include "containers/uuid.hpp"

class server_name_client_t;
class table_config_t;

/* Returns a list of directors, one per shard, based on the information in `config` and
the current state of the cluster. This is equivalent to issuing an `elect_directors`
command from ReQL. */
std::vector<machine_id_t> table_elect_directors(
        const table_config_t &config,
        server_name_client_t *name_client);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_ELECT_DIRECTOR_HPP_ */

