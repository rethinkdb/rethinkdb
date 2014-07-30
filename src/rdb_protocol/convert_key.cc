// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/convert_key.hpp"

#include "rdb_protocol/protocol.hpp"

key_range_t datum_range_to_primary_keyrange(const datum_range_t &range) {
    return key_range_t(
        range.left_bound_type,
        range.left_bound.has()
            ? store_key_t(range.left_bound->print_primary())
            : store_key_t::min(),
        range.right_bound_type,
        range.right_bound.has()
            ? store_key_t(range.right_bound->print_primary())
            : store_key_t::max());
}

key_range_t datum_range_to_sindex_keyrange(const datum_range_t &range) {
    return rdb_protocol::sindex_key_range(
        range.left_bound.has()
            ? store_key_t(range.left_bound->truncated_secondary())
            : store_key_t::min(),
        range.right_bound.has()
            ? store_key_t(range.right_bound->truncated_secondary())
            : store_key_t::max());
}

