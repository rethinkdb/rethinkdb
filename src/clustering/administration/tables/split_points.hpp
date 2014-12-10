// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_ADMINISTRATION_TABLES_SPLIT_POINTS_HPP_
#define CLUSTERING_ADMINISTRATION_TABLES_SPLIT_POINTS_HPP_

#include <map>
#include <string>
#include <vector>

#include "btree/keys.hpp"
#include "containers/uuid.hpp"

class real_reql_cluster_interface_t;
class signal_t;
class table_shard_scheme_t;

/* `fetch_distribution` fetches the distribution information from the database. */
bool fetch_distribution(
        const namespace_id_t &table_id,
        real_reql_cluster_interface_t *reql_cluster_interface,
        signal_t *interruptor,
        std::map<store_key_t, int64_t> *counts_out,
        std::string *error_out);

/* `calculate_split_points_with_distribution` generates a set of split points that are
guaranteed to divide the data approximately evenly, using the results of
`fetch_distribution()`. It fails if there are too few documents in the database. */
bool calculate_split_points_with_distribution(
        const std::map<store_key_t, int64_t> &counts,
        size_t num_shards,
        table_shard_scheme_t *split_points_out,
        std::string *error_out);

/* `calculate_split_points_for_uuids` generates a set of split points that will divide
the range of UUIDs evenly. */
void calculate_split_points_for_uuids(
        size_t num_shards,
        table_shard_scheme_t *split_points_out);

/* `calculate_split_points_by_interpolation` generates a set of split points on the
assumption that the given previous set of split points is evenly distributed. If the new
requested number of shards is different from the previous number of shards, it will use
interpolation. */
void calculate_split_points_by_interpolation(
        size_t num_shards,
        const table_shard_scheme_t &old_split_points,
        table_shard_scheme_t *split_points_out);

/* `calculate_split_points_intelligently` picks one of the above methods based on its
input. If the number of shards is being increased, it takes a distribution; if the number
is being decreased, it interpolates; and if the number stays the same, it uses the old
split points. */
bool calculate_split_points_intelligently(
        namespace_id_t table_id,
        real_reql_cluster_interface_t *reql_cluster_interface,
        size_t num_shards,
        const table_shard_scheme_t &old_split_points,
        signal_t *interruptor,
        table_shard_scheme_t *split_points_out,
        std::string *error_out);

#endif /* CLUSTERING_ADMINISTRATION_TABLES_SPLIT_POINTS_HPP_ */

