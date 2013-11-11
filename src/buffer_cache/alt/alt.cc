#define __STDC_LIMIT_MACROS
#include "buffer_cache/alt/alt.hpp"

#include "concurrency/auto_drainer.hpp"

namespace alt {

alt_cache_t::alt_cache_t(serializer_t *serializer)
    : page_cache_(serializer) { }

alt_cache_t::~alt_cache_t() {
    drainer_.reset();
}

block_size_t alt_cache_t::max_block_size() const {
    return page_cache_.max_block_size();
}

alt_txn_t::alt_txn_t(alt_cache_t *cache, alt_txn_t *preceding_txn)
    : cache_(cache),
      page_txn_(&cache->page_cache_, &preceding_txn->page_txn_),
      this_txn_timestamp_(repli_timestamp_t::invalid) { }

alt_buf_lock_t::alt_buf_lock_t(alt_txn_t *txn,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(txn),
      cache_(txn_->cache()),
      current_page_acq_(txn->page_txn(), block_id, access),
      snapshot_node_(NULL) {
    // RSI: Obviously, we want to use snapshot_node_ at some point.
    (void)snapshot_node_;
}

bool is_subordinate(alt_access_t parent, alt_access_t child) {
    return parent == alt_access_t::write || child == alt_access_t::read;
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(parent->txn_),
      cache_(txn_->cache()),
      current_page_acq_(),
      snapshot_node_(NULL) {

    guarantee(is_subordinate(parent->access(), access));

    if (access == alt_access_t::write) {
        parent->write_acq_signal()->wait();
    } else {
        parent->read_acq_signal()->wait();
    }

    current_page_acq_.init(txn_->page_txn(), block_id, access);
}

alt_buf_lock_t::~alt_buf_lock_t() {
}

void alt_buf_lock_t::snapshot_subtree() {
    // RSI: Actually implement this.
}

alt_buf_read_t::alt_buf_read_t(alt_buf_lock_t *lock)
    : lock_(lock) {
}

alt_buf_read_t::~alt_buf_read_t() { }

const void *alt_buf_read_t::get_data_read(uint32_t *block_size_out) {
    lock_->current_page_acq_.read_acq_signal()->wait();
    page_t *page = lock_->current_page_acq_.current_page_for_read();
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache_->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    *block_size_out = page_acq_.get_buf_size();
    return page_acq_.get_buf_read();
}

alt_buf_write_t::alt_buf_write_t(alt_buf_lock_t *lock)
    : lock_(lock) {
}

alt_buf_write_t::~alt_buf_write_t() { }

void *alt_buf_write_t::get_data_write(uint32_t block_size) {
    // RSI: Use block_size somehow.
    (void)block_size;
    lock_->current_page_acq_.write_acq_signal()->wait();
    page_t *page = lock_->current_page_acq_.current_page_for_write();
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache_->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    return page_acq_.get_buf_write();
}



}  // namespace alt
