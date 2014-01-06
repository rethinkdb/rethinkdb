#include "buffer_cache/alt/alt.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/auto_drainer.hpp"

// RSI: get rid of this.
#define ALT_DEBUG 0

namespace alt {

alt_cache_t::alt_cache_t(serializer_t *serializer)
    : tracker_(),
      page_cache_(serializer, &tracker_),
      drainer_(make_scoped<auto_drainer_t>()) { }

alt_cache_t::~alt_cache_t() {
    drainer_.reset();
}

block_size_t alt_cache_t::max_block_size() const {
    return page_cache_.max_block_size();
}

alt_inner_txn_t::alt_inner_txn_t(alt_cache_t *cache, alt_inner_txn_t *preceding_txn)
    : cache_(cache),
      page_txn_(&cache->page_cache_,
                preceding_txn == NULL ? NULL : &preceding_txn->page_txn_),
      this_txn_timestamp_(repli_timestamp_t::invalid) {
    // RSI: Use this_txn_timestamp_ for something.
    (void)this_txn_timestamp_;
}

alt_inner_txn_t::~alt_inner_txn_t() {
    // RSI: Do anything?
}

alt_txn_t::alt_txn_t(alt_cache_t *cache,
                     alt_txn_t *preceding_txn)
    : durability_(write_durability_t::HARD) {
    inner_.init(new alt_inner_txn_t(cache,
                                    preceding_txn == NULL ? NULL
                                    : preceding_txn->inner_.get()));
}

alt_txn_t::alt_txn_t(alt_cache_t *cache,
                     write_durability_t durability,
                     alt_txn_t *preceding_txn)
    : durability_(durability) {
    inner_.init(new alt_inner_txn_t(cache,
                                    preceding_txn == NULL ? NULL
                                    : preceding_txn->inner_.get()));
}

void destroy_inner_txn(alt_inner_txn_t *inner, auto_drainer_t::lock_t) {
    delete inner;
}

alt_txn_t::~alt_txn_t() {
    if (durability_ == write_durability_t::SOFT) {
        alt_inner_txn_t *inner = inner_.release();
        coro_t::spawn_sometime(std::bind(&destroy_inner_txn,
                                         inner,
                                         inner->cache()->drainer_->lock()));
    }
}

alt_buf_lock_t::alt_buf_lock_t()
    : txn_(NULL),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
}

#if ALT_DEBUG
const char *show(alt_access_t access) {
    return access == alt_access_t::read ? "read" : "write";
}
#endif

alt_buf_lock_t::alt_buf_lock_t(alt_buf_parent_t parent,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    alt_buf_lock_t::wait_for_parent(parent, access);
    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id, access));
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p %s %lu\n", cache(), this, show(access), block_id);
#endif
}

alt_buf_lock_t::alt_buf_lock_t(alt_txn_t *txn,
                               block_id_t block_id,
                               alt_create_t create)
    : txn_(txn),
      current_page_acq_(new current_page_acq_t(txn->page_txn(), block_id,
                                               alt_access_t::write, true)),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    guarantee(create == alt_create_t::create);
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p create %lu\n", cache(), this, block_id);
#endif
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_parent_t parent,
                               block_id_t block_id,
                               alt_create_t create)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0)
{
    guarantee(create == alt_create_t::create);
    alt_buf_lock_t::wait_for_parent(parent, alt_access_t::write);
    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id,
                                                  alt_access_t::write, true));
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
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {

    alt_buf_lock_t::wait_for_parent(alt_buf_parent_t(parent), access);

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id, access));
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p %s %lu\n", cache(), this, show(access), block_id);
#endif
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_parent_t parent,
                               alt_create_t create)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    guarantee(create == alt_create_t::create);
    wait_for_parent(parent, alt_access_t::write);
    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  alt_access_t::write));
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p create %lu\n", cache(), this, block_id());
#endif
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               alt_create_t create)
    : txn_(parent->txn_),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    guarantee(create == alt_create_t::create);
    wait_for_parent(alt_buf_parent_t(parent), alt_access_t::write);

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  alt_access_t::write));
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p create %lu\n", cache(), this, block_id());
#endif
}

alt_buf_lock_t::~alt_buf_lock_t() {
#if ALT_DEBUG
    if (txn_ != NULL) {
        debugf("%p: alt_buf_lock_t %p destroy %lu\n", cache(), this, block_id());
    }
#endif
    // RSI: We'll have to do something with snapshot_node_ here.
    (void)snapshot_node_;
    guarantee(access_ref_count_ == 0);
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t &&movee)
    : txn_(movee.txn_),
      current_page_acq_(std::move(movee.current_page_acq_)),
      snapshot_node_(movee.snapshot_node_),
      access_ref_count_(0)
{
    guarantee(movee.access_ref_count_ == 0);
    movee.txn_ = NULL;
    movee.current_page_acq_.reset();
    movee.snapshot_node_ = NULL;
}

alt_buf_lock_t &alt_buf_lock_t::operator=(alt_buf_lock_t &&movee) {
    guarantee(access_ref_count_ == 0);
    alt_buf_lock_t tmp(std::move(movee));
    swap(tmp);
    return *this;
}

void alt_buf_lock_t::swap(alt_buf_lock_t &other) {
    guarantee(access_ref_count_ == 0);
    guarantee(other.access_ref_count_ == 0);
    std::swap(txn_, other.txn_);
    current_page_acq_.swap(other.current_page_acq_);
    std::swap(snapshot_node_, other.snapshot_node_);
}

void alt_buf_lock_t::reset_buf_lock() {
    alt_buf_lock_t tmp;
    swap(tmp);
}

void alt_buf_lock_t::snapshot_subtree() {
    guarantee(!empty());
    // RSI: Actually implement this.
}

void alt_buf_lock_t::detach_child(block_id_t child_id) {
    (void)child_id;
    guarantee(!empty());
    // RSI: Actually implement this.
}

void alt_buf_lock_t::reduce_to_readonly() {
    guarantee(!empty());
    // RSI: Actually implement this.
}

void alt_buf_lock_t::reduce_to_nothing() {
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
    lock_->access_ref_count_++;
}

alt_buf_read_t::~alt_buf_read_t() {
    guarantee(!lock_->empty());
    lock_->access_ref_count_--;
}

const void *alt_buf_read_t::get_data_read(uint32_t *block_size_out) {
    guarantee(!lock_->empty());
    lock_->current_page_acq_->read_acq_signal()->wait();
    page_t *page = lock_->current_page_acq_->current_page_for_read();
    guarantee(!lock_->empty());
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache()->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    *block_size_out = page_acq_.get_buf_size();
    lock_->cache()->checker.check(lock_->block_id(), page_acq_.get_buf_read(), *block_size_out);
    return page_acq_.get_buf_read();
}

alt_buf_write_t::alt_buf_write_t(alt_buf_lock_t *lock)
    : lock_(lock) {
    guarantee(!lock_->empty());
    lock_->access_ref_count_++;
}

alt_buf_write_t::~alt_buf_write_t() {
    guarantee(!lock_->empty());
    lock_->cache()->checker.set(lock_->block_id(), get_data_write(0), 4096);
    lock_->access_ref_count_--;
}

void *alt_buf_write_t::get_data_write(uint32_t block_size) {
    guarantee(!lock_->empty());
    // RSI: Use block_size somehow.
    (void)block_size;
    lock_->current_page_acq_->write_acq_signal()->wait();
    guarantee(!lock_->empty());
    page_t *page = lock_->current_page_acq_->current_page_for_write();
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache()->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    return page_acq_.get_buf_write();
}

void *alt_buf_write_t::get_data_write() {
    return get_data_write(lock_->cache()->max_block_size().value());
}


}  // namespace alt
