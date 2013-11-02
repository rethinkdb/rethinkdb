// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_BATCHING_HPP_
#define RDB_PROTOCOL_BATCHING_HPP_

#include "utils.hpp"
#include "rpc/serialize_macros.hpp"

template<class T>
class counted_t;

namespace ql {

class datum_t;
class env_t;

enum batch_type_t {
    NORMAL = 0, // A normal batch.
    // A batch fetched for a terminal or terminal-like term, e.g. a big batched
    // insert.  Ignores latency caps because the latency the user encounters is
    // determined by bandwidth instead.
    TERMINAL = 1,
    // If we're ordering by an sindex, get a batch with a constant value for
    // that sindex.  We sometimes need a batch with that invariant for sorting.
    // (This replaces that SORTING_HINT_NEXT stuff.)
    SINDEX_CONSTANT = 2
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(batch_type_t, int8_t, NORMAL, SINDEX_CONSTANT);

class batcher_t {
public:
    static batcher_t user_batcher(batch_type_t batch_type,
                                  const counted_t<const datum_t> &conf);
    static batcher_t user_batcher(batch_type_t batch_type, env_t *env);
    static batcher_t empty_batcher_for_serialization() { return batcher_t(); }

    template<class T>
    bool note_el(const T &t) {
        seen_one_el = true;
        els_left -= 1;
        size_left -= serialized_size(t);
        return should_send_batch();
    }
    bool should_send_batch() const;
    batch_type_t get_batch_type() const { return batch_type; }
    RDB_MAKE_ME_SERIALIZABLE_5(batch_type, seen_one_el, els_left, size_left, end_time);
private:
    // I made this private and accessible through a static function because it
    // was being accidentally default-initialized.
    batcher_t() { } // USE ONLY FOR SERIALIZATION

    batcher_t(batch_type_t batch_type, int64_t els, int64_t size, microtime_t end_time);

    batch_type_t batch_type;
    bool seen_one_el;
    int64_t els_left, size_left;
    microtime_t end_time;
};

template<class T>
static inline bool past_array_limit(const T &t) {
    return t.size() > 100000;
}

template<class T>
std::string array_size_error(UNUSED const T &t) {
    return strprintf("Array size limit of 100000 exceeded.");
}

} // namespace ql

#endif // RDB_PROTOCOL_BATCHING_HPP_
