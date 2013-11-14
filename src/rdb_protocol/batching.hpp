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

enum class batch_type_t {
    // A normal batch.
    NORMAL = 0,
    // A batch fetched for a terminal or terminal-like term, e.g. a big batched
    // insert.  Ignores latency caps because the latency the user encounters is
    // determined by bandwidth instead.
    TERMINAL = 1,
    // If we're ordering by an sindex, get a batch with a constant value for
    // that sindex.  We sometimes need a batch with that invariant for sorting.
    // (This replaces that SORTING_HINT_NEXT stuff.)
    SINDEX_CONSTANT = 2
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    batch_type_t, int8_t, batch_type_t::NORMAL, batch_type_t::SINDEX_CONSTANT);

class batcher_t {
public:
    template<class T>
    bool note_el(const T &t) {
        seen_one_el = true;
        els_left -= 1;
        size_left -= serialized_size(t);
        return should_send_batch();
    }
    bool should_send_batch() const;
    batcher_t(batcher_t &&) = default;
private:
    DISABLE_COPYING(batcher_t);
    friend class batchspec_t;
    batcher_t(batch_type_t batch_type, int64_t els, int64_t size, microtime_t end_time);

    const batch_type_t batch_type;
    bool seen_one_el;
    int64_t els_left, size_left;
    const microtime_t end_time;
};

class batchspec_t {
public:
    static batchspec_t user(batch_type_t batch_type,
                            const counted_t<const datum_t> &conf);
    static batchspec_t user(batch_type_t batch_type, env_t *env);
    static batchspec_t empty() { return batchspec_t(); }
    batch_type_t get_batch_type() const { return batch_type; }
    batchspec_t with_new_batch_type(batch_type_t new_batch_type) const;
    batcher_t to_batcher() const;
    RDB_MAKE_ME_SERIALIZABLE_4(batch_type, els_left, size_left, end_time);
private:
    // I made this private and accessible through a static function because it
    // was being accidentally default-initialized.
    batchspec_t() { } // USE ONLY FOR SERIALIZATION
    batchspec_t(batch_type_t batch_type, int64_t els, int64_t size, microtime_t end);

    batch_type_t batch_type;
    int64_t els_left, size_left;
    microtime_t end_time;
};

// TODO: make user-tunable.
size_t array_size_limit();

} // namespace ql

#endif // RDB_PROTOCOL_BATCHING_HPP_
