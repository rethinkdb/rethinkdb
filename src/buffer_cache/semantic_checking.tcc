#include "buffer_cache/semantic_checking.hpp"

/* Buf */

template<class inner_cache_t>
block_id_t scc_buf_t<inner_cache_t>::get_block_id() {
    rassert(inner_buf);
    return inner_buf->get_block_id();
}

template<class inner_cache_t>
bool scc_buf_t<inner_cache_t>::is_dirty() {
    rassert(inner_buf);
    return inner_buf->is_dirty();
}

template<class inner_cache_t>
const void *scc_buf_t<inner_cache_t>::get_data_read() const {
    rassert(inner_buf);
    return inner_buf->get_data_read();
}

template<class inner_cache_t>
void *scc_buf_t<inner_cache_t>::get_data_major_write() {
    return inner_buf->get_data_major_write();
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::set_data(void *dest, const void *src, const size_t n) {
    inner_buf->set_data(dest, src, n);
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::move_data(void *dest, const void *src, const size_t n) {
    inner_buf->move_data(dest, src, n);
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::apply_patch(buf_patch_t *patch) {
    inner_buf->apply_patch(patch);
}

template<class inner_cache_t>
patch_counter_t scc_buf_t<inner_cache_t>::get_next_patch_counter() {
    return inner_buf->get_next_patch_counter();
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::mark_deleted(bool write_null) {
    rassert(inner_buf);
    inner_buf->mark_deleted(write_null);
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::touch_recency(repli_timestamp timestamp) {
    rassert(inner_buf);
    // TODO: Why are we not tracking this?
    inner_buf->touch_recency(timestamp);
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::release() {
    rassert(inner_buf);
    if (!snapshotted && !inner_buf->is_deleted()) {
        if (!inner_buf->is_dirty() && cache->crc_map.get(inner_buf->get_block_id())) {
            rassert(compute_crc() == cache->crc_map.get(inner_buf->get_block_id()));
        } else {
            cache->crc_map.set(inner_buf->get_block_id(), compute_crc());
        }
    }

    inner_buf->release();
    delete this;
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::on_block_available(typename inner_cache_t::buf_t *buf) {
    rassert(!inner_buf);
    rassert(buf);

    inner_buf = buf;
    if (!snapshotted) {
        if (cache->crc_map.get(inner_buf->get_block_id())) {
            rassert(compute_crc() == cache->crc_map.get(inner_buf->get_block_id()));
        } else {
            cache->crc_map.set(inner_buf->get_block_id(), compute_crc());
        }
    }
    if (available_cb) available_cb->on_block_available(this);
}

template<class inner_cache_t>
scc_buf_t<inner_cache_t>::scc_buf_t(scc_cache_t<inner_cache_t> *_cache, bool snapshotted)
    : snapshotted(snapshotted), inner_buf(NULL), available_cb(NULL), cache(_cache) { }

/* Transaction */

template<class inner_cache_t>
bool scc_transaction_t<inner_cache_t>::commit(transaction_commit_callback_t *callback) {
    if (inner_transaction->commit(this)) {
        delete this;
        return true;
    } else {
        commit_cb = callback;
        return false;
    }
}

template<class inner_cache_t>
scc_buf_t<inner_cache_t> *scc_transaction_t<inner_cache_t>::acquire(block_id_t block_id, access_t mode,
                   block_available_callback_t *callback, bool should_load) {
    scc_buf_t<inner_cache_t> *buf = new scc_buf_t<inner_cache_t>(this->cache, snapshotted);
    buf->cache = this->cache;
    if (typename inner_cache_t::buf_t *inner_buf = inner_transaction->acquire(block_id, mode, buf, should_load)) {
        buf->inner_buf = inner_buf;
        rassert(block_id == buf->get_block_id());
        if (!snapshotted) {
            if (cache->crc_map.get(block_id)) {
                rassert(buf->compute_crc() == cache->crc_map.get(block_id));
            } else {
                cache->crc_map.set(block_id, buf->compute_crc());
            }
        }
        return buf;
    } else {
        buf->available_cb = callback;
        return NULL;
    }
}

template<class inner_cache_t>
scc_buf_t<inner_cache_t> *scc_transaction_t<inner_cache_t>::allocate() {
    scc_buf_t<inner_cache_t> *buf = new scc_buf_t<inner_cache_t>(this->cache, snapshotted);
    buf->inner_buf = inner_transaction->allocate();
    cache->crc_map.set(buf->inner_buf->get_block_id(), buf->compute_crc());
    return buf;
}

template<class inner_cache_t>
void scc_transaction_t<inner_cache_t>::co_get_subtree_recencies(block_id_t *block_ids, size_t num_block_ids, repli_timestamp *recencies_out) {
    return inner_transaction->co_get_subtree_recencies(block_ids, num_block_ids, recencies_out);
}

template<class inner_cache_t>
scc_transaction_t<inner_cache_t>::scc_transaction_t(access_t _access, scc_cache_t<inner_cache_t> *_cache)
    : cache(_cache), snapshotted(false), access(_access), begin_cb(NULL), inner_transaction(NULL) { }

template<class inner_cache_t>
void scc_transaction_t<inner_cache_t>::on_txn_begin(typename inner_cache_t::transaction_t *txn) {
    rassert(!inner_transaction);
    inner_transaction = txn;
    if (begin_cb) begin_cb->on_txn_begin(this);
}

template<class inner_cache_t>
void scc_transaction_t<inner_cache_t>::on_txn_commit(UNUSED typename inner_cache_t::transaction_t *txn) {
    if (commit_cb) commit_cb->on_txn_commit(this);
    delete this;
}

/* Cache */

template<class inner_cache_t>
void scc_cache_t<inner_cache_t>::create(
        translator_serializer_t *serializer,
        mirrored_cache_static_config_t *static_config)
{
    inner_cache_t::create(serializer, static_config);
}

template<class inner_cache_t>
scc_cache_t<inner_cache_t>::scc_cache_t(
        translator_serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config)
    : inner_cache(serializer, dynamic_config) {
}

template<class inner_cache_t>
block_size_t scc_cache_t<inner_cache_t>::get_block_size() {
    return inner_cache.get_block_size();
}

template<class inner_cache_t>
scc_transaction_t<inner_cache_t> *scc_cache_t<inner_cache_t>::begin_transaction(access_t access, int expected_change_count, repli_timestamp recency_timestamp, transaction_begin_callback_t *callback) {
    scc_transaction_t<inner_cache_t> *txn = new scc_transaction_t<inner_cache_t>(access, this);
    if (typename inner_cache_t::transaction_t *inner_txn = inner_cache.begin_transaction(access, expected_change_count, recency_timestamp, txn)) {
        txn->inner_transaction = inner_txn;
        return txn;
    } else {
        txn->begin_cb = callback;
        return NULL;
    }
}
