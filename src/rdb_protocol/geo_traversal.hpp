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
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/shards.hpp"

namespace ql {
class datum_t;
class func_t;
class op_t;
}
class profile::disabler_t;
class profile::sampler_t;

class geo_io_data_t {
public:
    geo_io_data_t(geo_read_response_t *_response, btree_slice_t *_slice)
        : response(_response), slice(_slice) { }
private:
    friend class rget_geo_cb_t;
    geo_read_response_t *const response;
    btree_slice_t *const slice;
};

class geo_sindex_data_t {
public:
    geo_sindex_data_t(const key_range_t &_pkey_range,
                      const counted_t<const ql::datum_t> &_query_geometry,
                      ql::map_wire_func_t wire_func, sindex_multi_bool_t _multi)
        : pkey_range(_pkey_range), query_geometry(_query_geometry),
          func(wire_func.compile_wire_func()), multi(_multi) { }
private:
    friend class rget_geo_cb_t;
    const key_range_t pkey_range;
    const counted_t<const ql::datum_t> query_geometry;
    const counted_t<ql::func_t> func;
    const sindex_multi_bool_t multi;
};

class rget_geo_cb_t : public geo_index_traversal_helper_t {
public:
    rget_geo_cb_t(
            geo_io_data_t &&_io,
            const geo_sindex_data_t &&_sindex,
            ql::env_t *_env,
            const std::vector<ql::transform_variant_t> &_transforms,
            const boost::optional<ql::terminal_variant_t> &_terminal);

    void on_candidate(
            const btree_key_t *key,
            const void *value,
            buf_parent_t parent)
            THROWS_ONLY(interrupted_exc_t);

    void finish() THROWS_ONLY(interrupted_exc_t);
private:
    const geo_io_data_t io; // How do get data in/out.
    const geo_sindex_data_t sindex;

    ql::env_t *env;
    std::vector<scoped_ptr_t<ql::op_t> > transformers;
    scoped_ptr_t<ql::eager_acc_t> accumulator;

    // Stores the primary key of previously processed documents
    std::set<store_key_t> already_processed;

    // State for profiling.
    scoped_ptr_t<profile::disabler_t> disabler;
    scoped_ptr_t<profile::sampler_t> sampler;
};

#endif  // RDB_PROTOCOL_GEO_TRAVERSAL_HPP_
