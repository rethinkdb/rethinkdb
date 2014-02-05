// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

#include "utils.hpp"

#include "rdb_protocol/protocol.hpp"
#include "btree/concurrent_traversal.hpp"

namespace ql {

typedef std::vector<counted_t<const ql::datum_t> > lst_t;
typedef std::map<counted_t<const ql::datum_t>, lst_t> groups_t;

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
    virtual void finish(rdb_protocol_t::rget_read_response_t::result_t *out);
    virtual void unshard(
        const store_key_t &last_key,
        const std::vector<rget_read_response_t::res_groups_t *> &rgs) = 0;
private:
    virtual void finish_impl(rget_read_response_t::result_t *out) = 0;
    bool finished;
};

// Make sure to put these right into a scoped pointer.
accumulator_t *make_append(sorting_t sorting, batcher_t *batcher);
//                                    NULL if unsharding ^^^^^^^
accumulator_t *make_terminal(
    ql::env_t *env, const rdb_protocol_details::terminal_variant_t &t);
op_t *make_op(env_t *env, const rdb_protocol_details::transform_variant_t &tv);

} // namespace ql
#endif  // RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
