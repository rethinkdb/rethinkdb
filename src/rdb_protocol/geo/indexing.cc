// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/geo/indexing.hpp"

#include <string>
#include <vector>

#include "btree/keys.hpp"
#include "btree/leaf_node.hpp"
#include "concurrency/interruptor.hpp"
#include "concurrency/signal.hpp"
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

using geo::S2CellId;
using geo::S2Point;
using geo::S2Polygon;
using geo::S2Polyline;
using geo::S2RegionCoverer;
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
    char buffer[geo::kFastToBufferSize];
    // "GC" = Geospatial Cell
    return std::string("GC") + geo::FastHex64ToBuffer(id.id(), buffer);
}

S2CellId key_to_s2cellid(const std::string &sid) {
    guarantee(sid.length() >= 2
              // We need the static cast here because `'G' | 0x80` does integer
              // promotion and produces 199, whereas `sid[0]` produces -57
              // because it's a signed char.  C FTW!
              && (sid[0] == 'G' || static_cast<uint8_t>(sid[0]) == ('G' | 0x80))
              && sid[1] == 'C');
    return S2CellId::FromToken(sid.substr(2));
}

S2CellId btree_key_to_s2cellid(const btree_key_t *key) {
    rassert(key != NULL);
    return key_to_s2cellid(
        datum_t::extract_secondary(
            std::string(reinterpret_cast<const char *>(key->contents), key->size)));
}

std::vector<std::string> compute_index_grid_keys(
        const ql::datum_t &key, int goal_cells) {
    rassert(key.has());

    if (!key.is_ptype(ql::pseudo::geometry_string)) {
        throw geo_exception_t(
            "Expected geometry but found " + key.get_type_name() + ".");
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

geo_index_traversal_helper_t::geo_index_traversal_helper_t(const signal_t *interruptor)
    : is_initialized_(false), interruptor_(interruptor) { }

geo_index_traversal_helper_t::geo_index_traversal_helper_t(
        const std::vector<std::string> &query_grid_keys,
        const signal_t *interruptor)
    : is_initialized_(false), interruptor_(interruptor) {
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

continue_bool_t
geo_index_traversal_helper_t::handle_pair(
    scoped_key_value_t &&keyvalue,
    concurrent_traversal_fifo_enforcer_signal_t waiter)
        THROWS_ONLY(interrupted_exc_t) {
    guarantee(is_initialized_);

    if (interruptor_->is_pulsed()) {
        throw interrupted_exc_t();
    }

    const S2CellId key_cell = btree_key_to_s2cellid(keyvalue.key());
    if (any_query_cell_intersects(key_cell.range_min(), key_cell.range_max())) {
        return on_candidate(std::move(keyvalue), waiter);
    } else {
        return continue_bool_t::CONTINUE;
    }
}

bool geo_index_traversal_helper_t::is_range_interesting(
        const btree_key_t *left_excl_or_null,
        const btree_key_t *right_incl) {
    guarantee(is_initialized_);
    // We ignore the fact that the left key is exclusive and not inclusive.
    // In rare cases this costs us a little bit of efficiency because we consider
    // one extra key, but it saves us some complexity.
    return any_query_cell_intersects(left_excl_or_null, right_incl);
}

bool geo_index_traversal_helper_t::any_query_cell_intersects(
        const btree_key_t *left_incl_or_null, const btree_key_t *right_incl) {
    S2CellId left_cell =
        left_incl_or_null == NULL
        ? S2CellId::FromFacePosLevel(0, 0, 0) // The smallest valid cell id
        : btree_key_to_s2cellid(left_incl_or_null);
    S2CellId right_cell = btree_key_to_s2cellid(right_incl);

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
