#include "buffer_cache/alt/alt.hpp"

namespace alt {

alt_cache_t::alt_cache_t(serializer_t *serializer)
    : page_cache_(serializer) { }

alt_cache_t::~alt_cache_t() {
    drainer_.reset();
    page_cache_.reset();
}

alt_txn_t::alt_txn_t(alt_cache_t *cache, alt_txn_t *preceding_txn)
    : cache_(cache), preceding_txn_(preceding_txn),
      this_txn_timestamp_(repli_timestamp_t::invalid) { }

void alt_txn_t::remove_buf_lock(alt_buf_lock_t *buf_lock) {
    if (buf_lock->dirty()) {
        modified_blocks_.push_back(buf_lock->block_id());
    }
    if (buf_lock->access() == alt_access_t::write) {
        updated_metadata_.push_back(buf_lock->block_id());
    }
}

alt_buf_lock_t::alt_buf_lock_t(alt_txn_t *txn,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(txn),
      cache_(txn_->cache()),
      block_id_(block_id),
      current_page_acq_(txn->cache()->page_cache_.page_for_block_id(block_id),
                        access),
      snapshot_node_(NULL) {
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(parent->txn_),
      cache_(txn_->cache()),
      block_id_(block_id),
      current_page_acq_(parent->txn_->cache()->page_cache_.page_for_block_id(block_id),
                        access),
      snapshot_node_(NULL) {
}

alt_buf_lock_t::~alt_buf_lock_t() {
    txn_->remove_buf_lock(this);
}

alt_buf_read_t::alt_buf_read_t(alt_buf_lock_t *lock)
    : lock_(lock) {
}

alt_buf_read_t::~alt_buf_read_t() { }

const void *alt_buf_read_t::get_data_read(uint32_t *block_size_out) {
    lock_->current_page_acq_.read_acq_signal()->wait();
    page_t *page = lock_->current_page_acq_->page_for_read();
    if (!page_acq_.has()) {
        page_acq_.init(page);
    }
    page_acq_.buf_ready_signal()->wait();
    return page->get_buf();
}

alt_buf_write_t::alt_buf_write_t(alt_buf_lock_t *lock)
    : lock_(lock) {
}

alt_buf_write_t::~alt_buf_write_t() { }

void *alt_buf_write_t::get_data_write(uint32_t block_size) {
    lock_->current_page_acq_.write_acq_signal()->wait();
    page_t *page = lock_->current_page_acq_->page_for_write();
    if (!page_acq_.has()) {
        page_acq_.init(page);
    }
    page_acq_.buf_ready_signal()->wait();
    return page->get_buf();
}



}  // namespace alt
