#include "buffer_cache/alt/alt.hpp"

#include <stack>

#include "arch/types.hpp"
#include "arch/runtime/coroutines.hpp"
#include "buffer_cache/alt/stats.hpp"
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

// RSI: Use the perfmon_collection.
alt_cache_t::alt_cache_t(serializer_t *serializer, const alt_cache_config_t &config,
                         perfmon_collection_t *perfmon_collection)
    : stats_(make_scoped<alt_cache_stats_t>(perfmon_collection)),
      tracker_(),
      page_cache_(serializer, config.page_config, &tracker_),
      drainer_(make_scoped<auto_drainer_t>()) { }

alt_cache_t::~alt_cache_t() {
    drainer_.reset();
}

block_size_t alt_cache_t::max_block_size() const {
    return page_cache_.max_block_size();
}

void alt_cache_t::create_cache_account(int priority,
                                       scoped_ptr_t<alt_cache_account_t> *out) {
    page_cache_.create_cache_account(priority, out);
}

alt_snapshot_node_t *
alt_cache_t::matching_snapshot_node_or_null(block_id_t block_id,
                                            block_version_t block_version) {
    intrusive_list_t<alt_snapshot_node_t> *list
        = &snapshot_nodes_by_block_id_[block_id];
    for (alt_snapshot_node_t *p = list->tail(); p != NULL; p = list->prev(p)) {
        if (p->current_page_acq_->block_version() == block_version) {
            return p;
        }
    }
    return NULL;
}

void alt_cache_t::add_snapshot_node(block_id_t block_id,
                                     alt_snapshot_node_t *node) {
    snapshot_nodes_by_block_id_[block_id].push_back(node);
}

void alt_cache_t::remove_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node) {
    // In some hypothetical cache data structure (a disk backed queue) we could have
    // a long linked list of snapshot nodes.  So we avoid _recursively_ removing
    // snapshot nodes.

    // Nodes to be deleted.
    std::stack<std::pair<block_id_t, alt_snapshot_node_t *> > stack;
    stack.push(std::make_pair(block_id, node));

    while (!stack.empty()) {
        auto pair = stack.top();
        stack.pop();
        // Step 1. Remove the node to be deleted from its list in
        // snapshot_nodes_by_block_id_.
        snapshot_nodes_by_block_id_[pair.first].remove(pair.second);

        const std::map<block_id_t, alt_snapshot_node_t *> children
            = std::move(pair.second->children_);
        // Step 2. Destroy the node.
        delete pair.second;

        // Step 3. Take its children and reduce their reference count, readying them
        // for deletion if necessary.
        for (auto it = children.begin(); it != children.end(); ++it) {
#if ALT_DEBUG
            debugf("decring child %p from parent %p (in %p)\n",
                   it->second, pair.second, this);
#endif
            if (it->second != NULL) {
                --it->second->ref_count_;
                if (it->second->ref_count_ == 0) {
#if ALT_DEBUG
                    debugf("removing child %p from parent %p (in %p)\n",
                           it->second, pair.second, this);
#endif
                    stack.push(*it);
                }
            }
        }
    }
}

alt_inner_txn_t::alt_inner_txn_t(alt_cache_t *cache,
                                 repli_timestamp_t txn_recency,
                                 alt_inner_txn_t *preceding_txn)
    : cache_(cache),
      page_txn_(&cache->page_cache_,
                txn_recency,
                preceding_txn == NULL ? NULL : &preceding_txn->page_txn_) { }

alt_inner_txn_t::~alt_inner_txn_t() {
    // RSI: Do anything?
}

alt_cache_account_t::alt_cache_account_t(threadnum_t thread, file_account_t *io_account)
    : thread_(thread), io_account_(io_account) { }

alt_cache_account_t::~alt_cache_account_t() {
    on_thread_t thread_switcher(thread_);
    delete io_account_;
}

alt_txn_t::alt_txn_t(alt_cache_t *cache,
                     UNUSED alt_read_access_t read_access,
                     alt_txn_t *preceding_txn)
    : access_(alt_access_t::read),
      durability_(write_durability_t::SOFT),  // A B.S. value.  RSI: use valgrind_undefined
      saved_expected_change_count_(0) {
    // RSI: We could dedup the constructor body a bit.
    cache->assert_thread();
    // RSI: If I remember correctly the mirrored cache used 1 for some reason.
    cache->tracker_.begin_txn_or_throttle(saved_expected_change_count_);
    inner_.init(new alt_inner_txn_t(cache,
                                    repli_timestamp_t::invalid,
                                    preceding_txn == NULL ? NULL
                                    : preceding_txn->inner_.get()));

}

alt_txn_t::alt_txn_t(alt_cache_t *cache,
                     write_durability_t durability,
                     repli_timestamp_t txn_timestamp,
                     int64_t expected_change_count,
                     alt_txn_t *preceding_txn)
    : access_(alt_access_t::write),
      durability_(durability),
      saved_expected_change_count_(expected_change_count) {
    cache->assert_thread();
    cache->tracker_.begin_txn_or_throttle(expected_change_count);
    inner_.init(new alt_inner_txn_t(cache,
                                    txn_timestamp,
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

void alt_txn_t::set_account(alt_cache_account_t *cache_account) {
    inner_->page_txn()->set_account(cache_account);
}


alt_snapshot_node_t::alt_snapshot_node_t(scoped_ptr_t<current_page_acq_t> &&acq)
    : current_page_acq_(std::move(acq)), ref_count_(0) { }

alt_snapshot_node_t::~alt_snapshot_node_t() {
    // The only thing that deletes an alt_snapshot_node_t should be the
    // remove_snapshot_node function.
    rassert(ref_count_ == 0);
    rassert(current_page_acq_.has());
    rassert(children_.empty());
}

alt_buf_lock_t::alt_buf_lock_t()
    : txn_(NULL),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0),
      was_destroyed_(false) {
}

#if ALT_DEBUG
const char *show(alt_access_t access) {
    return access == alt_access_t::read ? "read" : "write";
}
#endif

// RSI: Add general explanation of correctness of snapshotting logic.  Explain why
// it's correct for making a child snapshot node to just construct a
// current_page_acq_t, and how there could not possibly be an intervening write
// transaction that we need to jump ahead of.

alt_snapshot_node_t *
alt_buf_lock_t::get_or_create_child_snapshot_node(alt_cache_t *cache,
                                                  alt_snapshot_node_t *parent,
                                                  block_id_t child_id) {
    auto it = parent->children_.find(child_id);
    if (it == parent->children_.end()) {
        // RSI: We could check snapshot_nodes_by_block_id_[child_id] here?  First see
        // if the version would change.
        auto acq = make_scoped<current_page_acq_t>(&cache->page_cache_,
                                                   child_id,
                                                   alt_read_access_t::read);
        acq->declare_snapshotted();
        alt_snapshot_node_t *child = new alt_snapshot_node_t(std::move(acq));
        rassert(child->ref_count_ == 0);
        cache->add_snapshot_node(child_id, child);
        child->ref_count_++;
        parent->children_.insert(std::make_pair(child_id, child));
        return child;
    } else {
        return it->second;
    }
}

void alt_buf_lock_t::create_child_snapshot_nodes(alt_cache_t *cache,
                                                 block_version_t parent_version,
                                                 block_id_t parent_id,
                                                 block_id_t child_id) {
    // We create at most one child snapshot node.

    alt_snapshot_node_t *child = NULL;
    intrusive_list_t<alt_snapshot_node_t> *list
        = &cache->snapshot_nodes_by_block_id_[parent_id];
    for (alt_snapshot_node_t *p = list->tail(); p != NULL; p = list->prev(p)) {
        auto it = p->children_.find(child_id);
        if (it != p->children_.end()) {
            // RSI: Can we kick nodes with children out of this list?
            // Already has a child, continue.
            continue;
        }
        if (p->current_page_acq_->block_version() >= parent_version) {
            // Version of snapshot node is _after_ the parent's version.
            continue;
        }

        if (child == NULL) {
            // RSI: We could check snapshot_nodes_by_block_id_[child_id] here?
            // Dedup with get_or_create_child_snapshot_node, too.
            auto acq = make_scoped<current_page_acq_t>(&cache->page_cache_, child_id,
                                                       alt_read_access_t::read);
            acq->declare_snapshotted();
            child = new alt_snapshot_node_t(std::move(acq));
            cache->add_snapshot_node(child_id, child);
        }

        child->ref_count_++;
        p->children_.insert(std::make_pair(child_id, child));
    }
}

// Puts markers in the parent saying that no such child exists.
void alt_buf_lock_t::create_empty_child_snapshot_nodes(alt_cache_t *cache,
                                                       block_version_t parent_version,
                                                       block_id_t parent_id,
                                                       block_id_t child_id) {
    intrusive_list_t<alt_snapshot_node_t> *list
        = &cache->snapshot_nodes_by_block_id_[parent_id];

    for (alt_snapshot_node_t *p = list->tail(); p != NULL; p = list->prev(p)) {
        auto it = p->children_.find(child_id);
        if (it != p->children_.end()) {
            // Already has a child, continue.
            continue;
        }
        if (p->current_page_acq_->block_version() >= parent_version) {
            // Version of snapshot node is _after_ the parent's version.
            continue;
        }

        p->children_.insert(std::make_pair(child_id,
                                           static_cast<alt_snapshot_node_t *>(NULL)));
    }
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_parent_t parent,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0),
      was_destroyed_(false) {
    alt_buf_lock_t::wait_for_parent(parent, access);
    if (parent.lock_or_null_ != NULL && parent.lock_or_null_->snapshot_node_ != NULL) {
        alt_buf_lock_t *parent_lock = parent.lock_or_null_;
        rassert(!parent_lock->current_page_acq_.has());
        snapshot_node_
            = get_or_create_child_snapshot_node(txn_->cache(),
                                                parent_lock->snapshot_node_, block_id);
        guarantee(snapshot_node_ != NULL,
                  "Tried to acquire (in cache %p) a deleted block (%" PRIu64
                  " as child of %" PRIu64 ") (with read access).",
                  txn_->cache(),
                  block_id, parent_lock->block_id());
        ++snapshot_node_->ref_count_;
    } else {
        if (access == alt_access_t::write && parent.lock_or_null_ != NULL) {
            create_child_snapshot_nodes(txn_->cache(),
                                        parent.lock_or_null_->current_page_acq()->block_version(),
                                        parent.lock_or_null_->block_id(),
                                        block_id);
        }
        current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id, access));
    }

#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p %s %lu\n", cache(), this, show(access), block_id);
#endif
}

bool is_subordinate(alt_access_t parent, alt_access_t child) {
    return parent == alt_access_t::write || child == alt_access_t::read;
}

alt_buf_lock_t::alt_buf_lock_t(alt_txn_t *txn,
                               block_id_t block_id,
                               UNUSED alt_create_t create)
    : txn_(txn),
      current_page_acq_(new current_page_acq_t(txn->page_txn(), block_id,
                                               alt_access_t::write, true)),
      snapshot_node_(NULL),
      access_ref_count_(0),
      was_destroyed_(false) {
    guarantee(is_subordinate(txn_->access(), alt_access_t::write));
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p create %lu\n", cache(), this, block_id);
#endif
}

void alt_buf_lock_t::mark_deleted() {
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p delete %lu\n", cache(), this, block_id());
#endif
    guarantee(!empty());
    current_page_acq()->mark_deleted();
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
    } else {
        guarantee(is_subordinate(parent.txn()->access(), access));
    }
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               block_id_t block_id,
                               alt_access_t access)
    : txn_(parent->txn_),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0),
      was_destroyed_(false) {
    // This implementation should be identical to the alt_buf_parent_t version of
    // this constructor.  (Unfortunately, we support compilers that lack full C++11
    // support.)

    alt_buf_lock_t::wait_for_parent(alt_buf_parent_t(parent), access);

    if (parent->snapshot_node_ != NULL) {
        rassert(!parent->current_page_acq_.has());
        snapshot_node_
            = get_or_create_child_snapshot_node(txn_->cache(),
                                                parent->snapshot_node_, block_id);
        guarantee(snapshot_node_ != NULL,
                  "Tried to acquire a deleted block (with read access.");
        ++snapshot_node_->ref_count_;
    } else {
        if (access == alt_access_t::write) {
            create_child_snapshot_nodes(txn_->cache(),
                                        parent->current_page_acq()->block_version(),
                                        parent->block_id(), block_id);
        }
        current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id,
                                                      access));
    }

#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p %s %lu\n", cache(), this, show(access), block_id);
#endif
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_parent_t parent,
                               UNUSED alt_create_t create)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0),
      was_destroyed_(false) {
    alt_buf_lock_t::wait_for_parent(parent, alt_access_t::write);

    // Makes sure nothing funny can happen in current_page_acq_t constructor.
    // Otherwise, we'd need to make choosing a block id a separate function, and call
    // create_empty_child_snapshot_nodes before constructing the current_page_acq_t.
    // RSI: Probably we should do that anyway.
    ASSERT_FINITE_CORO_WAITING;

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  alt_create_t::create));

    if (parent.lock_or_null_ != NULL) {
        create_empty_child_snapshot_nodes(txn_->cache(),
                                          parent.lock_or_null_->current_page_acq()->block_version(),
                                          parent.lock_or_null_->block_id(),
                                          current_page_acq_->block_id());
#if ALT_DEBUG
        debugf("%p: alt_buf_lock_t %p create %lu (as child of %lu)\n",
               cache(), this, block_id(), parent.lock_or_null_->block_id());
#endif
    } else {
#if ALT_DEBUG
        debugf("%p: alt_buf_lock_t %p create %lu (no parent)\n",
               cache(), this, block_id());
#endif
    }

}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t *parent,
                               UNUSED alt_create_t create)
    : txn_(parent->txn_),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0),
      was_destroyed_(false) {
    alt_buf_lock_t::wait_for_parent(alt_buf_parent_t(parent), alt_access_t::write);

    // Makes sure nothing funny can happen in current_page_acq_t constructor.
    // Otherwise, we'd need to make choosing a block id a separate function, and call
    // create_empty_child_snapshot_nodes before constructing the current_page_acq_t.
    // RSI: Probably we should do that anyway.
    ASSERT_FINITE_CORO_WAITING;

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  alt_create_t::create));

    create_empty_child_snapshot_nodes(txn_->cache(),
                                      parent->current_page_acq()->block_version(),
                                      parent->block_id(),
                                      current_page_acq_->block_id());

#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p create %lu\n", cache(), this, block_id());
#endif
}

alt_buf_lock_t::~alt_buf_lock_t() {
    guarantee(!was_destroyed_);
    was_destroyed_ = true;
#if ALT_DEBUG
    if (txn_ != NULL) {
        debugf("%p: alt_buf_lock_t %p destroy %lu\n", cache(), this, block_id());
    }
#endif
    guarantee(access_ref_count_ == 0);

    if (snapshot_node_ != NULL) {
        --snapshot_node_->ref_count_;
        if (snapshot_node_->ref_count_ == 0) {
#if ALT_DEBUG
            debugf("remove_snapshot_node %p by %p (in %p)\n",
                   snapshot_node_, this, cache());
#endif
            cache()->remove_snapshot_node(block_id(),
                                          snapshot_node_);
        }
    }
}

alt_buf_lock_t::alt_buf_lock_t(alt_buf_lock_t &&movee)
    : txn_(movee.txn_),
      current_page_acq_(std::move(movee.current_page_acq_)),
      snapshot_node_(movee.snapshot_node_),
      access_ref_count_(0),
      was_destroyed_(false) {
    guarantee(!movee.was_destroyed_);
    guarantee(movee.access_ref_count_ == 0);
    movee.txn_ = NULL;
    movee.current_page_acq_.reset();
    movee.snapshot_node_ = NULL;
}

alt_buf_lock_t &alt_buf_lock_t::operator=(alt_buf_lock_t &&movee) {
    guarantee(!movee.was_destroyed_);
    guarantee(!was_destroyed_);
    guarantee(access_ref_count_ == 0);
    alt_buf_lock_t tmp(std::move(movee));
    swap(tmp);
    return *this;
}

void alt_buf_lock_t::swap(alt_buf_lock_t &other) {
    guarantee(!was_destroyed_);
    guarantee(!other.was_destroyed_);
    guarantee(access_ref_count_ == 0);
    guarantee(other.access_ref_count_ == 0);
    std::swap(txn_, other.txn_);
    current_page_acq_.swap(other.current_page_acq_);
    std::swap(snapshot_node_, other.snapshot_node_);
}

void alt_buf_lock_t::reset_buf_lock() {
    guarantee(!was_destroyed_);
    alt_buf_lock_t tmp;
    swap(tmp);
}

// RSI: Rename this to snapshot_subdag.
void alt_buf_lock_t::snapshot_subtree() {
#if ALT_DEBUG
    debugf("%p: alt_buf_lock_t %p snapshot %lu\n", cache(), this, block_id());
#endif
    guarantee(!was_destroyed_);
    // RSI: Can this be ASSERT_NO_CORO_WAITING?
    ASSERT_FINITE_CORO_WAITING;
    guarantee(!empty());
    if (snapshot_node_ != NULL) {
        return;
    }

    alt_snapshot_node_t *matching_node
        = cache()->matching_snapshot_node_or_null(block_id(),
                                                  current_page_acq_->block_version());

    if (matching_node != NULL) {
        snapshot_node_ = matching_node;
        ++matching_node->ref_count_;
    } else {
        const block_id_t block_id = current_page_acq_->block_id();
        alt_snapshot_node_t *node
            = new alt_snapshot_node_t(std::move(current_page_acq_));
        rassert(node->ref_count_ == 0);
        ++node->ref_count_;
        txn_->cache()->add_snapshot_node(block_id, node);
        snapshot_node_ = node;
        node->current_page_acq_->declare_snapshotted();
    }

    // Our hold on the block now uses snapshot_node_, not current_page_acq_.
    current_page_acq_.reset();
}

current_page_acq_t *alt_buf_lock_t::current_page_acq() const {
    guarantee(!empty());
    if (snapshot_node_ != NULL) {
        return snapshot_node_->current_page_acq_.get();
    } else {
        return current_page_acq_.get();
    }
}

void alt_buf_lock_t::detach_child(block_id_t child_id) {
    guarantee(!empty());
    guarantee(access() == alt_access_t::write);

    alt_buf_lock_t::create_child_snapshot_nodes(cache(),
                                                current_page_acq()->block_version(),
                                                block_id(),
                                                child_id);
}

repli_timestamp_t alt_buf_lock_t::get_recency() const {
    guarantee(!empty());
    return current_page_acq()->recency();
}

page_t *alt_buf_lock_t::get_held_page_for_read() {
    guarantee(!empty());
    current_page_acq_t *cpa = current_page_acq();
    guarantee(cpa != NULL);
    cpa->read_acq_signal()->wait();
    guarantee(!empty());
    return cpa->current_page_for_read();
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
