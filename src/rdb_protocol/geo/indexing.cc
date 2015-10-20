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

/* Returns the S2CellId corresponding to the given key, which must be a correctly
formatted sindex key. */
S2CellId btree_key_to_s2cellid(const btree_key_t *key) {
    rassert(key != NULL);
    return key_to_s2cellid(
        datum_t::extract_secondary(
            std::string(reinterpret_cast<const char *>(key->contents), key->size)));
}

/* `key_or_null` represents a point to the left or right of a key in the B-tree
key-space. If `nullptr`, it means the point left of the leftmost key; otherwise, it means
the point right of `*key_or_null`. It need not be a valid sindex key.

`order_btree_key_relative_to_s2cellid_keys()` figures out where `key_or_null` lies
relative to geospatial sindex keys. There are four possible outcomes:
  - `key_or_null` lies within a range of sindex keys for a specific `S2CellId`. It will
    return `(cell ID, true)`.
  - `key_or_null` lies between two ranges of sindex keys for different `S2CellId`s. It
    will return `(cell ID to the right, false)`.
  - `key_or_null` lies after all possible sindex keys for `S2CellId`s. It will return
    `(S2CellId::Sentinel(), false)`.
  - `key_or_null` lies before all possible sindex keys for `S2CellId`s. It will return
    `(S2CellId::FromFacePosLevel(0, 0, geo::S2::kMaxCellLevel), false)`. */
std::pair<S2CellId, bool> order_btree_key_relative_to_s2cellid_keys(
        const btree_key_t *key_or_null,
        ql::skey_version_t skey_version) {
    static const std::pair<S2CellId, bool> before_all(
        S2CellId::FromFacePosLevel(0, 0, geo::S2::kMaxCellLevel), false);
    static const std::pair<S2CellId, bool> after_all(
        S2CellId::Sentinel(), false);

    /* A well-formed sindex key will start with the characters 'GC'. But if the sindex
    version is 1.16 or higher, the high bit on the 'G' will be set. */
    uint8_t first_char;
    switch (skey_version) {
        case ql::skey_version_t::post_1_16:
            first_char = static_cast<uint8_t>('G') | 0x80; break;
        default: unreachable();
    }
    if (key_or_null == nullptr || key_or_null->size == 0) return before_all;
    if (key_or_null->contents[0] < first_char) return before_all;
    if (key_or_null->contents[0] > first_char) return after_all;
    if (key_or_null->size == 1) return before_all;
    if (key_or_null->contents[1] < 'C') return before_all;
    if (key_or_null->contents[1] > 'C') return after_all;

    /* A well-formed sindex key will next have 16 hexadecimal digits, using lowercase
    letters. If `key_or_null` starts with such a well-formed string, we'll set
    `cell_number` to the number represented by that string and `inside_cell` to `true`.
    Otherwise we'll set `cell_number` to the smallest number represented by a larger
    string and `inside_cell()` to `false`. */
    uint64_t cell_number = 0;
    bool inside_cell = true;
    for (int i = 0; i < 16; ++i) {
        if (i + 2 >= key_or_null->size) {
            /* The string is too short. For example, "123" -> (0x1230..., false). */
            inside_cell = false;
            break;
        }
        uint8_t hex_digit = key_or_null->contents[i + 2];
        if (hex_digit >= '0' && hex_digit <= '9') {
            /* The string is still valid, so keep going. */
            cell_number += static_cast<uint64_t>(hex_digit - '0') << (4 * (15 - i));
        } else if (hex_digit >= 'a' && hex_digit <= 'f') {
            /* The string is still valid, so keep going. */
            cell_number +=
                static_cast<uint64_t>(10 + (hex_digit - 'a')) << (4 * (15 - i));
        } else if (hex_digit < '0') {
            /* For example, "123/..." -> (0x1230..., false). ('/' comes before '0' in
            ASCII order.) */
            inside_cell = false;
            break;
        } else if (hex_digit > 'f') {
            /* For example, "123g..." -> (0x1240..., false). */
            if (i == 0) {
                /* This case corresponds to "g123..." -> (sentinel, false). We have to
                handle this separately from the other overflow case below because if
                `i` is zero then `change` won't fit into a 64-bit int. */
                return after_all;
            }
            uint64_t change = static_cast<uint64_t>(16) << (4 * (15 - i));
            if (change > 0xffffffffffffffffull - cell_number) {
                /* This case corresponds to "fffg..." -> (sentinel, false). */
                return after_all;
            }
            cell_number += change;
            inside_cell = false;
            break;
        } else if (hex_digit > '9' && hex_digit < 'a') {
            /* For example, "123:..." -> (0x123a..., false). (':' comes after '9' in
            ASCII order.) */
            cell_number += static_cast<uint64_t>(10) << (4 * (15 - i));
            inside_cell = false;
            break;
        } else {
            unreachable();
        }
    }

    /* Not all 64-bit integers are valid S2 cell IDs. There are two possible problems:
      - The face index can be 6 or 7. In this case, the key is larger than any valid ID,
        since the face index is the most significant three bits.
      - The last bit is not set properly. In this case, we set the first bit that we
        can set that will turn it into a valid cell ID. In this case we have to set
        `inside_cell` to `false`. */
    S2CellId cell_id(cell_number);
    if (cell_id.face() >= 6) return after_all;
    if (!cell_id.is_valid()) {
        inside_cell = false;
        cell_id = S2CellId(cell_number | 1);
        guarantee(cell_id.is_valid());
    }

    return std::make_pair(cell_id, inside_cell);
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

geo_index_traversal_helper_t::geo_index_traversal_helper_t(
        ql::skey_version_t skey_version, const signal_t *interruptor)
    : is_initialized_(false), skey_version_(skey_version), interruptor_(interruptor) { }

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

void geo_index_traversal_helper_t::filter_range(
        const btree_key_t *left_excl_or_null,
        const btree_key_t *right_incl,
        bool *skip_out) {
    guarantee(is_initialized_);
    *skip_out = !any_query_cell_intersects(left_excl_or_null, right_incl);
}

bool geo_index_traversal_helper_t::any_query_cell_intersects(
        const btree_key_t *left_excl_or_null, const btree_key_t *right_incl) {
    std::pair<S2CellId, bool> left =
        order_btree_key_relative_to_s2cellid_keys(left_excl_or_null, skey_version_);
    std::pair<S2CellId, bool> right =
        order_btree_key_relative_to_s2cellid_keys(right_incl, skey_version_);

    /* This is more conservative than necessary. For example, if `left_excl_or_null` is
    after the largest possible cell or `right_incl` is before the smallest possible cell,
    we could shortcut and return `false`, but we don't. Also, if `right.second` were
    `false`, we could use the cell immediately before `right.first` instead of using
    `right.first`. But that would be more trouble than it's worth. */
    S2CellId left_cell = left.first == S2CellId::Sentinel()
        ? S2CellId::FromFacePosLevel(5, 0, 0) : left.first;
    S2CellId right_cell = right.first == S2CellId::Sentinel()
        ? S2CellId::FromFacePosLevel(5, 0, 0) : right.first;

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
