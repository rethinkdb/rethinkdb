// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_INDEXING_HPP_
#define RDB_PROTOCOL_GEO_INDEXING_HPP_

#include <string>
#include <vector>

#include "btree/concurrent_traversal.hpp"
#include "containers/counted.hpp"
#include "rdb_protocol/geo/s2/s2cellid.h"

namespace ql {
class datum_t;
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

// TODO (daniel): Support compound indexes somehow.
class geo_index_traversal_helper_t : public concurrent_traversal_callback_t {
public:
    geo_index_traversal_helper_t();
    explicit geo_index_traversal_helper_t(const std::vector<std::string> &query_grid_keys);

    void init_query(const std::vector<std::string> &query_grid_keys);

    /* Called for every pair that could potentially intersect with query_grid_keys.
    Note that this might be called multiple times for the same value. */
    virtual void on_candidate(
            const btree_key_t *key,
            const void *value,
            buf_parent_t parent)
            THROWS_ONLY(interrupted_exc_t) = 0;

    /* concurrent_traversal_callback_t interface */
    done_traversing_t handle_pair(
            scoped_key_value_t &&keyvalue,
            concurrent_traversal_fifo_enforcer_signal_t waiter)
            THROWS_ONLY(interrupted_exc_t);
    bool is_range_interesting(const key_range_t &range);

protected:
    // Once called, no further calls to on_candidate() will be made and
    // the traversal will be aborted as quickly as possible.
    void abort_traversal();

private:
    static bool cell_intersects_with_range(
            const S2CellId c, const S2CellId left_min, const S2CellId right_max);
    bool any_query_cell_intersects(const btree_key_t *left_excl,
                                   const btree_key_t *right_incl);
    bool any_query_cell_intersects(const S2CellId left_min,
                                   const S2CellId right_max);

    std::vector<S2CellId> query_cells_;
    bool abort_;
    bool is_initialized_;
};

#endif  // RDB_PROTOCOL_GEO_INDEXING_HPP_
