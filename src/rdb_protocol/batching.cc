// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "rdb_protocol/batching.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"

#define DEFAULT_MIN_WANTED_ELS      8
#define DEFAULT_FIRST_SCALEDOWN     4
#define DEFAULT_MAX_SIZE            MEGABYTE
#define DEFAULT_MAX_TIME            (500 * 1000)


namespace ql {

batchspec_t::batchspec_t(
    batch_type_t _batch_type,
    int64_t els,
    int64_t min_wanted_els,
    int64_t size,
    int32_t first_scaledown,
    microtime_t _end_time)
    : batch_type(_batch_type),
      els_left(els),
      min_wanted_els_left(min_wanted_els),
      size_left(size),
      first_scaledown_factor(first_scaledown),
      end_time(_batch_type == batch_type_t::NORMAL
               || _batch_type == batch_type_t::NORMAL_FIRST
                   ? _end_time
                   : std::numeric_limits<decltype(batchspec_t().end_time)>::max()) {
    r_sanity_check(first_scaledown_factor >= 1);
    r_sanity_check(els_left >= 1);
    r_sanity_check(min_wanted_els_left >= 0);
    r_sanity_check(min_wanted_els_left <= els_left);
}

batchspec_t batchspec_t::user(batch_type_t batch_type,
                              const counted_t<const datum_t> &conf) {
    counted_t<const datum_t> max_els_d, min_els_d, max_size_d, max_dur_d;
    counted_t<const datum_t> first_scaledown_d;
    if (conf.has()) {
        max_els_d = conf->get("max_els", NOTHROW);
        min_els_d = conf->get("min_els", NOTHROW);
        max_size_d = conf->get("max_size", NOTHROW);
        first_scaledown_d = conf->get("first_scaledown", NOTHROW);
        max_dur_d = conf->get("max_dur", NOTHROW);
    }
    return batchspec_t(
        batch_type,
        max_els_d.has()
            ? max_els_d->as_int()
            : std::numeric_limits<decltype(batchspec_t().els_left)>::max(),
        min_els_d.has() ? min_els_d->as_int() : DEFAULT_MIN_WANTED_ELS,
        max_size_d.has() ? max_size_d->as_int() : DEFAULT_MAX_SIZE,
        first_scaledown_d.has()
            ? first_scaledown_d->as_int()
            : DEFAULT_FIRST_SCALEDOWN,
        (batch_type == batch_type_t::NORMAL || batch_type == batch_type_t::NORMAL_FIRST)
            ? current_microtime() + (max_dur_d.has()
                                     ? max_dur_d->as_int()
                                     : DEFAULT_MAX_TIME)
            : std::numeric_limits<decltype(batchspec_t().end_time)>::max());
}

batchspec_t batchspec_t::all() {
    return batchspec_t(batch_type_t::TERMINAL,
                       std::numeric_limits<decltype(batchspec_t().els_left)>::max(),
                       DEFAULT_MIN_WANTED_ELS,
                       std::numeric_limits<decltype(batchspec_t().size_left)>::max(),
                       1,
                       std::numeric_limits<decltype(batchspec_t().end_time)>::max());
}

batchspec_t batchspec_t::user(batch_type_t batch_type, env_t *env) {
    counted_t<val_t> vconf = env->global_optargs.get_optarg(env, "batch_conf");
    return user(
        batch_type,
        vconf.has() ? vconf->as_datum() : counted_t<const datum_t>());
}

batchspec_t batchspec_t::with_new_batch_type(batch_type_t new_batch_type) const {
    return batchspec_t(new_batch_type, els_left, min_wanted_els_left, size_left,
                       first_scaledown_factor, end_time);
}

batchspec_t batchspec_t::with_at_most(uint64_t _max_els) const {
    int64_t max_els = std::min(uint64_t(std::numeric_limits<int64_t>::max()), _max_els);
    return batchspec_t(
        batch_type,
        std::max<int64_t>(1, std::min(els_left, max_els)),
        std::min<int64_t>(min_wanted_els_left, max_els),
        size_left,
        first_scaledown_factor,
        end_time);
}

batchspec_t batchspec_t::scale_down(int64_t divisor) const {
    // These numbers are sort of arbitrary, but they seem to work.  We divide by
    // 7/8th of the divisor and add 8 to reduce the chances of needing a second
    // round-trip (we add a constant because unequal division is more likely
    // with very small sizes).  Law of large numbers says that the chances of
    // needing a second round-trip for large, non-pathological datasets are
    // extremely low.
    int64_t new_els_left =
        els_left == std::numeric_limits<decltype(batchspec_t().els_left)>::max()
        ? els_left
        : std::min(els_left, (els_left * 8 / (7 * divisor)) + 8);
    int64_t new_size_left =
        size_left == std::numeric_limits<decltype(batchspec_t().size_left)>::max()
        ? size_left
        : std::min(size_left, (size_left * 8 / (7 * divisor)) + 8);

    return batchspec_t(batch_type, new_els_left, min_wanted_els_left, new_size_left,
                       first_scaledown_factor, end_time);
}

batcher_t batchspec_t::to_batcher() const {
    int64_t real_size_left =
        size_left == std::numeric_limits<decltype(batchspec_t().size_left)>::max()
        || batch_type != batch_type_t::NORMAL_FIRST
            ? size_left
            : std::max<int64_t>(1, size_left / first_scaledown_factor);
    microtime_t real_end_time =
        (batch_type == batch_type_t::NORMAL || batch_type == batch_type_t::NORMAL_FIRST)
        && end_time > current_microtime()
            ? end_time
            : std::numeric_limits<decltype(batchspec_t().end_time)>::max();
    return batcher_t(batch_type, els_left, min_wanted_els_left, real_size_left,
                     real_end_time);
}

bool batcher_t::should_send_batch() const {
    // We ignore size_left as long as we have not got at least `min_wanted_els`
    // documents.
    return els_left <= 0
        || (size_left <= 0 && min_wanted_els_left <= 0)
        || (current_microtime() >= end_time && seen_one_el);
}

batcher_t::batcher_t(
    batch_type_t _batch_type,
    int64_t els,
    int64_t min_wanted_els,
    int64_t size,
    microtime_t _end_time)
    : batch_type(_batch_type),
      seen_one_el(false),
      els_left(els),
      min_wanted_els_left(min_wanted_els),
      size_left(size),
      end_time(_end_time) { }

size_t array_size_limit() { return 100000; }

} // namespace ql
