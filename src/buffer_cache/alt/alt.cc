#include "buffer_cache/alt/alt.hpp"

#include "arch/runtime/coroutines.hpp"
#include "concurrency/auto_drainer.hpp"

// RSI: get rid of this.
#define ALT_DEBUG 0

// RSI: Add ASSERT_FINITE_CORO_WAITING or ASSERT_NO_CORO_WAITING wherever we can.

namespace alt {

alt_memory_tracker_t::alt_memory_tracker_t()
    : semaphore_(200) { }
alt_memory_tracker_t::~alt_memory_tracker_t() { }

void alt_memory_tracker_t::inform_memory_change(UNUSED uint64_t in_memory_size,
                                                UNUSED uint64_t memory_limit) {
    // RSI: implement this.
}

// RSI: An interface problem here is that this is measured in blocks while
// inform_memory_change is measured in bytes.
void alt_memory_tracker_t::begin_txn_or_throttle(int64_t expected_change_count) {
    semaphore_.co_lock(expected_change_count);
    // RSI: _Really_ implement this.
}

void alt_memory_tracker_t::end_txn(int64_t saved_expected_change_count) {
    // RSI: _Really_ implement this.
    semaphore_.unlock(saved_expected_change_count);
}


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

alt_snapshot_node_t *alt_cache_t::latest_snapshot_node(block_id_t block_id) {
    if (block_id < snapshot_nodes_by_block_id_.size()) {
        return snapshot_nodes_by_block_id_[block_id].tail();
    } else {
        return NULL;
    }
}

void alt_cache_t::push_latest_snapshot_node(block_id_t block_id,
                                            alt_snapshot_node_t *node) {
    snapshot_nodes_by_block_id_[block_id].push_back(node);
}

void alt_cache_t::remove_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node) {
    snapshot_nodes_by_block_id_[block_id].remove(node);
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
                     write_durability_t durability,
                     int64_t expected_change_count,
                     alt_txn_t *preceding_txn)
    : durability_(durability),
      saved_expected_change_count_(expected_change_count) {
    cache->assert_thread();
    cache->tracker_.begin_txn_or_throttle(expected_change_count);
    inner_.init(new alt_inner_txn_t(cache,
                                    preceding_txn == NULL ? NULL
                                    : preceding_txn->inner_.get()));
}

void alt_txn_t::destroy_inner_txn(alt_inner_txn_t *inner, alt_cache_t *cache,
                                  int64_t saved_expected_change_count,
                                  auto_drainer_t::lock_t) {
    delete inner;
    cache->tracker_.end_txn(saved_expected_change_count);
}

alt_txn_t::~alt_txn_t() {
    alt_cache_t *cache = inner_->cache();
    cache->assert_thread();
    alt_inner_txn_t *inner = inner_.release();
    if (durability_ == write_durability_t::SOFT) {
        coro_t::spawn_sometime(std::bind(&alt_txn_t::destroy_inner_txn,
                                         inner,
                                         cache,
                                         saved_expected_change_count_,
                                         cache->drainer_->lock()));
    } else {
        alt_txn_t::destroy_inner_txn(inner, cache, saved_expected_change_count_,
                                     cache->drainer_->lock());
    }
}

alt_snapshot_node_t::alt_snapshot_node_t(scoped_ptr_t<current_page_acq_t> &&acq)
    : current_page_acq_(std::move(acq)), ref_count_(0) { }

alt_snapshot_node_t::~alt_snapshot_node_t() {
    // RSI: Other guarantees to make here?
    guarantee(ref_count_ == 0);
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
    guarantee(access_ref_count_ == 0);

    if (snapshot_node_ != NULL) {
        // RSI: This will surely be duplicated elsewhere (the other place we reduce the
        // refcount).
        --snapshot_node_->ref_count_;
        if (snapshot_node_->ref_count_ == 0) {
            cache()->remove_snapshot_node(block_id(),
                                          snapshot_node_);
        }
    }
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

// RSI: Rename this to snapshot_subdag.
void alt_buf_lock_t::snapshot_subtree() {
    guarantee(!empty());
    if (snapshot_node_ != NULL) {
        return;
    }

    // Wait for read access.  Not waiting for read access would make subdag
    // snapshotting more difficult to implement.
    read_acq_signal()->wait();

    alt_snapshot_node_t *latest_node = cache()->latest_snapshot_node(block_id());

    rassert(latest_node == NULL
            || latest_node->current_page_acq_->block_version() <= current_page_acq_->block_version());
    if (latest_node != NULL
        && latest_node->current_page_acq_->block_version() == current_page_acq_->block_version()) {
        snapshot_node_ = latest_node;
        ++snapshot_node_->ref_count_;
    } else {
        ASSERT_FINITE_CORO_WAITING;
        // RSI: There's gotta be a more encapsulated way to do this.
        alt_snapshot_node_t *node
            = new alt_snapshot_node_t(std::move(current_page_acq_));
        rassert(node->ref_count_ == 0);
        ++node->ref_count_;
        cache()->push_latest_snapshot_node(block_id(), node);
        snapshot_node_ = node;

        current_page_acq_.reset();
    }

    // Snapshotting's set up, so we can now declare our hold on the block to be
    // snapshotted.  Or no, we can just release our hold on the block, since we never
    // use current_page_acq_ again -- snapshotted alt_buf_lock_t's just use the
    // snapshot node.
    // RSI: We do use declare_snapshotted in the page cache -- but is it overengineered?
    current_page_acq_.reset();
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
    // RSI: Actually implement this.  Or don't, if nobody uses it.
}

repli_timestamp_t alt_buf_lock_t::get_recency() const {
    guarantee(!empty());
    if (snapshot_node_ != NULL) {
        // RSI: dedup these branches?
        return snapshot_node_->current_page_acq_->recency();
    } else {
        return current_page_acq_->recency();
    }
}

page_t *alt_buf_lock_t::get_held_page_for_read() {
    guarantee(!empty());
    if (snapshot_node_ != NULL) {
        // RSI: dedup these branches?
        snapshot_node_->current_page_acq_->read_acq_signal()->wait();
        guarantee(!empty());
        return snapshot_node_->current_page_acq_->current_page_for_read();
    } else {
        current_page_acq_->read_acq_signal()->wait();
        guarantee(!empty());
        return current_page_acq_->current_page_for_read();
    }
}

page_t *alt_buf_lock_t::get_held_page_for_write() {
    guarantee(!empty());
    rassert(snapshot_node_ == NULL);
    current_page_acq_->write_acq_signal()->wait();
    guarantee(!empty());
    return current_page_acq_->current_page_for_write();
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
    page_t *page = lock_->get_held_page_for_read();
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache()->page_cache_);
    }
    page_acq_.buf_ready_signal()->wait();
    *block_size_out = page_acq_.get_buf_size();
    return page_acq_.get_buf_read();
}

alt_buf_write_t::alt_buf_write_t(alt_buf_lock_t *lock)
    : lock_(lock) {
    guarantee(!lock_->empty());
    lock_->access_ref_count_++;
}

alt_buf_write_t::~alt_buf_write_t() {
    guarantee(!lock_->empty());
    lock_->access_ref_count_--;
}

void *alt_buf_write_t::get_data_write(uint32_t block_size) {
    // RSI: Use block_size somehow.
    (void)block_size;
    page_t *page = lock_->get_held_page_for_write();
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
