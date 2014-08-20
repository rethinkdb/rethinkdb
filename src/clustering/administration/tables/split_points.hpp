// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_SPLIT_POINTS_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_SPLIT_POINTS_HPP_

#include <vector>

#include "btree/keys.hpp"
#include "containers/uuid.hpp"

class real_reql_cluster_interface_t;
class signal_t;

/* `calculate_split_points_with_distribution` generates a set of split points that are
guaranteed to divide the data approximately evenly. It does this by running a
distribution query against the database. However, it may fail. */
bool calculate_split_points_with_distribution(
        namespace_id_t table_id,
        real_reql_cluster_interface_t *reql_cluster_interface,
        int num_shards,
        signal_t *interruptor,
        std::vector<store_key_t> *split_points_out,
        std::string *error_out);

/* `calculate_split_points_by_estimation` generates a set of split points on the
assumption that the given previous set of split points is evenly distributed. If the new
requested number of shards is different from the previous number of shards, it will use
interpolation. */
/* RSI(reql_admin): This isn't currently used anywhere, but it will be used once #2895 is
fixed. See comments in `generate_config.cc`. */
void calculate_split_points_by_estimation(
        int num_shards,
        const std::vector<store_key_t> &old_split_points,
        std::vector<store_key_t> *split_points_out);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_SPLIT_POINTS_HPP_ */

