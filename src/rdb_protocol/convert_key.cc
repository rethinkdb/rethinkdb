// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "rdb_protocol/convert_key.hpp"

#include "rdb_protocol/protocol.hpp"

namespace rdb_protocol {

key_range_t sindex_key_range(const store_key_t &start,
                             const store_key_t &end) {
    store_key_t end_key;
    std::string end_key_str(key_to_unescaped_str(end));

    // Need to make the next largest store_key_t without making the key longer
    while (end_key_str.length() > 0 &&
           end_key_str[end_key_str.length() - 1] == static_cast<char>(255)) {
        end_key_str.erase(end_key_str.length() - 1);
    }

    if (end_key_str.length() == 0) {
        end_key = store_key_t::max();
    } else {
        ++end_key_str[end_key_str.length() - 1];
        end_key = store_key_t(end_key_str);
    }
    return key_range_t(key_range_t::closed, start, key_range_t::open, end_key);
}

}   // namespace rdb_protocol

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

