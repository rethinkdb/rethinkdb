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
      page_txn_(&cache->page_cache_,
                preceding_txn == NULL ? NULL : &preceding_txn->page_txn_),
      this_txn_timestamp_(repli_timestamp_t::invalid) {
    // RSI: Use this_txn_timestamp_ for something.
    (void)this_txn_timestamp_;
}

alt_txn_t::~alt_txn_t() {
    // RSI: Do anything?
}

alt_buf_lock_t::alt_buf_lock_t()
    : txn_(NULL),
      cache_(NULL),
      current_page_acq_(),
      snapshot_node_(NULL) {
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_parent_t parent,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(parent.txn()),
      cache_(txn_->cache()),
      current_page_acq_(),
      snapshot_node_(NULL) {
    alt_buf_lock_t::wait_for_parent(parent, access);
    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id, access));
}

alt_buf_lock_t::alt_buf_lock_t(alt_txn_t *txn,
                               block_id_t block_id,
                               alt_create_t create)
    : txn_(txn),
      cache_(txn_->cache()),
      current_page_acq_(new current_page_acq_t(txn->page_txn(), block_id,
                                               alt_access_t::write, true)),
      snapshot_node_(NULL) {
    guarantee(create == alt_create_t::create);
}


bool is_subordinate(alt_access_t parent, alt_access_t child) {
    return parent == alt_access_t::write || child == alt_access_t::read;
}

void alt_buf_lock_t::wait_for_parent(alt_buf_parent_t parent, alt_access_t access) {
    if (parent.lock_or_null_ != NULL) {
        alt_buf_lock_t *lock = parent.lock_or_null_;
        guarantee(is_subordinate(lock->access(), access));
        if (access == alt_access_t::write) {
            lock->write_acq_signal()->wait();
        } else {
            lock->read_acq_signal()->wait();
        }
    }
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(parent->txn_),
      cache_(txn_->cache()),
      current_page_acq_(),
      snapshot_node_(NULL) {

    alt_buf_lock_t::wait_for_parent(alt_buf_parent_t(parent), access);

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id, access));
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_parent_t parent,
                               alt_create_t create)
    : txn_(parent.txn()),
      cache_(txn_->cache()),
      current_page_acq_(),
      snapshot_node_(NULL) {
    guarantee(create == alt_create_t::create);
    wait_for_parent(parent, alt_access_t::write);
    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  alt_access_t::write));
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               alt_create_t create)
    : txn_(parent->txn_),
      cache_(txn_->cache()),
      current_page_acq_(),
      snapshot_node_(NULL) {
    guarantee(create == alt_create_t::create);
    wait_for_parent(alt_buf_parent_t(parent), alt_access_t::write);

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  alt_access_t::write));
}

alt_buf_lock_t::~alt_buf_lock_t() {
    // RSI: We'll have to do something with snapshot_node_ here.
    (void)snapshot_node_;
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t &&movee)
    : txn_(movee.txn_), cache_(movee.cache_),
      current_page_acq_(std::move(movee.current_page_acq_)),
      snapshot_node_(movee.snapshot_node_) {
    movee.txn_ = NULL;
    movee.cache_ = NULL;
    movee.current_page_acq_.reset();
    movee.snapshot_node_ = NULL;
}

alt_buf_lock_t &alt_buf_lock_t::operator=(alt_buf_lock_t &&movee) {
    alt_buf_lock_t tmp(std::move(movee));
    swap(tmp);
    return *this;
}

void alt_buf_lock_t::swap(alt_buf_lock_t &other) {
    std::swap(txn_, other.txn_);
    std::swap(cache_, other.cache_);
    current_page_acq_.swap(other.current_page_acq_);
    std::swap(snapshot_node_, other.snapshot_node_);
}

void alt_buf_lock_t::reset_buf_lock() {
    alt_buf_lock_t tmp;
    swap(tmp);
}

bool alt_buf_lock_t::empty() const {
    return txn_ == NULL;
}

void alt_buf_lock_t::snapshot_subtree() {
    guarantee(!empty());
    // RSI: Actually implement this.
}

repli_timestamp_t alt_buf_lock_t::get_recency() const {
    guarantee(!empty());
    return current_page_acq_->recency();
}

alt_buf_read_t::alt_buf_read_t(alt_buf_lock_t *lock)
    : lock_(lock) {
    guarantee(!lock_->empty());
}

alt_buf_read_t::~alt_buf_read_t() {
    guarantee(!lock_->empty());
}

const void *alt_buf_read_t::get_data_read(uint32_t *block_size_out) {
    guarantee(!lock_->empty());
    lock_->current_page_acq_->read_acq_signal()->wait();
    page_t *page = lock_->current_page_acq_->current_page_for_read();
    guarantee(!lock_->empty());
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache_->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    *block_size_out = page_acq_.get_buf_size();
    return page_acq_.get_buf_read();
}

alt_buf_write_t::alt_buf_write_t(alt_buf_lock_t *lock)
    : lock_(lock) {
    guarantee(!lock_->empty());
}

alt_buf_write_t::~alt_buf_write_t() {
    guarantee(!lock_->empty());
}

void *alt_buf_write_t::get_data_write(uint32_t block_size) {
    guarantee(!lock_->empty());
    // RSI: Use block_size somehow.
    (void)block_size;
    lock_->current_page_acq_->write_acq_signal()->wait();
    guarantee(!lock_->empty());
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
