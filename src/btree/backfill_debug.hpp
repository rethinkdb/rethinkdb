// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef BTREE_BACKFILL_DEBUG_HPP_
#define BTREE_BACKFILL_DEBUG_HPP_

#include <string>

#include "btree/keys.hpp"

/* If the `ENABLE_BACKFILL_DEBUG` symbol is defined, then backfilling-related events
will be recorded in a log and indexed by key. This can be useful for diagnosing bugs in
the backfilling logic. Obviously, this should be disabled when not in use. */
#define ENABLE_BACKFILL_DEBUG

#ifdef ENABLE_BACKFILL_DEBUG
void backfill_debug_key(const store_key_t &key, const std::string &msg);
void backfill_debug_range(const key_range_t &range, const std::string &msg);
void backfill_debug_all(const std::string &msg);
void backfill_debug_dump_log(const store_key_t &key);
#else
inline void backfill_debug_key(const store_key_t &, const std::string &) { }
inline void backfill_debug_range(const key_range_t &, const std::string &) { }
inline void backfill_debug_all(const std::string &) { }
inline void backfill_debug_dump_log(const store_key_t &) { }
#endif /* ENABLE_BACKFILL_DEBUG */

#endif /* BTREE_BACKFILL_DEBUG_HPP_ */

