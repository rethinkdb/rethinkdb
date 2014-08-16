// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/indexing.hpp"

#include <string>
#include <vector>

#include "btree/keys.hpp"
#include "btree/leaf_node.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/scoped.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/geojson.hpp"
#include "rdb_protocol/geo/geo_visitor.hpp"
#include "rdb_protocol/geo/s2/s2cellid.h"
#include "rdb_protocol/geo/s2/s2polygon.h"
#include "rdb_protocol/geo/s2/s2polyline.h"
#include "rdb_protocol/geo/s2/s2regioncoverer.h"
#include "rdb_protocol/geo/s2/strings/strutil.h"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/pseudo_geometry.hpp"

using ql::datum_t;

// TODO (daniel): Consider making this configurable through an opt-arg
//   (...at index creation?)
extern const int GEO_INDEX_GOAL_GRID_CELLS = 8;

class compute_covering_t : public s2_geo_visitor_t<scoped_ptr_t<std::vector<S2CellId> > > {
public:
    explicit compute_covering_t(int goal_cells) {
        coverer_.set_max_cells(goal_cells);
    }

    scoped_ptr_t<std::vector<S2CellId> > on_point(const S2Point &point) {
        scoped_ptr_t<std::vector<S2CellId> > result(new std::vector<S2CellId>());
        result->push_back(S2CellId::FromPoint(point));
        return result;
    }
    scoped_ptr_t<std::vector<S2CellId> > on_line(const S2Polyline &line) {
        scoped_ptr_t<std::vector<S2CellId> > result(new std::vector<S2CellId>());
        coverer_.GetCovering(line, result.get());
        return result;
    }
    scoped_ptr_t<std::vector<S2CellId> > on_polygon(const S2Polygon &polygon) {
        scoped_ptr_t<std::vector<S2CellId> > result(new std::vector<S2CellId>());
        coverer_.GetCovering(polygon, result.get());
        return result;
    }

private:
    S2RegionCoverer coverer_;
};

std::string s2cellid_to_key(S2CellId id) {
    // The important property of the result is that its lexicographic
    // ordering as a string must be equivalent to the integer ordering of id.
    // FastHex64ToBuffer() generates a hex representation of id that fulfills this
    // property (it comes padded with leading '0's).
    char buffer[kFastToBufferSize];
    // "GC" = Geospatial Cell
    return std::string("GC") + FastHex64ToBuffer(id.id(), buffer);
}

S2CellId key_to_s2cellid(const std::string &sid) {
    guarantee(sid.length() >= 2 && sid[0] == 'G' && sid[1] == 'C');
    return S2CellId::FromToken(sid.substr(2));
}

S2CellId btree_key_to_s2cellid(const btree_key_t *key) {
    rassert(key != NULL);
    return key_to_s2cellid(datum_t::extract_secondary(
        std::string(reinterpret_cast<const char *>(key->contents), key->size)));
}

std::vector<std::string> compute_index_grid_keys(
        const counted_t<const ql::datum_t> &key, int goal_cells) {
    rassert(key.has());

    if (!key->is_ptype(ql::pseudo::geometry_string)) {
        throw geo_exception_t("Expected geometry but found " + key->get_type_name() + ".");
    }
    if (goal_cells <= 0) {
        throw geo_exception_t("goal_cells must be positive (and should be >= 4).");
    }

    // Compute a cover of grid cells
    compute_covering_t coverer(goal_cells);
    scoped_ptr_t<std::vector<S2CellId> > covering = visit_geojson(&coverer, key);

    // Generate keys
    std::vector<std::string> result;
    result.reserve(covering->size());
    for (size_t i = 0; i < covering->size(); ++i) {
        result.push_back(s2cellid_to_key((*covering)[i]));
    }

    return result;
}

geo_index_traversal_helper_t::geo_index_traversal_helper_t()
    : abort_(false), is_initialized_(false) { }

geo_index_traversal_helper_t::geo_index_traversal_helper_t(
        const std::vector<std::string> &query_grid_keys)
    : abort_(false), is_initialized_(false) {
    init_query(query_grid_keys);
}

void geo_index_traversal_helper_t::init_query(
        const std::vector<std::string> &query_grid_keys) {
    guarantee(!is_initialized_);
    rassert(query_cells_.empty());
    query_cells_.reserve(query_grid_keys.size());
    for (size_t i = 0; i < query_grid_keys.size(); ++i) {
        query_cells_.push_back(key_to_s2cellid(query_grid_keys[i]));
    }
    is_initialized_ = true;
}

void geo_index_traversal_helper_t::process_a_leaf(buf_lock_t *leaf_node_buf,
        const btree_key_t *left_exclusive_or_null,
        const btree_key_t *right_inclusive_or_null,
        signal_t *interruptor,
        int *population_change_out) THROWS_ONLY(interrupted_exc_t) {
    guarantee(is_initialized_);

    *population_change_out = 0;

    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }

    if (!any_query_cell_intersects(left_exclusive_or_null, right_inclusive_or_null)) {
        return;
    }

    buf_read_t read(leaf_node_buf);
    const leaf_node_t *node = static_cast<const leaf_node_t *>(read.get_data_read());

    for (auto it = leaf::begin(*node); it != leaf::end(*node); ++it) {
        const btree_key_t *key = (*it).first;
        if (abort_ || !key) {
            break;
        }

        const S2CellId key_cell = btree_key_to_s2cellid(key);
        if (any_query_cell_intersects(key_cell.range_min(), key_cell.range_max())) {
            on_candidate(key, (*it).second, buf_parent_t(leaf_node_buf), interruptor);
        }
    }
}

void geo_index_traversal_helper_t::filter_interesting_children(
        UNUSED buf_parent_t parent,
        ranged_block_ids_t *ids_source,
        interesting_children_callback_t *cb) {
    guarantee(is_initialized_);

    for (int i = 0, e = ids_source->num_block_ids(); i < e && !abort_; ++i) {
        block_id_t block_id;
        const btree_key_t *left, *right;
        ids_source->get_block_id_and_bounding_interval(i, &block_id, &left, &right);

        if (any_query_cell_intersects(left, right)) {
            cb->receive_interesting_child(i);
        }
    }

    cb->no_more_interesting_children();
}

void geo_index_traversal_helper_t::abort_traversal() {
    abort_ = true;
}

bool geo_index_traversal_helper_t::any_query_cell_intersects(
        const btree_key_t *left_excl, const btree_key_t *right_incl) {
    // We ignore the fact that left_excl is exclusive and not inclusive.
    // In rare cases this costs us a little bit of efficiency, but saves us
    // some complexity.
    S2CellId left_cell =
        left_excl == NULL
        ? S2CellId::FromFacePosLevel(0, 0, 0) // The smallest valid cell id
        : btree_key_to_s2cellid(left_excl);
    S2CellId right_cell =
        right_incl == NULL
        ? S2CellId::FromFacePosLevel(5, 0, 0) // The largest valid cell id
        : btree_key_to_s2cellid(right_incl);

    // Determine a S2CellId range that is a superset of what's intersecting
    // with anything stored in [left_cell, right_cell].
    int common_level;
    if (left_cell.face() != right_cell.face()) {
        // Case 1: left_cell and right_cell are on different faces of the cube.
        // In that case [left_cell, right_cell] intersects at most with the full
        // range of faces in the range [left_cell.face(), right_cell.range()].
        guarantee(left_cell.face() < right_cell.face());
        common_level = 0;
    } else {
        // Case 2: left_cell and right_cell are on the same face. We locate
        // their smallest common parent. [left_cell, right_cell] can at most
        // intersect with anything below their common parent.
        common_level = std::min(left_cell.level(), right_cell.level());
        while (left_cell.parent(common_level) != right_cell.parent(common_level)) {
            guarantee(common_level > 0);
            --common_level;
        }
    }
    S2CellId range_min = left_cell.parent(common_level).range_min();
    S2CellId range_max = right_cell.parent(common_level).range_max();

    return any_query_cell_intersects(range_min, range_max);
}

bool geo_index_traversal_helper_t::any_query_cell_intersects(
        const S2CellId left_min, const S2CellId right_max) {
    // Check if any of the query cells intersects with the given range
    for (size_t j = 0; j < query_cells_.size(); ++j) {
        if (cell_intersects_with_range(query_cells_[j], left_min, right_max)) {
            return true;
        }
    }
    return false;
}

bool geo_index_traversal_helper_t::cell_intersects_with_range(
        const S2CellId c,
        const S2CellId left_min, const S2CellId right_max) {
    return left_min <= c.range_max() && right_max >= c.range_min();
}
