#include "buffer_cache/alt.hpp"

#include <stack>

#include "arch/types.hpp"
#include "arch/runtime/coroutines.hpp"
#include "buffer_cache/stats.hpp"
#include "concurrency/auto_drainer.hpp"
#include "utils.hpp"

#define ALT_DEBUG 0

using alt::block_version_t;
using alt::current_page_acq_t;
using alt::page_acq_t;
using alt::page_cache_t;
using alt::page_t;
using alt::page_txn_t;
using alt::throttler_acq_t;

const int64_t MINIMUM_SOFT_UNWRITTEN_CHANGES_LIMIT = 1;
const int64_t SOFT_UNWRITTEN_CHANGES_LIMIT = 8000;
const double SOFT_UNWRITTEN_CHANGES_MEMORY_FRACTION = 0.5;

// In addition to the data blocks themselves, transactions that are not completely
// flushed yet consume memory for the index writes and general metadata. If
// there are a lot of soft durability transactions, these can accumulate and consume
// an increasing amount of RAM. Hence we limit the number of unwritten index
// updates in addition to the number of unwritten blocks. We scale that limit
// proportionally to the unwritten block changes limit
const int64_t INDEX_CHANGES_LIMIT_FACTOR = 5;

// There are very few ASSERT_NO_CORO_WAITING calls (instead we have
// ASSERT_FINITE_CORO_WAITING) because most of the time we're at the mercy of the
// page cache, which often may need to load or evict blocks, which may involve a
// spawn_now call.


// The intrusive list of alt_snapshot_node_t contains all the snapshot nodes for a
// given block id, in order by version. (See cache_t::snapshot_nodes_by_block_id_.)
class alt_snapshot_node_t : public intrusive_list_node_t<alt_snapshot_node_t> {
public:
    explicit alt_snapshot_node_t(scoped_ptr_t<current_page_acq_t> &&acq);
    ~alt_snapshot_node_t();

private:
    friend class buf_lock_t;
    friend class cache_t;

    // This is never null (and is always a current_page_acq_t that has had
    // declare_snapshotted() called).
    scoped_ptr_t<current_page_acq_t> current_page_acq_;

    // A NULL pointer associated with a block id indicates that the block is deleted.
    std::map<block_id_t, alt_snapshot_node_t *> children_;

    // The number of buf_lock_t's referring to this node, plus the number of
    // alt_snapshot_node_t's referring to this node (via its children_ vector).
    int64_t ref_count_;


    DISABLE_COPYING(alt_snapshot_node_t);
};

alt_txn_throttler_t::alt_txn_throttler_t(int64_t minimum_unwritten_changes_limit)
    : minimum_unwritten_changes_limit_(minimum_unwritten_changes_limit),
      unwritten_block_changes_semaphore_(SOFT_UNWRITTEN_CHANGES_LIMIT),
      unwritten_index_changes_semaphore_(
          SOFT_UNWRITTEN_CHANGES_LIMIT * INDEX_CHANGES_LIMIT_FACTOR) { }

alt_txn_throttler_t::~alt_txn_throttler_t() { }

throttler_acq_t alt_txn_throttler_t::begin_txn_or_throttle(int64_t expected_change_count) {
    throttler_acq_t acq;
    acq.index_changes_semaphore_acq_.init(
        &unwritten_index_changes_semaphore_,
        expected_change_count);
    acq.index_changes_semaphore_acq_.acquisition_signal()->wait();
    acq.block_changes_semaphore_acq_.init(
        &unwritten_block_changes_semaphore_,
        expected_change_count);
    acq.block_changes_semaphore_acq_.acquisition_signal()->wait();
    return acq;
}

void alt_txn_throttler_t::end_txn(UNUSED throttler_acq_t acq) {
    // Just let the acq destructor do its thing.
}

void alt_txn_throttler_t::inform_memory_limit_change(uint64_t memory_limit,
                                                     const block_size_t max_block_size) {
    int64_t throttler_limit = std::min<int64_t>(SOFT_UNWRITTEN_CHANGES_LIMIT,
        (memory_limit / max_block_size.ser_value()) * SOFT_UNWRITTEN_CHANGES_MEMORY_FRACTION);

    // Always provide at least one capacity in the semaphore
    throttler_limit = std::max<int64_t>(throttler_limit, minimum_unwritten_changes_limit_);

    unwritten_index_changes_semaphore_.set_capacity(
        throttler_limit * INDEX_CHANGES_LIMIT_FACTOR);
    unwritten_block_changes_semaphore_.set_capacity(throttler_limit);
}

cache_t::cache_t(serializer_t *serializer,
                 cache_balancer_t *balancer,
                 perfmon_collection_t *perfmon_collection)
    : throttler_(MINIMUM_SOFT_UNWRITTEN_CHANGES_LIMIT),
      page_cache_(serializer, balancer, &throttler_),
      stats_(make_scoped<alt_cache_stats_t>(&page_cache_, perfmon_collection)) { }

cache_t::~cache_t() {
    guarantee(snapshot_nodes_by_block_id_.empty());
}

cache_account_t cache_t::create_cache_account(int priority) {
    return page_cache_.create_cache_account(priority);
}

alt_snapshot_node_t *
cache_t::matching_snapshot_node_or_null(block_id_t block_id,
                                        block_version_t block_version) {
    ASSERT_NO_CORO_WAITING;
    auto list_it = snapshot_nodes_by_block_id_.find(block_id);
    if (list_it == snapshot_nodes_by_block_id_.end()) {
        return NULL;
    }
    intrusive_list_t<alt_snapshot_node_t> *list = &list_it->second;
    for (alt_snapshot_node_t *p = list->tail(); p != NULL; p = list->prev(p)) {
        if (p->current_page_acq_->block_version() == block_version) {
            return p;
        }
    }
    return NULL;
}

void cache_t::add_snapshot_node(block_id_t block_id,
                                alt_snapshot_node_t *node) {
    ASSERT_NO_CORO_WAITING;
    snapshot_nodes_by_block_id_[block_id].push_back(node);
}

void cache_t::remove_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node) {
    ASSERT_FINITE_CORO_WAITING;
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
        auto list_it = snapshot_nodes_by_block_id_.find(pair.first);
        rassert(list_it != snapshot_nodes_by_block_id_.end());
        list_it->second.remove(pair.second);
        if (list_it->second.empty()) {
            snapshot_nodes_by_block_id_.erase(list_it);
        }

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

txn_t::txn_t(cache_conn_t *cache_conn,
             read_access_t)
    : cache_(cache_conn->cache()),
      cache_account_(cache_->page_cache_.default_reads_account()),
      access_(access_t::read),
      durability_(write_durability_t::SOFT) {
    // Right now, cache_conn is only used to control flushing of write txns.  When we
    // need to support other cache_conn_t related features (like read operations
    // magically passing write operations), we'll need to do something fancier with
    // read txns on cache conns.
    help_construct(0, NULL);
}

txn_t::txn_t(cache_conn_t *cache_conn,
             write_durability_t durability,
             int64_t expected_change_count)
    : cache_(cache_conn->cache()),
      cache_account_(cache_->page_cache_.default_reads_account()),
      access_(access_t::write),
      durability_(durability) {

    help_construct(expected_change_count, cache_conn);
}

void txn_t::help_construct(int64_t expected_change_count,
                           cache_conn_t *cache_conn) {
    cache_->assert_thread();
    guarantee(expected_change_count >= 0);
    throttler_acq_t throttler_acq
        = cache_->throttler_.begin_txn_or_throttle(expected_change_count);

    ASSERT_FINITE_CORO_WAITING;

    page_txn_.init(new page_txn_t(&cache_->page_cache_,
                                  std::move(throttler_acq),
                                  cache_conn));
}

void txn_t::inform_tracker(cache_t *cache, throttler_acq_t *throttler_acq) {
    cache->throttler_.end_txn(std::move(*throttler_acq));
}

void txn_t::pulse_and_inform_tracker(cache_t *cache,
                                     throttler_acq_t *throttler_acq,
                                     cond_t *pulsee) {
    inform_tracker(cache, throttler_acq);
    pulsee->pulse();
}

txn_t::~txn_t() {
    cache_->assert_thread();

    if (durability_ == write_durability_t::SOFT) {
        cache_->page_cache_.flush_and_destroy_txn(std::move(page_txn_),
                                                  std::bind(&txn_t::inform_tracker,
                                                            cache_,
                                                            ph::_1));
    } else {
        cond_t cond;
        cache_->page_cache_.flush_and_destroy_txn(
                std::move(page_txn_),
                std::bind(&txn_t::pulse_and_inform_tracker,
                          cache_, ph::_1, &cond));
        cond.wait();
    }
}

void txn_t::set_account(cache_account_t *cache_account) {
    cache_account_ = cache_account;
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

buf_lock_t::buf_lock_t()
    : txn_(NULL),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) { }

#if ALT_DEBUG
const char *show(access_t access) {
    return access == access_t::read ? "read" : "write";
}
#endif

alt_snapshot_node_t *buf_lock_t::help_make_child(cache_t *cache,
                                                 block_id_t child_id) {
    // KSI: This allocation is sometimes just unnecessary, right?
    auto acq = make_scoped<current_page_acq_t>(&cache->page_cache_, child_id,
                                               read_access_t::read);

    alt_snapshot_node_t *child
        = cache->matching_snapshot_node_or_null(child_id, acq->block_version());

    if (child != NULL) {
        acq.reset();
    } else {
        acq->declare_snapshotted();
        child = new alt_snapshot_node_t(std::move(acq));
        cache->add_snapshot_node(child_id, child);
    }
    return child;
}

// Returns a child snapshot node (using the current version of the block) for the
// specified child block id.
alt_snapshot_node_t *
buf_lock_t::get_or_create_child_snapshot_node(cache_t *cache,
                                              alt_snapshot_node_t *parent,
                                              block_id_t child_id) {
    ASSERT_FINITE_CORO_WAITING;
    auto it = parent->children_.find(child_id);
    if (it == parent->children_.end()) {
        // There's no child for the snapshot node for this child id.  That means a
        // write transaction has not yet acquired [1].  So we create a child using
        // the current version of the block.
        //
        // [1] assuming the cache is used properly, with the child always acquired
        // via the parent, or detached, before modification
        alt_snapshot_node_t *child = help_make_child(cache, child_id);

        // We don't attach the snapshot node to its parent, because this is a read
        // transaction.
        return child;
    } else {
        return it->second;
    }
}

// When a write transaction acquires a child of a parent node (for write), it needs
// to save the current value of the child buf and attach it to any of the parent's
// snapshot nodes (but only those that aren't newer than our acquisition of the
// parent [1]).
//
// [1] We might need to be careful about this because if you have two write
// acquisitions, W_X and W_Y of block B1 (where W_X acquires B1 before W_Y does),
// which then both acquire the child B2, you only want the child of acquisition W1 to
// attach to children of snapshot nodes that happened _before_ W_X and not after.
// This can happen -- the snapshot nodes that happen _after_ W_X wouldn't have
// acquired the block yet, but they'd exist, and they'd be in line, waiting for their
// turn.

void buf_lock_t::create_child_snapshot_attachments(cache_t *cache,
                                                   block_version_t parent_version,
                                                   block_id_t parent_id,
                                                   block_id_t child_id) {
    ASSERT_FINITE_CORO_WAITING;
    // We create at most one child snapshot node.

    alt_snapshot_node_t *child = NULL;
    auto list_it = cache->snapshot_nodes_by_block_id_.find(parent_id);
    if (list_it == cache->snapshot_nodes_by_block_id_.end()) {
        return;
    }
    for (alt_snapshot_node_t *p = list_it->second.tail();
         p != NULL;
         p = list_it->second.prev(p)) {
        auto it = p->children_.find(child_id);
        if (it != p->children_.end()) {
            // Already has a child, continue.
            continue;
        }
        if (p->current_page_acq_->block_version() >= parent_version) {
            // Version of snapshot node is _after_ the parent's version.
            continue;
        }

        if (child == NULL) {
            child = help_make_child(cache, child_id);
        }

        // Attach the child to its parent.
        child->ref_count_++;
        p->children_.insert(std::make_pair(child_id, child));
    }
}

// Puts markers in the parent saying that no such child exists.
void buf_lock_t::create_empty_child_snapshot_attachments(cache_t *cache,
                                                         block_version_t parent_version,
                                                         block_id_t parent_id,
                                                         block_id_t child_id) {
    ASSERT_NO_CORO_WAITING;

    auto list_it = cache->snapshot_nodes_by_block_id_.find(parent_id);
    if (list_it == cache->snapshot_nodes_by_block_id_.end()) {
        return;
    }
    for (alt_snapshot_node_t *p = list_it->second.tail();
         p != NULL;
         p = list_it->second.prev(p)) {
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

void buf_lock_t::help_construct(buf_parent_t parent, block_id_t block_id,
                                access_t access) {
    buf_lock_t::wait_for_parent(parent, access);
    ASSERT_FINITE_CORO_WAITING;
    if (parent.lock_or_null_ != NULL && parent.lock_or_null_->snapshot_node_ != NULL) {
        rassert(access == access_t::read);
        buf_lock_t *parent_lock = parent.lock_or_null_;
        rassert(!parent_lock->current_page_acq_.has());
        snapshot_node_
            = get_or_create_child_snapshot_node(txn_->cache(),
                                                parent_lock->snapshot_node_,
                                                block_id);
        guarantee(snapshot_node_ != NULL,
                  "Tried to acquire (in cache %p) a deleted block (%" PRIu64
                  " as child of %" PRIu64 ") (with read access).",
                  txn_->cache(),
                  block_id, parent_lock->block_id());
        ++snapshot_node_->ref_count_;
    } else {
        if (access == access_t::write && parent.lock_or_null_ != NULL) {
            create_child_snapshot_attachments(txn_->cache(),
                                              parent.lock_or_null_->current_page_acq()->block_version(),
                                              parent.lock_or_null_->block_id(),
                                              block_id);
        }
        current_page_acq_.init(new current_page_acq_t(txn_->page_txn(), block_id,
                                                      access));
    }

#if ALT_DEBUG
    debugf("%p: buf_lock_t %p %s %" PRIu64 "\n", cache(), this, show(access), block_id);
#endif
}

buf_lock_t::buf_lock_t(buf_parent_t parent,
                       block_id_t block_id,
                       access_t access)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    help_construct(parent, block_id, access);
}

buf_lock_t::buf_lock_t(buf_lock_t *parent,
                       block_id_t block_id,
                       access_t access)
    : txn_(parent->txn_),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    help_construct(buf_parent_t(parent), block_id, access);
}

bool is_subordinate(access_t parent, access_t child) {
    return parent == access_t::write || child == access_t::read;
}

void buf_lock_t::help_construct(buf_parent_t parent, block_id_t block_id,
                                alt_create_t) {
    buf_lock_t::wait_for_parent(parent, access_t::write);

    // Makes sure nothing funny can happen in current_page_acq_t constructor.
    // Otherwise, we'd need to make choosing a block id a separate function, and call
    // create_empty_child_snapshot_attachments before constructing the
    // current_page_acq_t.
    ASSERT_FINITE_CORO_WAITING;

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  block_id,
                                                  access_t::write,
                                                  alt::page_create_t::yes));

    if (parent.lock_or_null_ != NULL) {
        create_empty_child_snapshot_attachments(txn_->cache(),
                                          parent.lock_or_null_->current_page_acq()->block_version(),
                                          parent.lock_or_null_->block_id(),
                                          current_page_acq_->block_id());
#if ALT_DEBUG
        debugf("%p: buf_lock_t %p create %" PRIu64 " (as child of %" PRIu64 ")\n",
               cache(), this, buf_lock_t::block_id(), parent.lock_or_null_->block_id());
#endif
    } else {
#if ALT_DEBUG
        debugf("%p: buf_lock_t %p create %" PRIu64 " (no parent)\n",
               cache(), this, buf_lock_t::block_id());
#endif
    }
}

buf_lock_t::buf_lock_t(txn_t *txn,
                       block_id_t block_id,
                       alt_create_t create)
    : txn_(txn),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    help_construct(buf_parent_t(txn), block_id, create);
}

buf_lock_t::buf_lock_t(buf_parent_t parent,
                       block_id_t block_id,
                       alt_create_t create)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    help_construct(parent, block_id, create);
}

void buf_lock_t::mark_deleted() {
    ASSERT_FINITE_CORO_WAITING;
#if ALT_DEBUG
    debugf("%p: buf_lock_t %p delete %" PRIu64 "\n", cache(), this, block_id());
#endif
    guarantee(!empty());
    guarantee(current_page_acq()->write_acq_signal()->is_pulsed());
    current_page_acq()->mark_deleted();
}

void buf_lock_t::wait_for_parent(buf_parent_t parent, access_t access) {
    if (parent.lock_or_null_ != NULL) {
        buf_lock_t *lock = parent.lock_or_null_;
        guarantee(is_subordinate(lock->access(), access));
        if (access == access_t::write) {
            lock->write_acq_signal()->wait();
        } else {
            lock->read_acq_signal()->wait();
        }
    } else {
        guarantee(is_subordinate(parent.txn()->access(), access));
    }
}

void buf_lock_t::help_construct(
        buf_parent_t parent,
        alt_create_t,
        block_type_t block_type) {
    cache()->assert_thread();

    buf_lock_t::wait_for_parent(parent, access_t::write);

    // Makes sure nothing funny can happen in current_page_acq_t constructor.
    // Otherwise, we'd need to make choosing a block id a separate function, and call
    // create_empty_child_snapshot_attachments before constructing the
    // current_page_acq_t.
    ASSERT_FINITE_CORO_WAITING;

    current_page_acq_.init(new current_page_acq_t(txn_->page_txn(),
                                                  alt_create_t::create,
                                                  block_type));

    if (parent.lock_or_null_ != NULL) {
        create_empty_child_snapshot_attachments(txn_->cache(),
                                          parent.lock_or_null_->current_page_acq()->block_version(),
                                          parent.lock_or_null_->block_id(),
                                          current_page_acq_->block_id());
#if ALT_DEBUG
        debugf("%p: buf_lock_t %p create %" PRIu64 " (as child of %" PRIu64 ")\n",
               cache(), this, block_id(), parent.lock_or_null_->block_id());
#endif
    } else {
#if ALT_DEBUG
        debugf("%p: buf_lock_t %p create %" PRIu64 " (no parent)\n",
               cache(), this, block_id());
#endif
    }
}

buf_lock_t::buf_lock_t(buf_parent_t parent,
                       alt_create_t create,
                       block_type_t block_type)
    : txn_(parent.txn()),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    help_construct(parent, create, block_type);
}

buf_lock_t::buf_lock_t(buf_lock_t *parent,
                       alt_create_t create,
                       block_type_t block_type)
    : txn_(parent->txn_),
      current_page_acq_(),
      snapshot_node_(NULL),
      access_ref_count_(0) {
    help_construct(buf_parent_t(parent), create, block_type);
}

buf_lock_t::~buf_lock_t() {
    if (txn_ != NULL) {
        cache()->assert_thread();
#if ALT_DEBUG
        debugf("%p: buf_lock_t %p destroy %" PRIu64 "\n", cache(), this, block_id());
#endif
    }
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

buf_lock_t::buf_lock_t(buf_lock_t &&movee)
    : txn_(movee.txn_),
      current_page_acq_(std::move(movee.current_page_acq_)),
      snapshot_node_(movee.snapshot_node_),
      access_ref_count_(0) {
    guarantee(movee.access_ref_count_ == 0);
    movee.txn_ = NULL;
    movee.current_page_acq_.reset();
    movee.snapshot_node_ = NULL;
}

buf_lock_t &buf_lock_t::operator=(buf_lock_t &&movee) {
    guarantee(access_ref_count_ == 0);
    buf_lock_t tmp(std::move(movee));
    swap(tmp);
    return *this;
}

void buf_lock_t::swap(buf_lock_t &other) {
    ASSERT_NO_CORO_WAITING;
    guarantee(access_ref_count_ == 0);
    guarantee(other.access_ref_count_ == 0);
    std::swap(txn_, other.txn_);
    current_page_acq_.swap(other.current_page_acq_);
    std::swap(snapshot_node_, other.snapshot_node_);
}

void buf_lock_t::reset_buf_lock() {
    ASSERT_FINITE_CORO_WAITING;
    buf_lock_t tmp;
    swap(tmp);
}

bool buf_lock_t::is_snapshotted() const {
    cache()->assert_thread();
    return snapshot_node_ != nullptr;
}

void buf_lock_t::snapshot_subdag() {
    cache()->assert_thread();
#if ALT_DEBUG
    debugf("%p: buf_lock_t %p snapshot %" PRIu64 "\n", cache(), this, block_id());
#endif
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

current_page_acq_t *buf_lock_t::current_page_acq() const {
    ASSERT_NO_CORO_WAITING;
    guarantee(!empty());
    if (snapshot_node_ != NULL) {
        return snapshot_node_->current_page_acq_.get();
    } else {
        return current_page_acq_.get();
    }
}

void buf_lock_t::detach_child(block_id_t child_id) {
    ASSERT_FINITE_CORO_WAITING;
    guarantee(!empty());
    guarantee(access() == access_t::write);

    // _This_ txn isn't loading the page, so use the default reads account.
    buf_lock_t::create_child_snapshot_attachments(
            cache(),
            current_page_acq()->block_version(),
            block_id(),
            child_id);
}

repli_timestamp_t buf_lock_t::get_recency() const {
    guarantee(!empty());
    current_page_acq_t *cpa = current_page_acq();
    guarantee(cpa != NULL);

    // Emulate the cpa->recency() waiting behavior.  We only do this waiting here so
    // that we can guarantee(!empty()) after it's pulsed.  (FYI: We and the
    // page_cache wait for the write_acq_signal() so that a write acquirer can't see
    // its recency change before/after the write_acq_signal() gets pulsed.)
    if (access() == access_t::read) {
        cpa->read_acq_signal()->wait();
    } else {
        cpa->write_acq_signal()->wait();
    }

    ASSERT_FINITE_CORO_WAITING;
    guarantee(!empty());
    repli_timestamp_t ret = cpa->recency();
    // You may not call this on a buf lock that was marked deleted, and you
    // shouldn't call it on an aux block either.
    guarantee(ret != repli_timestamp_t::invalid);
    return ret;
}

void buf_lock_t::set_recency(repli_timestamp_t recency) {
    guarantee(!empty());
    // You should never need to set the recency of an aux block. It will be
    // discarded anyway.
    guarantee(!is_aux_block_id(block_id()));
    rassert(snapshot_node_ == NULL);

    // We only wait here so that we can guarantee(!empty()) after it's pulsed.
    current_page_acq_->write_acq_signal()->wait();

    ASSERT_FINITE_CORO_WAITING;
    guarantee(!empty());
    {
        repli_timestamp_t s = superceding_recency(current_page_acq_->recency(), recency);
        guarantee(s == recency);
    }
    current_page_acq_->set_recency(recency);
}

page_t *buf_lock_t::get_held_page_for_read() {
    guarantee(!empty());
    current_page_acq_t *cpa = current_page_acq();
    guarantee(cpa != NULL);
    // We only wait here so that we can guarantee(!empty()) after it's pulsed.
    cpa->read_acq_signal()->wait();

    ASSERT_FINITE_CORO_WAITING;
    guarantee(!empty());
    return cpa->current_page_for_read(txn()->account());
}

page_t *buf_lock_t::get_held_page_for_write() {
    guarantee(!empty());
    rassert(snapshot_node_ == NULL);
    // We only wait here so that we can guarantee(!empty()) after it's pulsed.
    current_page_acq_->write_acq_signal()->wait();

    ASSERT_FINITE_CORO_WAITING;
    guarantee(!empty());
    return current_page_acq_->current_page_for_write(txn()->account());
}

buf_read_t::buf_read_t(buf_lock_t *lock)
    : lock_(lock) {
    guarantee(!lock_->empty());
    lock_->access_ref_count_++;
}

buf_read_t::~buf_read_t() {
    guarantee(!lock_->empty());
    lock_->access_ref_count_--;
}

const void *buf_read_t::get_data_read(uint32_t *block_size_out) {
    page_t *page = lock_->get_held_page_for_read();
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache()->page_cache_,
                       lock_->txn()->account());
    }
    page_acq_.buf_ready_signal()->wait();
    *block_size_out = page_acq_.get_buf_size().value();
    return page_acq_.get_buf_read();
}

buf_write_t::buf_write_t(buf_lock_t *lock)
    : lock_(lock) {
    guarantee(lock_->access() == access_t::write);
    lock_->access_ref_count_++;
}

buf_write_t::~buf_write_t() {
    guarantee(!lock_->empty());
    lock_->access_ref_count_--;
}

void *buf_write_t::get_data_write(uint32_t block_size) {
    page_t *page = lock_->get_held_page_for_write();
    if (!page_acq_.has()) {
        page_acq_.init(page, &lock_->cache()->page_cache_,
                       lock_->txn()->account());
    }
    page_acq_.buf_ready_signal()->wait();
    return page_acq_.get_buf_write(block_size_t::make_from_cache(block_size));
}

void *buf_write_t::get_data_write() {
    return get_data_write(lock_->cache()->max_block_size().value());
}

