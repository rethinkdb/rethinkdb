/* Buf */

template<class inner_cache_t>
block_id_t scc_buf_t<inner_cache_t>::get_block_id() const {
    rassert(inner_buf);
    return inner_buf->get_block_id();
}

template<class inner_cache_t>
const void *scc_buf_t<inner_cache_t>::get_data_read() const {
    rassert(inner_buf);
    return inner_buf->get_data_read();
}

template<class inner_cache_t>
void *scc_buf_t<inner_cache_t>::get_data_major_write() {
    has_been_changed = true;
    return inner_buf->get_data_major_write();
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::set_data(void *dest, const void *src, const size_t n) {
    has_been_changed = true;
    inner_buf->set_data(dest, src, n);
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::move_data(void *dest, const void *src, const size_t n) {
    has_been_changed = true;
    inner_buf->move_data(dest, src, n);
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::apply_patch(buf_patch_t *patch) {
    has_been_changed = true;
    inner_buf->apply_patch(patch);
}

template<class inner_cache_t>
patch_counter_t scc_buf_t<inner_cache_t>::get_next_patch_counter() {
    return inner_buf->get_next_patch_counter();
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::mark_deleted() {
    rassert(inner_buf);
    inner_buf->mark_deleted();
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::touch_recency(repli_timestamp_t timestamp) {
    rassert(inner_buf);
    // TODO: Why are we not tracking this?
    inner_buf->touch_recency(timestamp);
}

template<class inner_cache_t>
void scc_buf_t<inner_cache_t>::release() {
    rassert(inner_buf);
    if (!snapshotted && !inner_buf->is_deleted()) {
        /* There are two valid use cases for should_load == false:
         1. deletion of buffers
         2. overwriting the value of a buffer
         If we got here, it has to be case 2, so we hope that the buffer has been filled
         with data by now and compute a new crc.
        */
        rassert(should_load || has_been_changed);
        if (!has_been_changed && cache->crc_map.get(inner_buf->get_block_id())) {
            rassert(compute_crc() == cache->crc_map.get(inner_buf->get_block_id()));
        } else {
            cache->crc_map.set(inner_buf->get_block_id(), compute_crc());
        }
    }

    // TODO: We want to track order tokens here.
    //    if (!snapshotted) {
    //        cache->sink_map[inner_buf->get_block_id()].check_out(order token);
    //    }

    inner_buf->release();
    delete this;
}

template<class inner_cache_t>
scc_buf_t<inner_cache_t>::scc_buf_t(scc_cache_t<inner_cache_t> *_cache, bool snapshotted, bool should_load)
    : snapshotted(snapshotted), should_load(should_load), has_been_changed(false), inner_buf(NULL), cache(_cache) { }

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
scc_transaction_t<inner_cache_t>::scc_transaction_t(scc_cache_t<inner_cache_t> *cache, access_t access) :
    cache(cache),
    order_token(order_token_t::ignore),
    snapshotted(false),
    access(access),
    inner_transaction(&cache->inner_cache, access)
{
    rassert(access == rwi_read || access == rwi_read_sync);
}

template<class inner_cache_t>
scc_transaction_t<inner_cache_t>::~scc_transaction_t() {
}

template<class inner_cache_t>
void scc_transaction_t<inner_cache_t>::set_account(const boost::shared_ptr<typename inner_cache_t::cache_account_t>& cache_account) {
    inner_transaction.set_account(cache_account);
}

template<class inner_cache_t>
scc_buf_t<inner_cache_t> *scc_transaction_t<inner_cache_t>::acquire(block_id_t block_id, access_t mode,
                   boost::function<void()> call_when_in_line, bool should_load) {
    scc_buf_t<inner_cache_t> *buf = new scc_buf_t<inner_cache_t>(this->cache, snapshotted || mode == rwi_read_outdated_ok, should_load);
    buf->cache = this->cache;
    if (!snapshotted) {
        cache->sink_map[block_id].check_out(order_token);
    }

    typename inner_cache_t::buf_t *inner_buf = inner_transaction.acquire(block_id, mode, call_when_in_line, should_load);
    buf->inner_buf = inner_buf;
    rassert(block_id == buf->get_block_id());
    if (!snapshotted && should_load) {
        if (cache->crc_map.get(block_id)) {
            rassert(buf->compute_crc() == cache->crc_map.get(block_id));
        } else {
            cache->crc_map.set(block_id, buf->compute_crc());
        }
    }

    return buf;
}

template<class inner_cache_t>
scc_buf_t<inner_cache_t> *scc_transaction_t<inner_cache_t>::allocate() {
    scc_buf_t<inner_cache_t> *buf = new scc_buf_t<inner_cache_t>(this->cache, snapshotted, true);
    buf->inner_buf = inner_transaction.allocate();
    cache->crc_map.set(buf->inner_buf->get_block_id(), buf->compute_crc());
    return buf;
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
scc_cache_t<inner_cache_t>::scc_cache_t(
        serializer_t *serializer,
        mirrored_cache_config_t *dynamic_config)
    : inner_cache(serializer, dynamic_config) {
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
bool scc_cache_t<inner_cache_t>::offer_read_ahead_buf(block_id_t block_id, void *buf, repli_timestamp_t recency_timestamp) {
    return inner_cache.offer_read_ahead_buf(block_id, buf, recency_timestamp);
}

template<class inner_cache_t>
bool scc_cache_t<inner_cache_t>::contains_block(block_id_t block_id) {
    return inner_cache.contains_block(block_id);
}
