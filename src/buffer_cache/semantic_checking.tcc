#include "buffer_cache/semantic_checking.hpp"

/* Buf */

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::swap(scc_buf_lock_t<inner_cache_t>& swapee) {
    rassert(internal_buf_lock.has());
    std::swap(cache, swapee.cache);
    internal_buf_lock->swap(*swapee.internal_buf_lock.get());
    std::swap(snapshotted, swapee.snapshotted);
    std::swap(has_been_changed, swapee.has_been_changed);
}

template<class inner_cache_t>
bool scc_buf_lock_t<inner_cache_t>::is_acquired() const {
    rassert(internal_buf_lock.has());
    return internal_buf_lock->is_acquired();
}

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::release_if_acquired() {
    if (internal_buf_lock->is_acquired()) {
        release();
    }
}

template<class inner_cache_t>
block_id_t scc_buf_lock_t<inner_cache_t>::get_block_id() const {
    rassert(internal_buf_lock.has());
    return internal_buf_lock->get_block_id();
}

template<class inner_cache_t>
const void *scc_buf_lock_t<inner_cache_t>::get_data_read() const {
    rassert(internal_buf_lock.has());
    return internal_buf_lock->get_data_read();
}

template<class inner_cache_t>
void *scc_buf_lock_t<inner_cache_t>::get_data_major_write() {
    rassert(internal_buf_lock.has());
    has_been_changed = true;
    return internal_buf_lock->get_data_major_write();
}

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::set_data(void *dest, const void *src, const size_t n) {
    rassert(internal_buf_lock.has());
    has_been_changed = true;
    internal_buf_lock->set_data(dest, src, n);
}

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::move_data(void *dest, const void *src, const size_t n) {
    rassert(internal_buf_lock.has());
    has_been_changed = true;
    internal_buf_lock->move_data(dest, src, n);
}

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::apply_patch(buf_patch_t *patch) {
    rassert(internal_buf_lock.has());
    has_been_changed = true;
    internal_buf_lock->apply_patch(patch);
}

template<class inner_cache_t>
patch_counter_t scc_buf_lock_t<inner_cache_t>::get_next_patch_counter() {
    rassert(internal_buf_lock.has());
    return internal_buf_lock->get_next_patch_counter();
}

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::mark_deleted() {
    rassert(internal_buf_lock.has());
    internal_buf_lock->mark_deleted();
}

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::touch_recency(repli_timestamp_t timestamp) {
    rassert(internal_buf_lock.has());
    // TODO: Why are we not tracking this?
    internal_buf_lock->touch_recency(timestamp);
}

template<class inner_cache_t>
void scc_buf_lock_t<inner_cache_t>::release() {
    rassert(internal_buf_lock.has());
    if (!snapshotted && !internal_buf_lock->is_deleted()) {
        if (!has_been_changed && cache->crc_map.get(internal_buf_lock->get_block_id())) {
            rassert(compute_crc() == cache->crc_map.get(internal_buf_lock->get_block_id()));
        } else {
            cache->crc_map.set(internal_buf_lock->get_block_id(), compute_crc());
        }
    }

    // TODO: We want to track order tokens here.
    //    if (!snapshotted) {
    //        cache->sink_map[internal_buf_lock->get_block_id()].check_out(order token);
    //    }

    internal_buf_lock->release();
}

template<class inner_cache_t>
repli_timestamp_t scc_buf_lock_t<inner_cache_t>::get_recency() const {
    rassert(internal_buf_lock.has());
    return internal_buf_lock->get_recency();
}

template<class inner_cache_t>
scc_buf_lock_t<inner_cache_t>::scc_buf_lock_t()
    : snapshotted(false), has_been_changed(false), internal_buf_lock(new typename inner_cache_t::buf_lock_t()), cache(NULL) { }

template<class inner_cache_t>
scc_buf_lock_t<inner_cache_t>::~scc_buf_lock_t() {
    release_if_acquired();
}

/* Transaction */

template<class inner_cache_t>
scc_transaction_t<inner_cache_t>::scc_transaction_t(scc_cache_t<inner_cache_t> *cache, access_t access, int expected_change_count, repli_timestamp_t recency_timestamp) :
    cache(cache),
    order_token(order_token_t::ignore),
    snapshotted(false),
    access(access),
    inner_transaction(&cache->inner_cache, access, expected_change_count, recency_timestamp)
    { }

template<class inner_cache_t>
scc_transaction_t<inner_cache_t>::~scc_transaction_t() {
}

template<class inner_cache_t>
void scc_transaction_t<inner_cache_t>::set_account(const boost::shared_ptr<typename inner_cache_t::cache_account_t>& cache_account) {
    inner_transaction.set_account(cache_account);
}

template<class inner_cache_t>
scc_buf_lock_t<inner_cache_t>::scc_buf_lock_t(scc_transaction_t<inner_cache_t> *txn, block_id_t block_id, access_t mode, buffer_cache_order_mode_t order_mode, lock_in_line_callback_t *call_when_in_line) :
    snapshotted(txn->snapshotted || mode == rwi_read_outdated_ok),
    has_been_changed(false),
    internal_buf_lock(NULL),
    cache(txn->cache)
{
    if (order_mode == buffer_cache_order_mode_check && !txn->snapshotted) {
        cache->sink_map[block_id].check_out(txn->order_token);
    }

    internal_buf_lock.init(new typename inner_cache_t::buf_lock_t(&txn->inner_transaction, block_id, mode, order_mode, call_when_in_line));
    rassert(block_id == get_block_id());
    if (!txn->snapshotted) {
        if (cache->crc_map.get(block_id)) {
            rassert(compute_crc() == cache->crc_map.get(block_id));
        } else {
            txn->cache->crc_map.set(block_id, compute_crc());
        }
    }
}

template<class inner_cache_t>
scc_buf_lock_t<inner_cache_t>::scc_buf_lock_t(scc_transaction_t<inner_cache_t> *txn) :
    snapshotted(txn->snapshotted || txn->access == rwi_read_outdated_ok),
    has_been_changed(false),
    internal_buf_lock(new typename inner_cache_t::buf_lock_t(&txn->inner_transaction)),
    cache(txn->cache)
{
    cache->crc_map.set(internal_buf_lock->get_block_id(), compute_crc());
}

template<class inner_cache_t>
void scc_transaction_t<inner_cache_t>::get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp_t *recencies_out, get_subtree_recencies_callback_t *cb) {
    return inner_transaction.get_subtree_recencies(block_ids, num_block_ids, recencies_out, cb);
}

/* Cache */

template<class inner_cache_t>
void scc_cache_t<inner_cache_t>::create(
        serializer_t *serializer,
        mirrored_cache_static_config_t *static_config)
{
    inner_cache_t::create(serializer, static_config);
}

template<class inner_cache_t>
scc_cache_t<inner_cache_t>::scc_cache_t(serializer_t *serializer,
                                        mirrored_cache_config_t *dynamic_config,
                                        perfmon_collection_t *parent)
    : inner_cache(serializer, dynamic_config, parent) {
}

template<class inner_cache_t>
block_size_t scc_cache_t<inner_cache_t>::get_block_size() {
    return inner_cache.get_block_size();
}

template<class inner_cache_t>
boost::shared_ptr<typename inner_cache_t::cache_account_t> scc_cache_t<inner_cache_t>::create_account(int priority) {
    return inner_cache.create_account(priority);
}

template<class inner_cache_t>
bool scc_cache_t<inner_cache_t>::offer_read_ahead_buf(block_id_t block_id, void *buf, const intrusive_ptr_t<standard_block_token_t>& token, repli_timestamp_t recency_timestamp) {
    return inner_cache.offer_read_ahead_buf(block_id, buf, token, recency_timestamp);
}

template<class inner_cache_t>
bool scc_cache_t<inner_cache_t>::contains_block(block_id_t block_id) {
    return inner_cache.contains_block(block_id);
}
