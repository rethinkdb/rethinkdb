// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_TRAVERSAL_HPP_
#define RDB_PROTOCOL_GEO_TRAVERSAL_HPP_

#include <set>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "btree/keys.hpp"
#include "btree/slice.hpp"
#include "containers/counted.hpp"
#include "geo/indexing.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {
class datum_t;
class func_t;
class op_t;
class exc_t;
}
namespace profile {
class disabler_t;
class sampler_t;
}


class geo_sindex_data_t {
public:
    geo_sindex_data_t(const key_range_t &_pkey_range,
                      const counted_t<const ql::datum_t> &_query_geometry,
                      ql::map_wire_func_t wire_func, sindex_multi_bool_t _multi)
        : pkey_range(_pkey_range), query_geometry(_query_geometry),
          func(wire_func.compile_wire_func()), multi(_multi) { }
private:
    friend class geo_intersecting_cb_t;
    const key_range_t pkey_range;
    const counted_t<const ql::datum_t> query_geometry;
    const counted_t<ql::func_t> func;
    const sindex_multi_bool_t multi;
};

// Calls `emit_result()` exactly once for each document that's intersecting
// with the query_geometry.
class geo_intersecting_cb_t : public geo_index_traversal_helper_t {
public:
    geo_intersecting_cb_t(
            btree_slice_t *_slice,
            const geo_sindex_data_t &&_sindex,
            ql::env_t *_env,
            std::set<store_key_t> *_distinct_emitted_in_out);
    virtual ~geo_intersecting_cb_t() { }

    void on_candidate(
            const btree_key_t *key,
            const void *value,
            buf_parent_t parent,
            signal_t *interruptor)
            THROWS_ONLY(interrupted_exc_t);

protected:
    virtual bool post_filter(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t) = 0;

    virtual void emit_result(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t) = 0;

    virtual void emit_error(
            const ql::exc_t &error)
            THROWS_ONLY(interrupted_exc_t) = 0;

private:
    btree_slice_t *slice;
    const geo_sindex_data_t sindex;

    ql::env_t *env;

    // Stores the primary key of previously processed documents, up to some limit
    // (this is an optimization for small query ranges, trading memory for efficiency)
    std::set<store_key_t> already_processed;
    // In contrast to `already_processed`, this set is critical to avoid emitting
    // duplicates. It's not just an optimization.
    std::set<store_key_t> *distinct_emitted;

    // State for profiling.
    scoped_ptr_t<profile::disabler_t> disabler;
    scoped_ptr_t<profile::sampler_t> sampler;
};

// Simply accumulates all intersecting results in an array without any
// post-filtering.
class collect_all_geo_intersecting_cb_t : public geo_intersecting_cb_t {
public:
    collect_all_geo_intersecting_cb_t(
            btree_slice_t *_slice,
            const geo_sindex_data_t &&_sindex,
            ql::env_t *_env);

    void finish(intersecting_geo_read_response_t *resp_out);

protected:
    bool post_filter(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t);

    void emit_result(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t);

    void emit_error(
            const ql::exc_t &error)
            THROWS_ONLY(interrupted_exc_t);

private:
    // Accumulates the data until finish() is called.
    ql::datum_ptr_t result_acc;

    boost::optional<ql::exc_t> error;

    std::set<store_key_t> distinct_emitted;
};








// TODO!
/*
struct nearest_traversal_state_t {
    nearest_traversal_state_t(
            const lat_lon_point_t &_center,
            const ellipsoid_spec_t &_reference_ellipsoid);

    std::set<store_key_t> distinct_emitted;
    const lat_lon_point_t center;
    lat_lon_line_t inner_circle;
    lat_lon_line_t outer_circle;
    double next_radius;
    const ellipsoid_spec_t reference_ellipsoid;
    // TODO! Max radius? Max results?
};*/

#endif  // RDB_PROTOCOL_GEO_TRAVERSAL_HPP_
