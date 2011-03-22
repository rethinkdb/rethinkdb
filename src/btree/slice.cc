#include <boost/scoped_ptr.hpp>

#include "btree/backfill.hpp"
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

void btree_slice_t::create(translator_serializer_t *serializer,
                           mirrored_cache_static_config_t *static_config) {

    cache_t::create(serializer, static_config);

    /* Construct a cache so we can write the superblock */

    /* The values we pass here are almost totally irrelevant. The cache-size parameter must
    be big enough to hold the patch log so we don't trip an assert, though. */
    mirrored_cache_config_t startup_dynamic_config;
    int size = static_config->n_patch_log_blocks * serializer->get_block_size().ser_value() + MEGABYTE;
    startup_dynamic_config.max_size = size * 2;
    startup_dynamic_config.wait_for_flush = false;
    startup_dynamic_config.flush_timer_ms = NEVER_FLUSH;
    startup_dynamic_config.max_dirty_size = size;
    startup_dynamic_config.flush_dirty_size = size;
    startup_dynamic_config.flush_waiting_threshold = INT_MAX;
    startup_dynamic_config.max_concurrent_flushes = 1;

    /* Cache is in a scoped pointer because it may be too big to allocate on the coroutine stack */
    boost::scoped_ptr<cache_t> cache(new cache_t(serializer, &startup_dynamic_config));

    /* Initialize the root block */
    transactor_t transactor(cache.get(), rwi_write, 1, current_time());
    buf_lock_t superblock(transactor, SUPERBLOCK_ID, rwi_write);
    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock->get_data_major_write());
    bzero((void*)sb, cache->get_block_size().value());
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

rget_result_t btree_slice_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key) {
    return btree_rget_slice(this, left_mode, left_key, right_mode, right_key);
}

struct btree_slice_change_visitor_t : public boost::static_visitor<mutation_result_t> {
    mutation_result_t operator()(const get_cas_mutation_t &m) {
        return btree_get_cas(m.key, parent, ct);
    }
    mutation_result_t operator()(const set_mutation_t &m) {
        return btree_set(m.key, parent, m.data, m.flags, m.exptime, m.add_policy, m.replace_policy, m.old_cas, ct);
    }
    mutation_result_t operator()(const incr_decr_mutation_t &m) {
        return btree_incr_decr(m.key, parent, (m.kind == incr_decr_INCR), m.amount, ct);
    }
    mutation_result_t operator()(const append_prepend_mutation_t &m) {
        return btree_append_prepend(m.key, parent, m.data, (m.kind == append_prepend_APPEND), ct);
    }
    mutation_result_t operator()(const delete_mutation_t &m) {
        return btree_delete(m.key, parent, ct.timestamp);
    }
    btree_slice_t *parent;
    castime_t ct;
};

mutation_result_t btree_slice_t::change(const mutation_t &m, castime_t castime) {
    btree_slice_change_visitor_t functor;
    functor.parent = this;
    functor.ct = castime;
    return boost::apply_visitor(functor, m.mutation);
}

void btree_slice_t::spawn_backfill(repli_timestamp since_when, backfill_callback_t *callback) {
    spawn_btree_backfill(this, since_when, callback);
}

void btree_slice_t::time_barrier(repli_timestamp lower_bound_on_future_timestamps) {
    transactor_t transactor(&cache(), rwi_write, lower_bound_on_future_timestamps);
    buf_lock_t superblock(transactor, SUPERBLOCK_ID, rwi_write);
    superblock->touch_recency(lower_bound_on_future_timestamps);
}
