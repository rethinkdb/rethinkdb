// Copyright 2010-2016 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_INDEXING_HPP_
#define RDB_PROTOCOL_GEO_INDEXING_HPP_

#include <string>
#include <vector>

#include "btree/concurrent_traversal.hpp"
#include "containers/counted.hpp"
#include "rdb_protocol/geo/s2/s2cellid.h"

namespace ql {
class datum_t;
enum class skey_version_t;
}
class signal_t;


/* Polygons and lines are inserted into an index by computing a coverage of them
consisting of cells on a pre-defined multi-level grid.
This constant determines how many grid cells should be used to cover the polygon/line.
If the number is small, index insertion becomes more efficient, but querying the
index becomes less efficient. High values make geo indexes larger and inserting
into them slower, while usually improving query efficiency.
See the comments in s2regioncoverer.h for further explanation and for statistics
on the effects of different choices of this parameter.*/
extern const int GEO_INDEX_GOAL_GRID_CELLS;

std::vector<std::string> compute_index_grid_keys(
        const ql::datum_t &key,
        int goal_cells);
std::vector<geo::S2CellId> compute_cell_covering(
        const ql::datum_t &key,
        int goal_cells);
std::vector<geo::S2CellId> compute_interior_cell_covering(
        const ql::datum_t &key,
        const std::vector<geo::S2CellId> &exterior_covering);

// TODO (daniel): Support compound indexes somehow.
class geo_index_traversal_helper_t : public concurrent_traversal_callback_t {
public:
    geo_index_traversal_helper_t(
        ql::skey_version_t skey_version, const signal_t *interruptor);

    void init_query(
        const std::vector<geo::S2CellId> &query_cell_covering,
        const std::vector<geo::S2CellId> &query_interior_cell_covering);

    /* Called for every pair that could potentially intersect with query_grid_keys.
    Note that this might be called multiple times for the same value.
    Correct ordering of the call is not guaranteed. Implementations are expected
    to call waiter.wait_interruptible() before performing ordering-sensitive
    operations.
    `definitely_intersects_if_point` is true only if the key is also contained in the
    interior cell covering. If the key corresponds to a point, that implies that the
    point is contained in the query geometry. Note that this doesn't hold for more
    complex geometry, since the keys corresponding to more complex geometry are
    generated from a covering. The way we compute the covering, we do not guarantee that
    each cell in the covering actually intersects with the covered geometry
    (S2RegionCoverer uses `MayIntersect` tests rather than exact `Intersects` tests).
    `definitely_intersects_if_point` can be used to avoid unnecessary intersection
    tests during post-filtering. */
    virtual continue_bool_t on_candidate(
        scoped_key_value_t &&keyvalue,
        concurrent_traversal_fifo_enforcer_signal_t waiter,
        bool definitely_intersects_if_point)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* concurrent_traversal_callback_t interface */
    continue_bool_t handle_pair(scoped_key_value_t &&keyvalue,
                                concurrent_traversal_fifo_enforcer_signal_t waiter)
            THROWS_ONLY(interrupted_exc_t);
    void filter_range(
            const btree_key_t *left_excl_or_null,
            const btree_key_t *right_incl,
            bool *skip_out);

private:
    static bool cell_intersects_with_range(const geo::S2CellId c,
                                           const geo::S2CellId left_min,
                                           const geo::S2CellId right_max);
    bool any_query_cell_intersects(const btree_key_t *left_excl_or_null,
                                   const btree_key_t *right_incl) const;
    static bool any_cell_intersects(const std::vector<geo::S2CellId> &cells,
                                    const geo::S2CellId left_min,
                                    const geo::S2CellId right_max);
    static bool any_cell_contains(const std::vector<geo::S2CellId> &cells,
                                  const geo::S2CellId key);

    std::vector<geo::S2CellId> query_cells_;
    std::vector<geo::S2CellId> query_interior_cells_;
    bool is_initialized_;
    const ql::skey_version_t skey_version_;
    const signal_t *interruptor_;
};

#endif  // RDB_PROTOCOL_GEO_INDEXING_HPP_
