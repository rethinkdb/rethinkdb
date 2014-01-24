#ifndef BUFFER_CACHE_ALT_ALT_HPP_
#define BUFFER_CACHE_ALT_ALT_HPP_

#include <map>
#include <vector>
#include <utility>

#include "buffer_cache/alt/page.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/semaphore.hpp"
#include "containers/two_level_array.hpp"
#include "repli_timestamp.hpp"
#include "utils.hpp"

class serializer_t;

class buf_lock_t;
class alt_cache_config_t;
class alt_cache_stats_t;
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

class cache_t : public home_thread_mixin_t {
public:
    explicit cache_t(serializer_t *serializer,
                     const alt_cache_config_t &dynamic_config,
                     perfmon_collection_t *perfmon_collection);
    ~cache_t();

    block_size_t max_block_size() const;
    // KSI: Remove this.
    block_size_t get_block_size() const { return max_block_size(); }

    // These todos come from the mirrored cache.  The real problem is that whole
    // cache account / priority thing is just one ghetto hack amidst a dozen other
    // throttling systems.  TODO: Come up with a consistent priority scheme,
    // i.e. define a "default" priority etc.  TODO: As soon as we can support it, we
    // might consider supporting a mem_cap paremeter.
    void create_cache_account(int priority, scoped_ptr_t<alt_cache_account_t> *out);

private:
    friend class txn_t;  // for drainer_->lock()
    friend class alt_inner_txn_t;  // for &page_cache_
    friend class buf_read_t;  // for &page_cache_
    friend class buf_write_t;  // for &page_cache_

    friend class buf_lock_t;  // for latest_snapshot_node and
                              // push_latest_snapshot_node

    alt_snapshot_node_t *matching_snapshot_node_or_null(block_id_t block_id,
                                                        block_version_t block_version);
    void add_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node);
    void remove_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node);

    scoped_ptr_t<alt_cache_stats_t> stats_;

    // tracker_ is used for throttling (which can cause the txn_t constructor to
    // block).  RSI: The throttling interface is bad (maybe) because it's worried
    // about transaction_t's passing one another(?) or maybe the callers are bad with
    // their use of chained mutexes.  Make sure that timestamps don't get mixed up in
    // their ordering, once they begin to play a role.
    alt_memory_tracker_t tracker_;
    page_cache_t page_cache_;

    two_level_nevershrink_array_t<intrusive_list_t<alt_snapshot_node_t> > snapshot_nodes_by_block_id_;

    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(cache_t);
};

class alt_inner_txn_t {
public:
    // This is public because scoped_ptr_t needs to call it.
    ~alt_inner_txn_t();

private:
    friend class txn_t;
    alt_inner_txn_t(cache_t *cache,
                    // Unused for read transactions, pass repli_timestamp_t::invalid.
                    repli_timestamp_t txn_recency,
                    alt_inner_txn_t *preceding_txn_or_null);

    cache_t *cache() { return cache_; }

    page_txn_t *page_txn() { return &page_txn_; }

    cache_t *cache_;
    page_txn_t page_txn_;

    DISABLE_COPYING(alt_inner_txn_t);
};

class txn_t {
public:
    // Constructor for read-only transactions.
    // RSI: Generally speaking I don't think we use preceding_txn.
    explicit txn_t(cache_t *cache,
                   alt_read_access_t read_access,
                   txn_t *preceding_txn = NULL);


    // KSI: Remove default parameter for expected_change_count.
    // RSI: Generally speaking I don't think we use preceding_txn and we should.
    txn_t(cache_t *cache,
          write_durability_t durability,
          repli_timestamp_t txn_timestamp,
          int64_t expected_change_count = 2,
          txn_t *preceding_txn = NULL);

    ~txn_t();

    cache_t *cache() { return inner_->cache(); }
    page_txn_t *page_txn() { return inner_->page_txn(); }
    alt_access_t access() const { return access_; }

    void set_account(alt_cache_account_t *cache_account);

private:
    void help_construct(cache_t *cache,
                        repli_timestamp_t txn_timestamp,
                        txn_t *preceding_txn);

    static void destroy_inner_txn(alt_inner_txn_t *inner,
                                  cache_t *cache,
                                  int64_t saved_expected_change_count,
                                  auto_drainer_t::lock_t);

    const alt_access_t access_;

    // Only applicable if access_ == write.
    const write_durability_t durability_;
    const int64_t saved_expected_change_count_;  // RSI: A fugly relationship with
                                                 // the tracker.

    scoped_ptr_t<alt_inner_txn_t> inner_;
    DISABLE_COPYING(txn_t);
};

class buf_parent_t;

class buf_lock_t {
public:
    buf_lock_t();

    // RSI: These constructors definitely duplicate one another.  Too bad one
    // constructor can't call another (in GCC 4.4).  Maybe we could still dedup
    // these, though, or get rid of some.  (Make alt_access_t include create, and
    // separate it from page_access_t?)

    // alt_buf_parent_t is a type that either points at a buf_lock_t (its parent) or
    // merely at a txn_t (e.g. for acquiring the superblock, which has no parent).
    // If acquiring the child for read, the constructor will wait for the parent to
    // be acquired for read.  Similarly, if acquiring the child for write, the
    // constructor will wait for the parent to be acquired for write.  Once the
    // constructor returns, you are "in line" for the block, meaning you'll acquire
    // it in the same order relative other agents as you did when acquiring the same
    // parent.  (Of course, readers can intermingle.)

    // These constructors will _not_ yield the coroutine _if_ the parent is already
    // {access}-acquired.

    // Acquires an existing block for read or write access.
    buf_lock_t(buf_parent_t parent,
               block_id_t block_id,
               alt_access_t access);

    // Creates a new block with a specified block id, one that doesn't have a parent.
    buf_lock_t(txn_t *txn,
               block_id_t block_id,
               alt_create_t create);

    // Creates a new block with a specified block id as the child of a parent (if it's
    // not just a txn_t *).
    buf_lock_t(buf_parent_t parent,
               block_id_t block_id,
               alt_create_t create);

    // Acquires an existing block given the parent.
    buf_lock_t(buf_lock_t *parent,
               block_id_t block_id,
               alt_access_t access);

    // Creates a block, a new child of the given parent.  It gets assigned a block id
    // from one of the unused block id's.
    buf_lock_t(buf_parent_t parent,
               alt_create_t create);

    // Creates a block, a new child of the given parent.  It gets assigned a block id
    // from one of the unused block id's.
    buf_lock_t(buf_lock_t *parent,
               alt_create_t create);

    ~buf_lock_t();

    buf_lock_t(buf_lock_t &&movee);
    buf_lock_t &operator=(buf_lock_t &&movee);

    void swap(buf_lock_t &other);
    void reset_buf_lock();
    bool empty() const {
        return txn_ == NULL;
    }

    void snapshot_subdag();

    void detach_child(block_id_t child_id);

    block_id_t block_id() const {
        guarantee(txn_ != NULL);
        return current_page_acq()->block_id();
    }

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

    txn_t *txn() const { return txn_; }
    cache_t *cache() const { return txn_->cache(); }

private:
    void help_construct(buf_parent_t parent, alt_create_t create);
    void help_construct(buf_parent_t parent, block_id_t block_id, alt_create_t create);

    static void wait_for_parent(buf_parent_t parent, alt_access_t access);
    static alt_snapshot_node_t *
    get_or_create_child_snapshot_node(cache_t *cache,
                                      alt_snapshot_node_t *parent,
                                      block_id_t child_id);
    static void create_empty_child_snapshot_nodes(cache_t *cache,
                                                  block_version_t parent_version,
                                                  block_id_t parent_id,
                                                  block_id_t child_id);
    static void create_child_snapshot_nodes(cache_t *cache,
                                            block_version_t parent_version,
                                            block_id_t parent_id,
                                            block_id_t child_id);
    current_page_acq_t *current_page_acq() const;

    friend class buf_read_t;  // for get_held_page_for_read, access_ref_count_.
    friend class buf_write_t;  // for get_held_page_for_write, access_ref_count_.

    page_t *get_held_page_for_read();
    page_t *get_held_page_for_write();

    txn_t *txn_;

    scoped_ptr_t<current_page_acq_t> current_page_acq_;

    alt_snapshot_node_t *snapshot_node_;

    // Keeps track of how many alt_buf_{read|write}_t have been created for
    // this lock, for assertion/guarantee purposes.
    intptr_t access_ref_count_;

    DISABLE_COPYING(buf_lock_t);
};


class buf_parent_t {
public:
    buf_parent_t() : txn_(NULL), lock_or_null_(NULL) { }

    explicit buf_parent_t(buf_lock_t *lock)
        : txn_(lock->txn()), lock_or_null_(lock) {
        guarantee(lock != NULL);
        guarantee(!lock->empty());
    }

    explicit buf_parent_t(txn_t *txn)
        : txn_(txn), lock_or_null_(NULL) {
        rassert(txn != NULL);
    }

    bool empty() const {
        return txn_ == NULL;
    }

    txn_t *txn() const {
        guarantee(!empty());
        return txn_;
    }
    cache_t *cache() const {
        guarantee(!empty());
        return txn_->cache();
    }

private:
    friend class buf_lock_t;
    txn_t *txn_;
    buf_lock_t *lock_or_null_;
};

class buf_read_t {
public:
    explicit buf_read_t(buf_lock_t *lock);
    ~buf_read_t();

    const void *get_data_read(uint32_t *block_size_out);
    const void *get_data_read() {
        uint32_t block_size;
        const void *data = get_data_read(&block_size);
        guarantee(block_size == lock_->cache()->max_block_size().value());
        return data;
    }

private:
    buf_lock_t *lock_;
    page_acq_t page_acq_;

    DISABLE_COPYING(buf_read_t);
};

class buf_write_t {
public:
    explicit buf_write_t(buf_lock_t *lock);
    ~buf_write_t();

    void *get_data_write(uint32_t block_size);
    // Equivalent to passing the max_block_size.
    void *get_data_write();

private:
    buf_lock_t *lock_;
    page_acq_t page_acq_;

    DISABLE_COPYING(buf_write_t);
};


#endif  // BUFFER_CACHE_ALT_ALT_HPP_
