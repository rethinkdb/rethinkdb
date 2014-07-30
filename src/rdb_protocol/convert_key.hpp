// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_CONVERT_KEY_HPP_
#define RDB_PROTOCOL_CONVERT_KEY_HPP_

#include "btree/keys.hpp"
#include "rdb_protocol/datum.hpp"

class datum_range_t;

key_range_t datum_range_to_primary_keyrange(const datum_range_t &range);
key_range_t datum_range_to_sindex_keyrange(const datum_range_t &range);

#endif /* RDB_PROTOCOL_CONVERT_KEY_HPP_ */

