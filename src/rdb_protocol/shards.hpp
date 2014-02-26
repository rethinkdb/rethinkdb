// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_SHARDS_HPP_
#define RDB_PROTOCOL_SHARDS_HPP_

#include <map>
#include <limits>
#include <utility>
#include <vector>

#include "btree/concurrent_traversal.hpp"
#include "btree/keys.hpp"
#include "containers/archive/stl_types.hpp"
#include "rdb_protocol/batching.hpp"
#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/profile.hpp"
#include "rdb_protocol/rdb_protocol_json.hpp"
#include "rdb_protocol/wire_func.hpp"

namespace ql {

enum class sorting_t {
    UNORDERED,
    ASCENDING,
    DESCENDING
};
// UNORDERED sortings aren't reversed
bool reversed(sorting_t sorting);

// This stuff previously resided in the protocol, but has been broken out since
// we want to use this logic in multiple places.
typedef std::vector<counted_t<const ql::datum_t> > datums_t;
typedef std::map<counted_t<const ql::datum_t>, datums_t> groups_t;

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
typedef std::vector<rget_item_t> stream_t;

// We write all of these serializations and deserializations explicitly because:
// * It stops people from inadvertently using a new `grouped_t<T>` without thinking.
// * Some grouped elements need specialized serialization.
static inline void serialize_grouped(
    write_message_t *msg, const counted_t<const datum_t> &d) {
    *msg << d.has();
    if (d.has()) *msg << d;
}
static inline void serialize_grouped(write_message_t *msg, uint64_t sz) {
    serialize_varint_uint64(msg, sz);
}
static inline void serialize_grouped(write_message_t *msg, double d) {
    *msg << d;
}
static inline void serialize_grouped(write_message_t *msg,
                                     const std::pair<double, uint64_t> &p) {
    *msg << p.first;
    serialize_varint_uint64(msg, p.second);
}
static inline void serialize_grouped(write_message_t *msg, const stream_t &sz) {
    *msg << sz;
}
static inline void serialize_grouped(write_message_t *msg, const datums_t &ds) {
    *msg << ds;
}

static inline archive_result_t deserialize_grouped(
    read_stream_t *s, counted_t<const datum_t> *d) {
    bool has;
    archive_result_t res = deserialize(s, &has);
    if (bad(res)) { return res; }
    if (has) {
        return deserialize(s, d);
    } else {
        d->reset();
        return archive_result_t::SUCCESS;
    }
}
static inline archive_result_t deserialize_grouped(read_stream_t *s, uint64_t *sz) {
    return deserialize_varint_uint64(s, sz);
}
static inline archive_result_t deserialize_grouped(read_stream_t *s, double *d) {
    return deserialize(s, d);
}
static inline archive_result_t deserialize_grouped(read_stream_t *s,
                                                   std::pair<double, uint64_t> *p) {
    archive_result_t res = deserialize(s, &p->first);
    if (bad(res)) { return res; }
    return deserialize_varint_uint64(s, &p->second);
}
static inline archive_result_t deserialize_grouped(read_stream_t *s, stream_t *sz) {
    return deserialize(s, sz);
}
static inline archive_result_t deserialize_grouped(read_stream_t *s, datums_t *ds) {
    return deserialize(s, ds);
}

// This is basically a templated typedef with special serialization.
template<class T>
class grouped_t {
public:
    virtual ~grouped_t() { } // See grouped_data_t below.
    void rdb_serialize(write_message_t &msg) const { // NOLINT
        serialize_varint_uint64(&msg, m.size());
        for (auto it = m.begin(); it != m.end(); ++it) {
            serialize_grouped(&msg, it->first);
            serialize_grouped(&msg, it->second);
        }
    }
    archive_result_t rdb_deserialize(read_stream_t *s) {
        uint64_t sz = m.size();
        guarantee(sz == 0);
        archive_result_t res = deserialize_varint_uint64(s, &sz);
        if (bad(res)) { return res; }
        if (sz > std::numeric_limits<size_t>::max()) {
            return archive_result_t::RANGE_ERROR;
        }
        auto pos = m.begin();
        for (uint64_t i = 0; i < sz; ++i) {
            std::pair<counted_t<const datum_t>, T> el;
            res = deserialize_grouped(s, &el.first);
            if (bad(res)) { return res; }
            res = deserialize_grouped(s, &el.second);
            if (bad(res)) { return res; }
            pos = m.insert(pos, std::move(el));
        }
        return archive_result_t::SUCCESS;
    }


    // We pass these through manually rather than using inheritance because
    // `std::map` lacks a virtual destructor.
    typename std::map<counted_t<const datum_t>, T>::iterator
    begin() { return m.begin(); }
    typename std::map<counted_t<const datum_t>, T>::iterator
    end() { return m.end(); }

    std::pair<typename std::map<counted_t<const datum_t>, T>::iterator, bool>
    insert(std::pair<counted_t<const datum_t>, T> &&val) {
        return m.insert(std::move(val));
    }

    size_t size() { return m.size(); }
    void clear() { return m.clear(); }
    T &operator[](const counted_t<const datum_t> &k) { return m[k]; }

    void swap(grouped_t<T> &other) { m.swap(other.m); } // NOLINT
    std::map<counted_t<const datum_t>, T> *get_underlying_map() { return &m; }
private:
    std::map<counted_t<const datum_t>, T> m;
};

// We need a separate class for this because inheriting from
// `slow_atomic_countable_t` deletes our copy constructor, but boost variants
// want us to have a copy constructor.
class grouped_data_t : public grouped_t<counted_t<const datum_t> >,
                       public slow_atomic_countable_t<grouped_data_t> { }; // NOLINT

typedef boost::variant<
    grouped_t<uint64_t>, // Count.
    grouped_t<double>, // Sum.
    grouped_t<std::pair<double, uint64_t> >, // Avg.
    grouped_t<counted_t<const ql::datum_t> >, // Reduce (may be NULL), min, max.
    grouped_t<stream_t>, // No terminal.,
    exc_t // Don't re-order (we don't want this to initialize to an error.)
    > result_t;

typedef boost::variant<map_wire_func_t,
                       group_wire_func_t,
                       filter_wire_func_t,
                       concatmap_wire_func_t
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
    virtual void operator()(groups_t *groups,
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
    virtual done_traversing_t operator()(groups_t *groups,
                              store_key_t &&key,
                              // sindex_val may be NULL
                              counted_t<const datum_t> &&sindex_val) = 0;
    virtual void finish(result_t *out);
    virtual void unshard(
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
    virtual void operator()(groups_t *groups) = 0;
    virtual void add_res(result_t *res) = 0;
    virtual counted_t<val_t> finish_eager(
        protob_t<const Backtrace> bt, bool is_grouped) = 0;
};

// Make sure to put these right into a scoped pointer.
accumulator_t *make_append(const sorting_t &sorting, batcher_t *batcher);
//                                           NULL if unsharding ^^^^^^^
accumulator_t *make_terminal(
    ql::env_t *env, const terminal_variant_t &t);

eager_acc_t *make_to_array();
eager_acc_t *make_eager_terminal(
    ql::env_t *env, const terminal_variant_t &t);

op_t *make_op(env_t *env, const transform_variant_t &tv);

} // namespace ql

#endif  // RDB_PROTOCOL_SHARDS_HPP_
