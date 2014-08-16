// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_GEO_TRAVERSAL_HPP_
#define RDB_PROTOCOL_GEO_TRAVERSAL_HPP_

#include <set>
#include <utility>
#include <vector>

#include "errors.hpp"
#include <boost/optional.hpp>

#include "btree/keys.hpp"
#include "btree/slice.hpp"
#include "btree/types.hpp"
#include "containers/counted.hpp"
#include "rdb_protocol/geo/ellipsoid.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/indexing.hpp"
#include "rdb_protocol/geo/lat_lon_types.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {
class datum_t;
class func_t;
class op_t;
class exc_t;
class base_exc_t;
}
namespace profile {
class disabler_t;
class sampler_t;
}


class geo_sindex_data_t {
public:
    geo_sindex_data_t(
            const key_range_t &_pkey_range,
            const ql::map_wire_func_t &_wire_func,
            reql_version_t _func_reql_version,
            sindex_multi_bool_t _multi) :
        pkey_range(_pkey_range),
        func(_wire_func.compile_wire_func()),
        func_reql_version(_func_reql_version),
        multi(_multi) { }
private:
    friend class geo_intersecting_cb_t;
    const key_range_t pkey_range;
    const counted_t<const ql::func_t> func;
    const reql_version_t func_reql_version;
    const sindex_multi_bool_t multi;
};

// Calls `emit_result()` exactly once for each document that's intersecting
// with the query_geometry.
class geo_intersecting_cb_t : public geo_index_traversal_helper_t {
public:
    geo_intersecting_cb_t(
            btree_slice_t *_slice,
            geo_sindex_data_t &&_sindex,
            ql::env_t *_env,
            std::set<store_key_t> *_distinct_emitted_in_out);
    virtual ~geo_intersecting_cb_t() { }

    void init_query(const counted_t<const ql::datum_t> &_query_geometry);

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
            THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t) = 0;

    virtual void emit_result(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t) = 0;

    virtual void emit_error(
            const ql::exc_t &error)
            THROWS_ONLY(interrupted_exc_t) = 0;

private:
    btree_slice_t *slice;
    geo_sindex_data_t sindex;
    counted_t<const ql::datum_t> query_geometry;

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
            geo_sindex_data_t &&_sindex,
            ql::env_t *_env,
            const counted_t<const ql::datum_t> &_query_geometry);

    void finish(intersecting_geo_read_response_t *resp_out);

protected:
    bool post_filter(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t);

    void emit_result(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t);

    void emit_error(
            const ql::exc_t &error)
            THROWS_ONLY(interrupted_exc_t);

private:
    // Accumulates the data until finish() is called.
    ql::datum_array_builder_t result_acc;
    boost::optional<ql::exc_t> error;

    std::set<store_key_t> distinct_emitted;
};


class nearest_traversal_state_t {
public:
    nearest_traversal_state_t(
            const lat_lon_point_t &_center,
            uint64_t _max_results,
            double _max_radius,
            const ellipsoid_spec_t &_reference_ellipsoid);

    // Modifies the state such that the next batch will be generated the next
    // time this state is used.
    done_traversing_t proceed_to_next_batch();
private:
    friend class nearest_traversal_cb_t;

    /* State that changes over time */
    std::set<store_key_t> distinct_emitted;
    size_t previous_size;
    // Which radius around `center` has been previously processed?
    double processed_inradius;
    double current_inradius;

    /* Constant data, initialized by the constructor */
    const lat_lon_point_t center;
    const uint64_t max_results;
    const double max_radius;
    const ellipsoid_spec_t reference_ellipsoid;
};

// Generates a batch of results, sorted by increasing distance.
class nearest_traversal_cb_t : public geo_intersecting_cb_t {
public:
    nearest_traversal_cb_t(
            btree_slice_t *_slice,
            geo_sindex_data_t &&_sindex,
            ql::env_t *_env,
            nearest_traversal_state_t *_state);

    void finish(nearest_geo_read_response_t *resp_out);

protected:
    bool post_filter(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t);

    void emit_result(
            const counted_t<const ql::datum_t> &sindex_val,
            const counted_t<const ql::datum_t> &val)
            THROWS_ONLY(interrupted_exc_t, ql::base_exc_t, geo_exception_t);

    void emit_error(
            const ql::exc_t &error)
            THROWS_ONLY(interrupted_exc_t);

private:
    void init_query_geometry();

    // Accumulate results for the current batch until finish() is called
    std::vector<std::pair<double, counted_t<const ql::datum_t> > > result_acc;
    boost::optional<ql::exc_t> error;

    nearest_traversal_state_t *state;
};

#endif  // RDB_PROTOCOL_GEO_TRAVERSAL_HPP_
