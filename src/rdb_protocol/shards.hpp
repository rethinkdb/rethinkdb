// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_SHARDS_HPP_
#define RDB_PROTOCOL_SHARDS_HPP_

#include <algorithm>
#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "btree/concurrent_traversal.hpp"
#include "btree/keys.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/varint.hpp"
#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/configured_limits.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "rdb_protocol/wire_func.hpp"

enum class sorting_t {
    UNORDERED,
    ASCENDING,
    DESCENDING
};
// UNORDERED sortings aren't reversed
bool reversed(sorting_t sorting);

namespace ql {

// This stuff previously resided in the protocol, but has been broken out since
// we want to use this logic in multiple places.
typedef std::vector<counted_t<const ql::datum_t> > datums_t;
typedef std::map<counted_t<const ql::datum_t>, datums_t, counted_datum_less_t> groups_t;

struct rget_item_t {
    rget_item_t() { }
    // Works for both rvalue and lvalue references.
    template<class T>
    rget_item_t(T &&_key,
                const counted_t<const ql::datum_t> &_sindex_key,
                const counted_t<const ql::datum_t> &_data)
        : key(std::forward<T>(_key)), sindex_key(_sindex_key), data(_data) { }
    store_key_t key;
    counted_t<const ql::datum_t> sindex_key, data;
    RDB_DECLARE_ME_SERIALIZABLE;
};

RDB_SERIALIZE_OUTSIDE(rget_item_t);

typedef std::vector<rget_item_t> stream_t;

class optimizer_t {
public:
    optimizer_t();
    optimizer_t(const counted_t<const datum_t> &_row,
                const counted_t<const datum_t> &_val);

    void swap_if_other_better(optimizer_t &other, // NOLINT
                              reql_version_t reql_version,
                              bool (*beats)(reql_version_t,
                                            const counted_t<const datum_t> &val1,
                                            const counted_t<const datum_t> &val2));
    counted_t<const datum_t> unpack(const char *name);
    counted_t<const datum_t> row, val;
};

template <cluster_version_t W>
void serialize_grouped(write_message_t *wm, const optimizer_t &o) {
    serialize<W>(wm, o.row.has());
    if (o.row.has()) {
        r_sanity_check(o.val.has());
        serialize<W>(wm, o.row);
        serialize<W>(wm, o.val);
    }
}
template <cluster_version_t W>
archive_result_t deserialize_grouped(read_stream_t *s, optimizer_t *o) {
    archive_result_t res;
    bool has;
    res = deserialize<W>(s, &has);
    if (bad(res)) { return res; }
    if (has) {
        res = deserialize<W>(s, &o->row);
        if (bad(res)) { return res; }
        res = deserialize<W>(s, &o->val);
        if (bad(res)) { return res; }
    }
    return archive_result_t::SUCCESS;
}

// We write all of these serializations and deserializations explicitly because:
// * It stops people from inadvertently using a new `grouped_t<T>` without thinking.
// * Some grouped elements need specialized serialization.
template <cluster_version_t W>
void serialize_grouped(
    write_message_t *wm, const counted_t<const datum_t> &d) {
    serialize<W>(wm, d.has());
    if (d.has()) {
        serialize<W>(wm, d);
    }
}
template <cluster_version_t W>
void serialize_grouped(write_message_t *wm, uint64_t sz) {
    serialize_varint_uint64(wm, sz);
}
template <cluster_version_t W>
void serialize_grouped(write_message_t *wm, double d) {
    serialize<W>(wm, d);
}
template <cluster_version_t W>
void serialize_grouped(write_message_t *wm,
                       const std::pair<double, uint64_t> &p) {
    serialize<W>(wm, p.first);
    serialize_varint_uint64(wm, p.second);
}
template <cluster_version_t W>
void serialize_grouped(write_message_t *wm, const stream_t &sz) {
    serialize<W>(wm, sz);
}
template <cluster_version_t W>
void serialize_grouped(write_message_t *wm, const datums_t &ds) {
    serialize<W>(wm, ds);
}

template <cluster_version_t W>
archive_result_t deserialize_grouped(
    read_stream_t *s, counted_t<const datum_t> *d) {
    bool has;
    archive_result_t res = deserialize<W>(s, &has);
    if (bad(res)) { return res; }
    if (has) {
        return deserialize<W>(s, d);
    } else {
        d->reset();
        return archive_result_t::SUCCESS;
    }
}
template <cluster_version_t W>
archive_result_t deserialize_grouped(read_stream_t *s, uint64_t *sz) {
    return deserialize_varint_uint64(s, sz);
}
template <cluster_version_t W>
archive_result_t deserialize_grouped(read_stream_t *s, double *d) {
    return deserialize<W>(s, d);
}
template <cluster_version_t W>
archive_result_t deserialize_grouped(read_stream_t *s,
                                     std::pair<double, uint64_t> *p) {
    archive_result_t res = deserialize<W>(s, &p->first);
    if (bad(res)) { return res; }
    return deserialize_varint_uint64(s, &p->second);
}
template <cluster_version_t W>
archive_result_t deserialize_grouped(read_stream_t *s, stream_t *sz) {
    return deserialize<W>(s, sz);
}
template <cluster_version_t W>
archive_result_t deserialize_grouped(read_stream_t *s, datums_t *ds) {
    return deserialize<W>(s, ds);
}

// Passing this means the caller affirms that they're not iterating a grouped_t in a
// way such that its map order matters.  (This means that its counted_datum_less_t
// can use any reql version, and it doesn't need the value as a parameter.)
namespace grouped {
enum class order_doesnt_matter_t { };
}

// This is basically a templated typedef with special serialization.
template<class T>
class grouped_t {
public:
    // We assume v1_14 ordering.  We could get fancy and allow either v1_13 or v1_14
    // ordering, but usage of grouped_t inside of secondary index functions is the
    // only place where we'd want v1_13 ordering, so let's not bother.
    explicit grouped_t() : m(counted_datum_less_t(reql_version_t::v1_14_is_latest)) { }
    virtual ~grouped_t() { } // See grouped_data_t below.
    template <cluster_version_t W>
    typename std::enable_if<W == cluster_version_t::CLUSTER, void>::type
    rdb_serialize(write_message_t *wm) const {
        serialize_varint_uint64(wm, m.size());
        for (auto it = m.begin(); it != m.end(); ++it) {
            serialize_grouped<W>(wm, it->first);
            serialize_grouped<W>(wm, it->second);
        }
    }
    template <cluster_version_t W>
    typename std::enable_if<W == cluster_version_t::CLUSTER, archive_result_t>::type
    rdb_deserialize(read_stream_t *s) {
        guarantee(m.empty());

        uint64_t sz;
        archive_result_t res = deserialize_varint_uint64(s, &sz);
        if (bad(res)) { return res; }
        if (sz > std::numeric_limits<size_t>::max()) {
            return archive_result_t::RANGE_ERROR;
        }
        auto pos = m.begin();
        for (uint64_t i = 0; i < sz; ++i) {
            std::pair<counted_t<const datum_t>, T> el;
            res = deserialize_grouped<W>(s, &el.first);
            if (bad(res)) { return res; }
            res = deserialize_grouped<W>(s, &el.second);
            if (bad(res)) { return res; }
            pos = m.insert(pos, std::move(el));
        }
        return archive_result_t::SUCCESS;
    }

    // You're not allowed to use, in any way, the intrinsic ordering of the
    // grouped_t.  If you're processing its data into a parallel map, you're ok,
    // since the parallel map provides its own ordering (that you specify).  This
    // way, we know it's OK for the map ordering to use any reql_version (instead of
    // taking that as a parameter, which would be completely impracticable).
    typename std::map<counted_t<const datum_t>, T, counted_datum_less_t>::iterator
    begin(grouped::order_doesnt_matter_t) { return m.begin(); }
    typename std::map<counted_t<const datum_t>, T, counted_datum_less_t>::iterator
    end(grouped::order_doesnt_matter_t) { return m.end(); }

    std::pair<typename std::map<counted_t<const datum_t>, T, counted_datum_less_t>::iterator, bool>
    insert(std::pair<counted_t<const datum_t>, T> &&val) {
        return m.insert(std::move(val));
    }
    void
    erase(typename std::map<counted_t<const datum_t>, T, counted_datum_less_t>::iterator pos) {
        m.erase(pos);
    }

    size_t size() { return m.size(); }
    void clear() { return m.clear(); }
    T &operator[](const counted_t<const datum_t> &k) { return m[k]; }

    void swap(grouped_t<T> &other) { m.swap(other.m); } // NOLINT
    std::map<counted_t<const datum_t>, T, counted_datum_less_t> *
    get_underlying_map(grouped::order_doesnt_matter_t) {
        return &m;
    }
private:
    std::map<counted_t<const datum_t>, T, counted_datum_less_t> m;
};

RDB_SERIALIZE_TEMPLATED_OUTSIDE(grouped_t);

namespace grouped_details {

template <class T>
class grouped_pair_compare_t {
public:
    explicit grouped_pair_compare_t(reql_version_t _reql_version)
        : reql_version(_reql_version) { }

    bool operator()(const std::pair<counted_t<const datum_t>, T> &a,
                    const std::pair<counted_t<const datum_t>, T> &b) const {
        // We know the keys are different, this is only used in
        // iterate_ordered_by_version.
        return a.first->compare_lt(reql_version, *b.first);
    }

private:
    reql_version_t reql_version;
};

}  // namespace grouped_details

// For some people that iterate a grouped_t, order matters.  If the grouped_t is
// sorted by a different ordering than the one which they're expecting (because of
// reql version compatibility), we have to copy the elements to a vector and sort
// them before iterating them.
template <class T, class Callable>
void iterate_ordered_by_version(reql_version_t reql_version,
                                grouped_t<T> &grouped,
                                Callable &&callable) {
    std::map<counted_t<const datum_t>, T, counted_datum_less_t> *m
        = grouped.get_underlying_map(grouped::order_doesnt_matter_t());
    if (m->key_comp().reql_version() == reql_version) {
        for (std::pair<const counted_t<const datum_t>, T> &pair : *m) {
            callable(pair.first, pair.second);
        }
    } else {
        // The map is sorted with the wrong reql_version.  Copy it into a vector and
        // sort it first.
        std::vector<std::pair<counted_t<const datum_t>, T> >
            vec(m->begin(), m->end());
        // The keys (pulled straight out of a std::map) are unique, so std::sort
        // works fine.
        std::sort(vec.begin(), vec.end(),
                  grouped_details::grouped_pair_compare_t<T>(reql_version));
        for (std::pair<counted_t<const datum_t>, T> &pair : vec) {  // NOLINT(runtime/references)
            callable(pair.first, pair.second);
        }
    }
}


// We need a separate class for this because inheriting from
// `slow_atomic_countable_t` deletes our copy constructor, but boost variants
// want us to have a copy constructor.
class grouped_data_t : public grouped_t<counted_t<const datum_t> >,
                       public slow_atomic_countable_t<grouped_data_t> { }; // NOLINT

typedef boost::variant<
    grouped_t<uint64_t>, // Count.
    grouped_t<double>, // Sum.
    grouped_t<std::pair<double, uint64_t> >, // Avg.
    grouped_t<counted_t<const ql::datum_t> >, // Reduce (may be NULL)
    grouped_t<optimizer_t>, // min, max
    grouped_t<stream_t>, // No terminal.,
    exc_t // Don't re-order (we don't want this to initialize to an error.)
    > result_t;

typedef boost::variant<map_wire_func_t,
                       group_wire_func_t,
                       filter_wire_func_t,
                       concatmap_wire_func_t,
                       distinct_wire_func_t
                       > transform_variant_t;

typedef boost::variant<count_wire_func_t,
                       sum_wire_func_t,
                       avg_wire_func_t,
                       min_wire_func_t,
                       max_wire_func_t,
                       reduce_wire_func_t
                       > terminal_variant_t;

class op_t {
public:
    op_t() { }
    virtual ~op_t() { }
    virtual void operator()(env_t *env,
                            groups_t *groups,
                            // sindex_val may be NULL
                            const counted_t<const datum_t> &sindex_val) = 0;
};

class accumulator_t {
public:
    accumulator_t();
    virtual ~accumulator_t();
    // May be overridden as an optimization (currently is for `count`).
    virtual bool uses_val() { return true; }
    virtual bool should_send_batch() = 0;
    virtual done_traversing_t operator()(env_t *env,
                                         groups_t *groups,
                                         const store_key_t &key,
                                         // sindex_val may be NULL
                                         const counted_t<const datum_t> &sindex_val) = 0;
    virtual void finish(result_t *out);
    virtual void unshard(env_t *env,
                         const store_key_t &last_key,
                         const std::vector<result_t *> &results) = 0;
protected:
    void mark_finished();
private:
    virtual void finish_impl(result_t *out) = 0;
    bool finished;
};

class eager_acc_t {
public:
    eager_acc_t() { }
    virtual ~eager_acc_t() { }
    virtual void operator()(env_t *env, groups_t *groups) = 0;
    virtual void add_res(env_t *env, result_t *res) = 0;
    virtual counted_t<val_t> finish_eager(
        protob_t<const Backtrace> bt, bool is_grouped,
        const ql::configured_limits_t &limits) = 0;
};

scoped_ptr_t<accumulator_t> make_append(const sorting_t &sorting, batcher_t *batcher);
//                                                        NULL if unsharding ^^^^^^^
scoped_ptr_t<accumulator_t> make_terminal(const terminal_variant_t &t);

scoped_ptr_t<eager_acc_t> make_to_array(reql_version_t reql_version);
scoped_ptr_t<eager_acc_t> make_eager_terminal(const terminal_variant_t &t);

scoped_ptr_t<op_t> make_op(const transform_variant_t &tv);

} // namespace ql

#endif  // RDB_PROTOCOL_SHARDS_HPP_
