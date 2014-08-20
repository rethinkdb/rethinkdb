// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "rdb_protocol/batching.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

static const int64_t DEFAULT_MIN_ELS = 8;
static const int64_t DEFAULT_FIRST_SCALEDOWN = 4;
static const int64_t DEFAULT_MAX_SIZE = MEGABYTE;
// The maximum duration of a batch in microseconds.
static const int64_t DEFAULT_MAX_DURATION = 500 * 1000;
// These numbers are sort of arbitrary, but they seem to work. See `scale_down()`
// for an explanation.
static const int64_t DIVISOR_SCALING_FACTOR = 8;
static const int64_t SCALE_CONSTANT = 8;

RDB_IMPL_SERIALIZABLE_7(batchspec_t,
                        batch_type,
                        min_els,
                        max_els,
                        max_size,
                        first_scaledown_factor,
                        max_dur,
                        start_time);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(batchspec_t);

batchspec_t::batchspec_t(
    batch_type_t _batch_type,
    int64_t _min_els,
    int64_t _max_els,
    int64_t _max_size,
    int64_t _first_scaledown,
    int64_t _max_dur,
    microtime_t _start_time)
    : batch_type(_batch_type),
      min_els(_min_els),
      max_els(_max_els),
      max_size(_max_size),
      first_scaledown_factor(_first_scaledown),
      max_dur(_max_dur),
      start_time(_start_time) {
    r_sanity_check(first_scaledown_factor >= 1);
    r_sanity_check(max_els >= 1);
    r_sanity_check(min_els >= 1);
    r_sanity_check(max_els >= min_els);
    r_sanity_check(max_size >= 1);
    r_sanity_check(max_dur >= 0);
}

batchspec_t batchspec_t::user(batch_type_t batch_type,
                              const counted_t<const datum_t> &conf) {
    counted_t<const datum_t> max_els_d, min_els_d, max_size_d, max_dur_d;
    counted_t<const datum_t> first_scaledown_d;
    if (conf.has()) {
        min_els_d = conf->get("min_els", NOTHROW);
        max_els_d = conf->get("max_els", NOTHROW);
        max_size_d = conf->get("max_size", NOTHROW);
        first_scaledown_d = conf->get("first_scaledown", NOTHROW);
        max_dur_d = conf->get("max_dur", NOTHROW);
    }
    int64_t max_els = max_els_d.has()
                      ? max_els_d->as_int()
                      : std::numeric_limits<decltype(batchspec_t().max_els)>::max();
    int64_t min_els = min_els_d.has()
                      ? min_els_d->as_int()
                      : std::min<int64_t>(max_els, DEFAULT_MIN_ELS);
    int64_t max_size = max_size_d.has() ? max_size_d->as_int() : DEFAULT_MAX_SIZE;
    int64_t first_sd = first_scaledown_d.has()
                       ? first_scaledown_d->as_int()
                       : DEFAULT_FIRST_SCALEDOWN;
    int64_t max_dur = max_dur_d.has() ? max_dur_d->as_int() : DEFAULT_MAX_DURATION;
    // Protect the user in case they're a dork.  Normally we would do rfail and
    // trigger exceptions, but due to NOTHROWs above this may not be safe.
    min_els = std::min<int64_t>(min_els, max_els);
    return batchspec_t(batch_type,
                       min_els,
                       max_els,
                       max_size,
                       first_sd,
                       max_dur,
                       current_microtime());
}

batchspec_t batchspec_t::all() {
    return batchspec_t(batch_type_t::TERMINAL,
                       DEFAULT_MIN_ELS,
                       std::numeric_limits<decltype(batchspec_t().max_els)>::max(),
                       std::numeric_limits<decltype(batchspec_t().max_size)>::max(),
                       1,
                       0,
                       current_microtime());
}

batchspec_t batchspec_t::user(batch_type_t batch_type, env_t *env) {
    counted_t<val_t> v = env->get_optarg(env, "batch_conf");
    return user(batch_type, v.has() ? v->as_datum() : counted_t<const datum_t>());
}

batchspec_t batchspec_t::with_new_batch_type(batch_type_t new_batch_type) const {
    return batchspec_t(new_batch_type, min_els, max_els, max_size,
                       first_scaledown_factor, max_dur, start_time);
}

batchspec_t batchspec_t::with_at_most(uint64_t _max_els) const {
    // Special case: if _max_els is 1, we want min_els to also be 1 for maximum
    // efficiency.
    // If _max_els on the other hand is > 1 but still small, having
    // min_els = _max_els could result in multiple round-trips, since our unsharding
    // code would throw out a lot of documents. So in that case we just keep min_els
    // unchanged (and in fact elevate _max_els to min_els if _max_els < min_els
    // otherwise).
    int64_t new_min_els = _max_els == 1 ? 1 : min_els;
    int64_t new_max_els = std::max<int64_t>(new_min_els,
            std::min(uint64_t(std::numeric_limits<int64_t>::max()), _max_els));
    return batchspec_t(
        batch_type,
        new_min_els,
        new_max_els,
        max_size,
        first_scaledown_factor,
        max_dur,
        start_time);
}

batchspec_t batchspec_t::scale_down(int64_t divisor) const {
    // We divide by e.g. 7/8th of the divisor (assuming DIVISOR_SCALING_FACTOR == 8)
    // and add SCALE_CONSTANT to reduce the chances of needing a second round-trip
    // (we add a constant because unequal division is more likely with very small
    // sizes).  Law of large numbers says that the chances of needing a second
    // round-trip for large, non-pathological datasets are extremely low.
    int64_t new_max_els =
        max_els == std::numeric_limits<decltype(batchspec_t().max_els)>::max()
            ? max_els
            : std::min(max_els, (max_els * DIVISOR_SCALING_FACTOR
                                 / ((DIVISOR_SCALING_FACTOR - 1) * divisor))
                                + SCALE_CONSTANT);
    int64_t new_max_size =
        max_size == std::numeric_limits<decltype(batchspec_t().max_size)>::max()
            ? max_size
            : std::min(max_size, (max_size * DIVISOR_SCALING_FACTOR
                                  / ((DIVISOR_SCALING_FACTOR - 1) * divisor))
                                 + SCALE_CONSTANT);
    // to avoid problems when the batches get really tiny, we clamp new_max_els to be at least min_els.
    new_max_els = std::max(min_els, new_max_els);

    return batchspec_t(batch_type, min_els, new_max_els, new_max_size,
                       first_scaledown_factor, max_dur, start_time);
}

batcher_t batchspec_t::to_batcher() const {
    int64_t real_min_els =
        batch_type != batch_type_t::NORMAL_FIRST
            ? min_els
            : std::max<int64_t>(1, min_els / first_scaledown_factor);
    int64_t real_max_els =
        max_els == std::numeric_limits<decltype(batchspec_t().max_els)>::max()
        || batch_type != batch_type_t::NORMAL_FIRST
            ? max_els
            : std::max<int64_t>(1, max_els / first_scaledown_factor);
    int64_t real_max_size =
        max_size == std::numeric_limits<decltype(batchspec_t().max_size)>::max()
        || batch_type != batch_type_t::NORMAL_FIRST
            ? max_size
            : std::max<int64_t>(1, max_size / first_scaledown_factor);
    microtime_t cur_time = current_microtime();
    microtime_t end_time;
    if (batch_type == batch_type_t::NORMAL) {
        end_time = std::max(cur_time + (cur_time - start_time), start_time + max_dur);
    } else if (batch_type == batch_type_t::NORMAL_FIRST) {
        end_time = std::max(start_time + (max_dur / first_scaledown_factor),
                            cur_time + (max_dur / (first_scaledown_factor * 2)));
    } else {
        end_time = std::numeric_limits<decltype(end_time)>::max();
    }
    return batcher_t(batch_type, real_min_els, real_max_els, real_max_size, end_time);
}

bool batcher_t::should_send_batch() const {
    // We ignore size_left as long as we have not got at least `min_wanted_els`
    // documents.
    return els_left <= 0
        || (size_left <= 0 && min_els_left <= 0)
        || (current_microtime() >= end_time && seen_one_el);
}

batcher_t::batcher_t(
    batch_type_t _batch_type,
    int64_t min_els,
    int64_t max_els,
    int64_t max_size,
    microtime_t _end_time)
    : batch_type(_batch_type),
      seen_one_el(false),
      min_els_left(min_els),
      els_left(max_els),
      size_left(max_size),
      end_time(_end_time) { }

} // namespace ql
