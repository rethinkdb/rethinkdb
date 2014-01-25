// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
#define RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_

#include <list>
#include <vector>

#include "http/json.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/lazy_json.hpp"
#include "rdb_protocol/protocol.hpp"

namespace ql {
namespace shards {

// RSI: check no copying
typedef std::vector<counted_t<const datum_t> > lst_t;
typedef std::map<counted_t<const datum_t>, lst_t> groups_t;
typedef boost::variant<lst_t, groups_t> lst_or_groups_t;

void transform(env_t *env, const transform_variant_t &t, lst_or_groups_t *target);
void terminate(env_t *env, const terminal_variant_t &t,
               lst_or_groups_t *data, rget_read_response_t::result_t *target);

void append(const store_key_t &key,
            counted_t<const datum_t> sindex_val, // may be NULL
            sorting_t sorting,
            batcher_t *batcher,
            lst_or_groups_t *data,
            rget_read_response_t::result_t *target);

rget_read_response_t::res_t terminal_start(const terminal_variant_t &t);
rget_read_response_t::res_t stream_start();

exc_t terminal_exc(const datum_exc_t &e, const terminal_variant_t &t);
exc_t transform_exc(const datum_exc_t &e, const transform_variant_t &t);

bool terminal_uses_value(const terminal_variant_t &t);

} // namespace shards
} // namespace ql

#endif  // RDB_PROTOCOL_TRANSFORM_VISITORS_HPP_
