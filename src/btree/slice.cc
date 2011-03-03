#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "buffer_cache/transactor.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "concurrency/cond_var.hpp"
#include "btree/get.hpp"
#include "btree/rget.hpp"
#include "btree/set.hpp"
#include "btree/incr_decr.hpp"
#include "btree/append_prepend.hpp"
#include "btree/delete.hpp"
#include "btree/get_cas.hpp"
#include "replication/master.hpp"
#include <boost/scoped_ptr.hpp>

void btree_slice_t::create(translator_serializer_t *serializer,
                           mirrored_cache_static_config_t *static_config) {

    cache_t::create(serializer, static_config);

    /* Construct a cache so we can write the superblock */

    /* The values we pass here are almost totally irrelevant. The cache-size parameter must
    be big enough to hold the patch log so we don't trip an assert, though. */
    mirrored_cache_config_t startup_dynamic_config;
    int size = static_config->n_patch_log_blocks * serializer->get_block_size().value() + MEGABYTE;
    startup_dynamic_config.max_size = size;
    startup_dynamic_config.wait_for_flush = false;
    startup_dynamic_config.flush_timer_ms = NEVER_FLUSH;
    startup_dynamic_config.max_dirty_size = size;
    startup_dynamic_config.flush_dirty_size = size;
    startup_dynamic_config.flush_waiting_threshold = INT_MAX;
    startup_dynamic_config.max_concurrent_flushes = 1;

    /* Cache is in a scoped pointer because it may be too big to allocate on the coroutine stack */
    boost::scoped_ptr<cache_t> cache(new cache_t(serializer, &startup_dynamic_config));

    /* Initialize the root block */
    transactor_t transactor(cache.get(), rwi_write);
    buf_lock_t superblock(transactor, SUPERBLOCK_ID, rwi_write);
    btree_superblock_t *sb = (btree_superblock_t*)(superblock.buf()->get_data_major_write());
    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;

    // Destructors handle cleanup and stuff
}

btree_slice_t::btree_slice_t(translator_serializer_t *serializer,
                             mirrored_cache_config_t *dynamic_config)
    : cache_(serializer, dynamic_config) { }

btree_slice_t::~btree_slice_t() {
    // Cache's destructor handles flushing and stuff
}

get_result_t btree_slice_t::get(const store_key_t &key) {
    return btree_get(key, this);
}

get_result_t btree_slice_t::get_cas(const store_key_t &key, castime_t castime) {
    return btree_get_cas(key, this, castime);
}

rget_result_t btree_slice_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key) {
    return btree_rget_slice(this, left_mode, left_key, right_mode, right_key);
}

set_result_t btree_slice_t::sarc(const store_key_t &key, data_provider_t *data, mcflags_t flags, exptime_t exptime, castime_t castime,
                                          add_policy_t add_policy, replace_policy_t replace_policy, cas_t old_cas) {
    return btree_set(key, this, data, flags, exptime, add_policy, replace_policy, old_cas, castime);
}

incr_decr_result_t btree_slice_t::incr_decr(incr_decr_kind_t kind, const store_key_t &key, uint64_t amount, castime_t castime) {
    return btree_incr_decr(key, this, kind == incr_decr_INCR, amount, castime);
}

append_prepend_result_t btree_slice_t::append_prepend(append_prepend_kind_t kind, const store_key_t &key, data_provider_t *data, castime_t castime) {
    return btree_append_prepend(key, this, data, kind == append_prepend_APPEND, castime);
}

delete_result_t btree_slice_t::delete_key(const store_key_t &key, repli_timestamp timestamp) {
    return btree_delete(key, this, timestamp);
}

// Stats

perfmon_duration_sampler_t
    pm_cmd_set("cmd_set", secs_to_ticks(1)),
    pm_cmd_get_without_threads("cmd_get_without_threads", secs_to_ticks(1)),
    pm_cmd_get("cmd_get", secs_to_ticks(1)),
    pm_cmd_rget("cmd_rget", secs_to_ticks(1));
