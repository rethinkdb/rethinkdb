// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_BATCHING_HPP_
#define RDB_PROTOCOL_BATCHING_HPP_

#include <utility>

#include "containers/archive/archive.hpp"
#include "containers/archive/versioned.hpp"
#include "rdb_protocol/datum.hpp"
#include "rpc/serialize_macros.hpp"
#include "time.hpp"

template<class T>
class counted_t;

namespace ql {

class datum_t;
class env_t;

enum class batch_type_t {
    // A normal batch.
    NORMAL = 0,
    // The first batch in a series of normal batches. The size limit is reduced
    // to help minimizing the latency until a user receives their first response.
    NORMAL_FIRST = 1,
    // A batch fetched for a terminal or terminal-like term, e.g. a big batched
    // insert.  Ignores latency caps because the latency the user encounters is
    // determined by bandwidth instead.
    TERMINAL = 2,
    // If we're ordering by an sindex, get a batch with a constant value for
    // that sindex.  We sometimes need a batch with that invariant for sorting.
    // (This replaces that SORTING_HINT_NEXT stuff.)
    SINDEX_CONSTANT = 3
};
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
    batch_type_t, int8_t, batch_type_t::NORMAL, batch_type_t::SINDEX_CONSTANT);

class batcher_t {
public:
    bool note_el(const datum_t &t) {
        seen_one_el = true;
        els_left -= 1;
        min_els_left -= 1;
        size_left -= serialized_size<cluster_version_t::CLUSTER>(t);
        return should_send_batch();
    }
    bool should_send_batch() const;
    batcher_t(batcher_t &&other) :
        batch_type(std::move(other.batch_type)),
        seen_one_el(std::move(other.seen_one_el)),
        min_els_left(std::move(other.min_els_left)),
        els_left(std::move(other.els_left)),
        size_left(std::move(other.size_left)),
        end_time(std::move(other.end_time)) { }
    microtime_t microtime_left() {
        microtime_t cur_time = current_microtime();
        return end_time > cur_time ? end_time - cur_time : 0;
    }
    batch_type_t get_batch_type() { return batch_type; }
private:
    DISABLE_COPYING(batcher_t);
    friend class batchspec_t;
    batcher_t(batch_type_t batch_type, int64_t min_els, int64_t max_els,
              int64_t max_size, microtime_t end_time);

    const batch_type_t batch_type;
    bool seen_one_el;
    int64_t min_els_left, els_left, size_left;
    const microtime_t end_time;
};

class batchspec_t {
public:
    static batchspec_t user(batch_type_t batch_type, env_t *env);
    static batchspec_t all(); // Gimme everything.
    static batchspec_t empty() { return batchspec_t(); }
    static batchspec_t default_for(batch_type_t batch_type);
    batch_type_t get_batch_type() const { return batch_type; }
    batchspec_t with_new_batch_type(batch_type_t new_batch_type) const;
    batchspec_t with_max_dur(int64_t new_max_dur) const;
    batchspec_t with_at_most(uint64_t max_els) const;
    batchspec_t scale_down(int64_t divisor) const;
    batcher_t to_batcher() const;

private:
    template<cluster_version_t W>
    friend void serialize(write_message_t *, const batchspec_t &);
    template<cluster_version_t W>
    friend archive_result_t deserialize(read_stream_t *, batchspec_t *);
    // I made this private and accessible through a static function because it
    // was being accidentally default-initialized.
    batchspec_t() { } // USE ONLY FOR SERIALIZATION
    batchspec_t(batch_type_t batch_type, int64_t min_els, int64_t max_els,
                int64_t max_size, int64_t first_scaledown,
                int64_t max_dur, microtime_t start_time);

    batch_type_t batch_type;
    int64_t min_els, max_els, max_size, first_scaledown_factor, max_dur;
    microtime_t start_time;
};
RDB_DECLARE_SERIALIZABLE(batchspec_t);

} // namespace ql

#endif // RDB_PROTOCOL_BATCHING_HPP_
