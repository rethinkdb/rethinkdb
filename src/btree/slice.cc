#include "errors.hpp"
#include "btree/backfill.hpp"
#include "btree/erase_range.hpp"
#include "btree/slice.hpp"
#include "btree/node.hpp"
#include "buffer_cache/buffer_cache.hpp"
#include "buffer_cache/buf_lock.hpp"
#include "concurrency/cond_var.hpp"
#include "btree/get.hpp"
#include "btree/rget.hpp"
#include "btree/set.hpp"
#include "btree/incr_decr.hpp"
#include "btree/append_prepend.hpp"
#include "btree/delete.hpp"
#include "btree/get_cas.hpp"

void btree_slice_t::create(cache_t *cache) {

    /* Initialize the btree superblock and the delete queue */
    transaction_t txn(cache, rwi_write, 1, repli_timestamp_t::distant_past);

    buf_lock_t superblock(&txn, SUPERBLOCK_ID, rwi_write);

    // Initialize replication time barrier to 0 so that if we are a slave, we will begin by pulling
    // ALL updates from master.
    superblock->touch_recency(repli_timestamp_t::distant_past);

    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock->get_data_major_write());
    bzero(sb, cache->get_block_size().value());

    sb->magic = btree_superblock_t::expected_magic;
    sb->root_block = NULL_BLOCK_ID;

    sb->replication_clock = sb->last_sync = repli_timestamp_t::distant_past;
    sb->replication_master_id = sb->replication_slave_id = 0;
}

btree_slice_t::btree_slice_t(cache_t *c)
    : cache_(c), backfill_account(cache()->create_account(BACKFILL_CACHE_PRIORITY)) {
    order_checkpoint_.set_tagappend("slice");
    post_begin_transaction_checkpoint_.set_tagappend("post");
}

btree_slice_t::~btree_slice_t() {
    // Cache's destructor handles flushing and stuff
}

get_result_t btree_slice_t::get(const store_key_t &key, order_token_t token) {
    assert_thread();
    token = order_checkpoint_.check_through(token);
    return btree_get(key, this, token);
}

rget_result_t btree_slice_t::rget(rget_bound_mode_t left_mode, const store_key_t &left_key, rget_bound_mode_t right_mode, const store_key_t &right_key, order_token_t token) {
    assert_thread();
    token = order_checkpoint_.check_through(token);
    return btree_rget_slice(this, left_mode, left_key, right_mode, right_key, token);
}

struct btree_slice_change_visitor_t : public boost::static_visitor<mutation_result_t> {
    mutation_result_t operator()(const get_cas_mutation_t &m) {
        return mutation_result_t(btree_get_cas(m.key, parent, ct, order_token));
    }
    mutation_result_t operator()(const sarc_mutation_t &m) {
        return mutation_result_t(btree_set(m.key, parent, m.data, m.flags, m.exptime, m.add_policy, m.replace_policy, m.old_cas, ct, order_token));
    }
    mutation_result_t operator()(const incr_decr_mutation_t &m) {
        return mutation_result_t(btree_incr_decr(m.key, parent, (m.kind == incr_decr_INCR), m.amount, ct, order_token));
    }
    mutation_result_t operator()(const append_prepend_mutation_t &m) {
        return mutation_result_t(btree_append_prepend(m.key, parent, m.data, (m.kind == append_prepend_APPEND), ct, order_token));
    }
    mutation_result_t operator()(const delete_mutation_t &m) {
        return mutation_result_t(btree_delete(m.key, m.dont_put_in_delete_queue, parent, ct.timestamp, order_token));
    }

    btree_slice_change_visitor_t(btree_slice_t *_parent, castime_t _ct, order_token_t _order_token)
        : parent(_parent), ct(_ct), order_token(_order_token) { }

private:
    btree_slice_t *parent;
    castime_t ct;
    order_token_t order_token;

    DISABLE_COPYING(btree_slice_change_visitor_t);
};

mutation_result_t btree_slice_t::change(const mutation_t &m, castime_t castime, order_token_t token) {
    // If you're calling this from the wrong thread, you're not
    // thinking about the problem enough.
    assert_thread();

    token = order_checkpoint_.check_through(token);

    btree_slice_change_visitor_t functor(this, castime, token);
    return boost::apply_visitor(functor, m.mutation);
}


void btree_slice_t::backfill_delete_range(key_tester_t *tester,
                                          bool left_key_supplied, const store_key_t& left_key_exclusive,
                                          bool right_key_supplied, const store_key_t& right_key_inclusive,
                                          order_token_t token) {
    assert_thread();

    token = order_checkpoint_.check_through(token);

    btree_erase_range(this, tester,
                      left_key_supplied, left_key_exclusive,
                      right_key_supplied, right_key_inclusive,
                      token);
}


void btree_slice_t::backfill(repli_timestamp_t since_when, backfill_callback_t *callback, order_token_t token) {
    assert_thread();

    token = order_checkpoint_.check_through(token);

    btree_backfill(this, since_when, backfill_account, callback, token);
}

void btree_slice_t::set_replication_clock(repli_timestamp_t t, order_token_t token) {
    assert_thread();

    token = order_checkpoint_.check_through(token);

    transaction_t transaction(cache(), rwi_write, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token (not with the token parameter).
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_write);
    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock->get_data_major_write());
    //    rassert(sb->replication_clock < t, "sb->replication_clock = %u, t = %u", sb->replication_clock.time, t.time);
    sb->replication_clock = std::max(sb->replication_clock, t);
}

// TODO: Why are we using repli_timestamp_t::distant_past instead of
// repli_timestamp_t::invalid?

repli_timestamp_t btree_slice_t::get_replication_clock() {
    on_thread_t th(cache()->home_thread());
    transaction_t transaction(cache(), rwi_read, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token.
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_read);
    const btree_superblock_t *sb = reinterpret_cast<const btree_superblock_t *>(superblock->get_data_read());
    return sb->replication_clock;
}

void btree_slice_t::set_last_sync(repli_timestamp_t t, UNUSED order_token_t token) {
    on_thread_t th(cache()->home_thread());

    // TODO: We need to make sure that callers are using a proper substore token.

    //    token = order_checkpoint_.check_out(token);

    transaction_t transaction(cache(), rwi_write, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token (not with the token parameter).
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_write);
    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock->get_data_major_write());
    sb->last_sync = t;
}

repli_timestamp_t btree_slice_t::get_last_sync() {
    on_thread_t th(cache()->home_thread());
    transaction_t transaction(cache(), rwi_read, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token.
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_read);
    const btree_superblock_t *sb = reinterpret_cast<const btree_superblock_t *>(superblock->get_data_read());
    return sb->last_sync;
}

void btree_slice_t::set_replication_master_id(uint32_t t) {
    on_thread_t th(cache()->home_thread());
    transaction_t transaction(cache(), rwi_write, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token.
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_write);
    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock->get_data_major_write());
    sb->replication_master_id = t;
}

uint32_t btree_slice_t::get_replication_master_id() {
    on_thread_t th(cache()->home_thread());
    transaction_t transaction(cache(), rwi_read, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token.
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_read);
    const btree_superblock_t *sb = reinterpret_cast<const btree_superblock_t *>(superblock->get_data_read());
    return sb->replication_master_id;
}

void btree_slice_t::set_replication_slave_id(uint32_t t) {
    on_thread_t th(cache()->home_thread());
    transaction_t transaction(cache(), rwi_write, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token.
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_write);
    btree_superblock_t *sb = reinterpret_cast<btree_superblock_t *>(superblock->get_data_major_write());
    sb->replication_slave_id = t;
}

uint32_t btree_slice_t::get_replication_slave_id() {
    on_thread_t th(cache()->home_thread());
    transaction_t transaction(cache(), rwi_read, 0, repli_timestamp_t::distant_past);
    // TODO: Set the transaction's order token.
    buf_lock_t superblock(&transaction, SUPERBLOCK_ID, rwi_read);
    const btree_superblock_t *sb = reinterpret_cast<const btree_superblock_t *>(superblock->get_data_read());
    return sb->replication_slave_id;
}

