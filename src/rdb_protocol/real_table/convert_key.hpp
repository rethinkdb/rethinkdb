// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_REAL_TABLE_CONVERT_KEY_HPP_
#define RDB_PROTOCOL_REAL_TABLE_CONVERT_KEY_HPP_

#include "btree/keys.hpp"
#include "rdb_protocol/datum.hpp"

namespace rdb_protocol {

const size_t MAX_PRIMARY_KEY_SIZE = 128;

// Constructs a region which will query an sindex for matches to a specific key
key_range_t sindex_key_range(const store_key_t &start,
                             const store_key_t &end);
}

key_range_t datum_range_to_primary_keyrange(const ql::datum_range_t &range);
key_range_t datum_range_to_sindex_keyrange(const ql::datum_range_t &range);

#endif /* RDB_PROTOCOL_REAL_TABLE_CONVERT_KEY_HPP_ */

