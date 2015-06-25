// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "btree/backfill_debug.hpp"

#include <deque>

#include "arch/spinlock.hpp"

#ifdef ENABLE_BACKFILL_DEBUG

class backfill_debug_msg_t {
public:
    key_range_t keys;
    std::string msg;
};

std::deque<backfill_debug_msg_t> backfill_debug_msgs;
spinlock_t backfill_debug_lock;

void backfill_debug_key(const store_key_t &key, const std::string &msg) {
    backfill_debug_range(key_range_t(key.btree_key()), msg);
}

void backfill_debug_range(const key_range_t &range, const std::string &msg) {
    spinlock_acq_t acq(&backfill_debug_lock);
    backfill_debug_msgs.push_back(backfill_debug_msg_t { range, msg });
}

void backfill_debug_all(const std::string &msg) {
    backfill_debug_range(key_range_t::universe(), msg);
}

void backfill_debug_dump_log(const store_key_t &key) {
    spinlock_acq_t acq(&backfill_debug_lock);
    fprintf(stderr, "-------- BACKFILL DEBUG LOG (%s) --------\n",
        key_to_debug_str(key).c_str());
    for (const backfill_debug_msg_t &msg : backfill_debug_msgs) {
        if (!msg.keys.contains_key(key)) {
            continue;
        }
        if (msg.keys == key_range_t::universe()) {
            fprintf(stderr, "<general>: %s\n", msg.msg.c_str());
        } else if (msg.keys == key_range_t(key.btree_key())) {
            fprintf(stderr, "%s: %s\n", key_to_debug_str(key).c_str(), msg.msg.c_str());
        } else {
            fprintf(stderr, "%s: %s\n",
                key_range_to_string(msg.keys).c_str(), msg.msg.c_str());
        }
    }
}

#endif /* ENABLE_BACKFILL_DEBUG */

