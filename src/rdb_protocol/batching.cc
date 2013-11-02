// Copyright 2010-2013 RethinkDB, all rights reserved.

#include "rdb_protocol/batching.hpp"

#include "rdb_protocol/datum.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

batcher_t::batcher_t(batch_type_t _batch_type, int64_t els, int64_t size,
                     microtime_t _end_time)
    : batch_type(_batch_type), seen_one_el(false),
      els_left(els), size_left(size), end_time(_end_time) { }

batcher_t batcher_t::user_batcher(batch_type_t batch_type,
                                  const counted_t<const datum_t> &conf) {
    counted_t<const datum_t> max_els_d, max_size_d, max_dur_d;
    if (conf.has()) {
        max_els_d = conf->get("max_els", NOTHROW);
        max_size_d = conf->get("max_size", NOTHROW);
        max_dur_d = conf->get("max_dur", NOTHROW);
    }
    return batcher_t(
        batch_type,
        max_els_d.has()
            ? max_els_d->as_int()
            : std::numeric_limits<decltype(batcher_t().els_left)>::max(),
        max_size_d.has() ? max_size_d->as_int() : MEGABYTE / 4,
        (batch_type == NORMAL)
            ? current_microtime() + (max_dur_d.has() ? max_dur_d->as_int() : 500 * 1000)
            : std::numeric_limits<decltype(batcher_t().end_time)>::max());
}

batcher_t batcher_t::user_batcher(batch_type_t batch_type, env_t *env) {
    counted_t<val_t> vconf = env->global_optargs.get_optarg(env, "batch_conf");
    return user_batcher(
        batch_type,
        vconf.has() ? vconf->as_datum() : counted_t<const datum_t>());
}

bool batcher_t::should_send_batch() const {
    return els_left <= 0
        || size_left <= 0
        || (current_microtime() >= end_time && seen_one_el);
}

} // namespace ql
