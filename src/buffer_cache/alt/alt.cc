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

alt_buf_lock_t::alt_buf_lock_t()
    : txn_(NULL),
      cache_(NULL),
      current_page_acq_(),
      snapshot_node_(NULL) {
}

alt_buf_lock_t::alt_buf_lock_t(alt_txn_t *txn,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(txn),
      cache_(txn_->cache()),
      current_page_acq_(new current_page_acq_t(txn->page_txn(), block_id, access)),
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

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id, access));
}

alt_buf_lock_t::alt_buf_lock_t(alt_txn_t *txn,
                               alt_access_t access)
    : txn_(txn),
      cache_(txn_->cache()),
      current_page_acq_(),
      snapshot_node_(NULL) {
    guarantee(access == alt_access_t::write);
    current_page_acq_.init(new current_page_acq_t(txn->page_txn(), access));
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               alt_access_t access)
    : txn_(parent->txn_),
      cache_(txn_->cache()),
      current_page_acq_(),
      snapshot_node_(NULL) {
    guarantee(access == alt_access_t::write);

    // RSI: vvv Dedup this section with the other constructor.
    guarantee(is_subordinate(parent->access(), access));

    if (access == alt_access_t::write) {
        parent->write_acq_signal()->wait();
    } else {
        parent->read_acq_signal()->wait();
    }
    // ^^^ Dedup this section.

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), access));
}

alt_buf_lock_t::~alt_buf_lock_t() {
    // RSI: We'll have to do something with snapshot_node_ here.
}

void alt_buf_lock_t::swap(alt_buf_lock_t &other) {
    std::swap(txn_, other.txn_);
    std::swap(cache_, other.cache_);
    current_page_acq_.swap(other.current_page_acq_);
    std::swap(snapshot_node_, other.snapshot_node_);
}

void alt_buf_lock_t::snapshot_subtree() {
    guarantee(txn_ != NULL);
    // RSI: Actually implement this.
}

alt_buf_read_t::alt_buf_read_t(alt_buf_lock_t *lock)
    : lock_(lock) {
    guarantee(lock_->txn_ != NULL);
}

alt_buf_read_t::~alt_buf_read_t() {
    guarantee(lock_->txn_ != NULL);
}

const void *alt_buf_read_t::get_data_read(uint32_t *block_size_out) {
    guarantee(lock_->txn_ != NULL);
    lock_->current_page_acq_->read_acq_signal()->wait();
    page_t *page = lock_->current_page_acq_->current_page_for_read();
    guarantee(lock_->txn_ != NULL);
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache_->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    *block_size_out = page_acq_.get_buf_size();
    return page_acq_.get_buf_read();
}

alt_buf_write_t::alt_buf_write_t(alt_buf_lock_t *lock)
    : lock_(lock) {
    guarantee(lock_->txn_ != NULL);
}

alt_buf_write_t::~alt_buf_write_t() {
    guarantee(lock_->txn_ != NULL);
}

void *alt_buf_write_t::get_data_write(uint32_t block_size) {
    guarantee(lock_->txn_ != NULL);
    // RSI: Use block_size somehow.
    (void)block_size;
    lock_->current_page_acq_->write_acq_signal()->wait();
    guarantee(lock_->txn_ != NULL);
    page_t *page = lock_->current_page_acq_->current_page_for_write();
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache_->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    return page_acq_.get_buf_write();
}

void *alt_buf_write_t::get_data_write() {
    return get_data_write(lock_->cache_->max_block_size().value());
}


}  // namespace alt
