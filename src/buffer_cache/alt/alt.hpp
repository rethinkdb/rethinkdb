#ifndef BUFFER_CACHE_ALT_ALT_HPP_
#define BUFFER_CACHE_ALT_ALT_HPP_

#include <vector>
#include <utility>

#include "buffer_cache/alt/page.hpp"
#include "buffer_cache/general_types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/semaphore.hpp"
#include "containers/two_level_array.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

class serializer_t;

namespace alt {

class alt_buf_lock_t;
class alt_snapshot_node_t;

class alt_memory_tracker_t : public memory_tracker_t {
public:
    alt_memory_tracker_t();
    ~alt_memory_tracker_t();
    void inform_memory_change(uint64_t in_memory_size,
                              uint64_t memory_limit);
    void begin_txn_or_throttle(int64_t expected_change_count);
    void end_txn(int64_t saved_expected_change_count);
private:
    adjustable_semaphore_t semaphore_;
    DISABLE_COPYING(alt_memory_tracker_t);
};

class alt_cache_t : public home_thread_mixin_t {
public:
    explicit alt_cache_t(serializer_t *serializer);
    ~alt_cache_t();

    block_size_t max_block_size() const;
    // RSI: Remove this.
    block_size_t get_block_size() const { return max_block_size(); }

private:
    friend class alt_txn_t;  // for drainer_->lock()
    friend class alt_inner_txn_t;  // for &page_cache_
    friend class alt_buf_read_t;  // for &page_cache_
    friend class alt_buf_write_t;  // for &page_cache_

    friend class alt_buf_lock_t;  // for latest_snapshot_node and
                                  // push_latest_snapshot_node

    alt_snapshot_node_t *matching_snapshot_node_or_null(block_id_t block_id,
                                                        block_version_t block_version);
    void add_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node);
    void remove_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node);

    // tracker_ is used for throttling (which can cause the alt_txn_t constructor to
    // block).  RSI: The throttling interface is bad (maybe) because it's worried
    // about transaction_t's passing one another(?) or maybe the callers are bad with
    // their use of chained mutexes.  Make sure that timestamps don't get mixed up in
    // their ordering, once they begin to play a role.
    alt_memory_tracker_t tracker_;
    page_cache_t page_cache_;

    two_level_nevershrink_array_t<intrusive_list_t<alt_snapshot_node_t> > snapshot_nodes_by_block_id_;

    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(alt_cache_t);
};

class alt_inner_txn_t {
public:
    // This is public because scoped_ptr_t needs to call it.
    ~alt_inner_txn_t();

private:
    friend class alt_txn_t;
    alt_inner_txn_t(alt_cache_t *cache,
                    // Unused for read transactions, pass repli_timestamp_t::invalid.
                    repli_timestamp_t txn_recency,
                    alt_inner_txn_t *preceding_txn_or_null);

    alt_cache_t *cache() { return cache_; }

    page_txn_t *page_txn() { return &page_txn_; }

    alt_cache_t *cache_;
    page_txn_t page_txn_;

    DISABLE_COPYING(alt_inner_txn_t);
};

class alt_txn_t {
public:
    // Constructor for read-only transactions.
    // RSI: Generally speaking I don't think we use preceding_txn -- and should read
    // transactions use preceding_txn at all?
    explicit alt_txn_t(alt_cache_t *cache,
                       alt_read_access_t read_access,
                       alt_txn_t *preceding_txn = NULL);


    // RSI: Remove default parameter for expected_change_count.
    // RSI: Generally speaking I don't think we use preceding_txn and we should.
    alt_txn_t(alt_cache_t *cache,
              write_durability_t durability,
              repli_timestamp_t txn_timestamp,
              int64_t expected_change_count = 2,
              alt_txn_t *preceding_txn = NULL);

    ~alt_txn_t();

    alt_cache_t *cache() { return inner_->cache(); }
    page_txn_t *page_txn() { return inner_->page_txn(); }
    alt_access_t access() const { return access_; }
private:
    static void destroy_inner_txn(alt_inner_txn_t *inner,
                                  alt_cache_t *cache,
                                  int64_t saved_expected_change_count,
                                  auto_drainer_t::lock_t);

    const alt_access_t access_;

    // Only applicable if access_ == write.
    const write_durability_t durability_;
    const int64_t saved_expected_change_count_;  // RSI: A fugly relationship with
                                                 // the tracker.
    scoped_ptr_t<alt_inner_txn_t> inner_;
    DISABLE_COPYING(alt_txn_t);
};

// The intrusive list of alt_snapshot_node_t contains all the snapshot nodes for a
// given block id, in order by version.  (See
// alt_cache_t::snapshot_nodes_by_block_id_.)
class alt_snapshot_node_t : public intrusive_list_node_t<alt_snapshot_node_t> {
public:
    alt_snapshot_node_t(scoped_ptr_t<current_page_acq_t> &&acq);
    ~alt_snapshot_node_t();

private:
    // RSI: Should this really use friends?  Does this type need to be visible in the
    // header?
    friend class alt_buf_lock_t;
    friend class alt_cache_t;

    // This is never null (and is always a current_page_acq_t that has had
    // declare_snapshotted() called).
    scoped_ptr_t<current_page_acq_t> current_page_acq_;

    // RSP: std::map memory usage.
    // A NULL pointer associated with a block id indicates that the block is deleted.
    std::map<block_id_t, alt_snapshot_node_t *> children_;

    // The number of alt_buf_lock_t's referring to this node, plus the number of
    // alt_snapshot_node_t's referring to this node (via its children_ vector).
    int64_t ref_count_;


    DISABLE_COPYING(alt_snapshot_node_t);
};

class alt_buf_parent_t;

class alt_buf_lock_t {
public:
    alt_buf_lock_t();

    // RSI: These constructors definitely duplicate one another.  Too bad one
    // constructor can't call another (in GCC 4.4).  Maybe we could still dedup
    // these, though, or get rid of some.  (Make alt_access_t include create, and
    // separate it from page_access_t?)

    // Nonblocking constructor.
    alt_buf_lock_t(alt_buf_parent_t parent,
                   block_id_t block_id,
                   alt_access_t access);

    // Nonblocking constructor, creates a new block with a specified block id.
    alt_buf_lock_t(alt_txn_t *txn,
                   block_id_t block_id,
                   alt_create_t create);

    // Nonblocking constructor, IF parent->{access}_acq_signal() has already been
    // pulsed.  In either case, returns before the block is acquired, but after we're
    // _in line_ for the block.
    alt_buf_lock_t(alt_buf_lock_t *parent,
                   block_id_t block_id,
                   alt_access_t access);

    // Nonblocking constructor that acquires a block with a new block id.  `access`
    // must be `write`.
    alt_buf_lock_t(alt_buf_parent_t parent,
                   alt_create_t create);

    // Nonblocking constructor, IF parent->{access}_acq_signal() has already been
    // pulsed.  Allocates a block with a new block id.  `access` must be `write`.
    alt_buf_lock_t(alt_buf_lock_t *parent,
                   alt_create_t create);

    ~alt_buf_lock_t();

    alt_buf_lock_t(alt_buf_lock_t &&movee);
    alt_buf_lock_t &operator=(alt_buf_lock_t &&movee);

    void swap(alt_buf_lock_t &other);
    void reset_buf_lock();
    bool empty() const {
        return txn_ == NULL;
    }

    void snapshot_subtree();

    void detach_child(block_id_t child_id);

    block_id_t block_id() const {
        guarantee(txn_ != NULL);
        return current_page_acq()->block_id();
    }
    // RSI: Remove get_block_id().
    block_id_t get_block_id() const { return block_id(); }

    // RSI: Rename.
    repli_timestamp_t get_recency() const;

    alt_access_t access() const {
        guarantee(!empty());
        return current_page_acq()->access();
    }

    signal_t *read_acq_signal() {
        guarantee(!empty());
        return current_page_acq()->read_acq_signal();
    }
    signal_t *write_acq_signal() {
        guarantee(!empty());
        return current_page_acq()->write_acq_signal();
    }

    void mark_deleted();

    alt_txn_t *txn() const { return txn_; }
    alt_cache_t *cache() const { return txn_->cache(); }

private:
    static void wait_for_parent(alt_buf_parent_t parent, alt_access_t access);
    static alt_snapshot_node_t *
    get_or_create_child_snapshot_node(alt_cache_t *cache,
                                      alt_snapshot_node_t *parent,
                                      block_id_t child_id);
    static void create_empty_child_snapshot_nodes(alt_cache_t *cache,
                                                  block_version_t parent_version,
                                                  block_id_t parent_id,
                                                  block_id_t child_id);
    static void create_child_snapshot_nodes(alt_cache_t *cache,
                                            block_version_t parent_version,
                                            block_id_t parent_id,
                                            block_id_t child_id);
    current_page_acq_t *current_page_acq() const;

    friend class alt_buf_read_t;  // for get_held_page_for_read, access_ref_count_.
    friend class alt_buf_write_t;  // for get_held_page_for_write, access_ref_count_.

    page_t *get_held_page_for_read();
    page_t *get_held_page_for_write();

    alt_txn_t *txn_;

    scoped_ptr_t<current_page_acq_t> current_page_acq_;

    alt_snapshot_node_t *snapshot_node_;

    // Keeps track of how many alt_buf_{read|write}_t have been created for
    // this lock, for assertion/guarantee purposes.
    intptr_t access_ref_count_;

    // RSI: We should get rid of this variable.
    bool was_destroyed_;

    DISABLE_COPYING(alt_buf_lock_t);
};


class alt_buf_parent_t {
public:
    alt_buf_parent_t() : txn_(NULL), lock_or_null_(NULL) { }

    explicit alt_buf_parent_t(alt_buf_lock_t *lock)
        : txn_(lock->txn()), lock_or_null_(lock) {
        guarantee(lock != NULL);
        guarantee(!lock->empty());
    }
    // RSI: Replace this constructor with a create_dangerously static method.
    explicit alt_buf_parent_t(alt_txn_t *txn)
        : txn_(txn), lock_or_null_(NULL) { }

    alt_txn_t *txn() const {
        guarantee(txn_ != NULL);
        return txn_;
    }
    alt_cache_t *cache() const {
        guarantee(txn_ != NULL);
        return txn_->cache();
    }

private:
    friend class alt_buf_lock_t;
    alt_txn_t *txn_;
    alt_buf_lock_t *lock_or_null_;
};

class alt_buf_read_t {
public:
    explicit alt_buf_read_t(alt_buf_lock_t *lock);
    ~alt_buf_read_t();

    const void *get_data_read(uint32_t *block_size_out);
    // RSI: Remove.
    const void *get_data_read() {
        uint32_t unused_block_size;
        return get_data_read(&unused_block_size);
    }

private:
    alt_buf_lock_t *lock_;
    page_acq_t page_acq_;

    DISABLE_COPYING(alt_buf_read_t);
};

class alt_buf_write_t {
public:
    explicit alt_buf_write_t(alt_buf_lock_t *lock);
    ~alt_buf_write_t();

    void *get_data_write(uint32_t block_size);
    // Equivalent to passing the max_block_size.
    void *get_data_write();

private:
    alt_buf_lock_t *lock_;
    page_acq_t page_acq_;

    DISABLE_COPYING(alt_buf_write_t);
};


}  // namespace alt

#endif  // BUFFER_CACHE_ALT_ALT_HPP_
