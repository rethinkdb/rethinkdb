// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_SHARDS_HPP_
#define RDB_PROTOCOL_SHARDS_HPP_

#include "utils.hpp"

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
typedef std::vector<counted_t<const ql::datum_t> > lst_t;
typedef std::map<counted_t<const ql::datum_t>, lst_t> groups_t;
template<class T>
class grouped : public std::map<counted_t<const ql::datum_t>, T> {
public:
    // This is needed for serialization to overload correctly (don't ask me why).
    void rdb_serialize(write_message_t &msg) const { // NOLINT
        msg << *this;
    }
    archive_result_t rdb_deserialize(read_stream_t *s) {
        return deserialize(s, this);
    }
};

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

typedef boost::variant<
    ql::grouped<size_t>, // Count.
    ql::grouped<counted_t<const ql::datum_t> >, // Reduce (may be NULL).
    ql::grouped<stream_t>, // No terminal.
    ql::exc_t // Don't re-order (we don't want this to initialize to an error.)
    > result_t;

typedef boost::variant<ql::map_wire_func_t,
                       ql::filter_wire_func_t,
                       ql::concatmap_wire_func_t> transform_variant_t;

typedef boost::variant<ql::count_wire_func_t,
                       ql::reduce_wire_func_t> terminal_variant_t;

class op_t {
public:
    op_t() { }
    virtual ~op_t() { }
    virtual void operator()(groups_t *groups) = 0;
};

// RSI: does append keep track of the last considered key?
class accumulator_t {
public:
    accumulator_t() : finished(false) { }
    virtual ~accumulator_t() { guarantee(finished); }
    // May be overridden as an optimization (currently is for `count`).
    virtual bool uses_val() { return true; }
    virtual bool should_send_batch() = 0;
    virtual done_t operator()(groups_t *groups,
                              store_key_t &&key,
                              // sindex_val may be NULL
                              counted_t<const datum_t> &&sindex_val) = 0;
    virtual void finish(result_t *out);
    virtual counted_t<val_t> unpack(result_t &&res, protob_t<const Backtrace> bt) = 0;
    virtual void unshard(
        const store_key_t &last_key,
        const std::vector<result_t *> &results) = 0;
private:
    virtual void finish_impl(result_t *out) = 0;
    bool finished;
};

// Make sure to put these right into a scoped pointer.
accumulator_t *make_append(const sorting_t &sorting, batcher_t *batcher);
//                                    NULL if unsharding ^^^^^^^
accumulator_t *make_terminal(
    ql::env_t *env, const terminal_variant_t &t);
op_t *make_op(env_t *env, const transform_variant_t &tv);

} // namespace ql

#endif  // RDB_PROTOCOL_SHARDS_HPP_
