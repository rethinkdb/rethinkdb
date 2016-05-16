// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_ALT_HPP_
#define BUFFER_CACHE_ALT_HPP_

#include <map>
#include <vector>
#include <utility>

#include "buffer_cache/page_cache.hpp"
#include "buffer_cache/types.hpp"
#include "containers/two_level_array.hpp"
#include "repli_timestamp.hpp"

class serializer_t;

class buf_lock_t;
class alt_cache_stats_t;
class alt_snapshot_node_t;
class perfmon_collection_t;
class cache_balancer_t;

class alt_txn_throttler_t {
public:
    explicit alt_txn_throttler_t(int64_t minimum_unwritten_changes_limit);
    ~alt_txn_throttler_t();

    alt::throttler_acq_t begin_txn_or_throttle(int64_t expected_change_count);
    void end_txn(alt::throttler_acq_t acq);

    void inform_memory_limit_change(uint64_t memory_limit,
                                    block_size_t max_block_size);

private:
    const int64_t minimum_unwritten_changes_limit_;

    new_semaphore_t unwritten_block_changes_semaphore_;
    new_semaphore_t unwritten_index_changes_semaphore_;

    DISABLE_COPYING(alt_txn_throttler_t);
};

class cache_t : public home_thread_mixin_t {
public:
    explicit cache_t(serializer_t *serializer,
                     cache_balancer_t *balancer,
                     perfmon_collection_t *perfmon_collection);
    ~cache_t();

    max_block_size_t max_block_size() const { return page_cache_.max_block_size(); }

    // These todos come from the mirrored cache.  The real problem is that whole
    // cache account / priority thing is just one ghetto hack amidst a dozen other
    // throttling systems.  TODO: Come up with a consistent priority scheme,
    // i.e. define a "default" priority etc.  TODO: As soon as we can support it, we
    // might consider supporting a mem_cap paremeter.
    cache_account_t create_cache_account(int priority);

private:
    friend class txn_t;
    friend class buf_read_t;
    friend class buf_write_t;
    friend class buf_lock_t;

    alt_snapshot_node_t *matching_snapshot_node_or_null(
            block_id_t block_id,
            alt::block_version_t block_version);
    void add_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node);
    void remove_snapshot_node(block_id_t block_id, alt_snapshot_node_t *node);

    // throttler_ can cause the txn_t constructor to block
    alt_txn_throttler_t throttler_;
    alt::page_cache_t page_cache_;

    scoped_ptr_t<alt_cache_stats_t> stats_;

    std::map<block_id_t, intrusive_list_t<alt_snapshot_node_t> >
        snapshot_nodes_by_block_id_;

    DISABLE_COPYING(cache_t);
};

class txn_t {
public:
    // Constructor for read-only transactions.
    txn_t(cache_conn_t *cache_conn, read_access_t read_access);

    txn_t(cache_conn_t *cache_conn,
          write_durability_t durability,
          int64_t expected_change_count);

    ~txn_t();

    cache_t *cache() { return cache_; }
    alt::page_txn_t *page_txn() { return page_txn_.get(); }
    access_t access() const { return access_; }

    void set_account(cache_account_t *cache_account);
    cache_account_t *account() { return cache_account_; }

private:
    // Resets the *throttler_acq parameter.
    static void inform_tracker(cache_t *cache,
                               alt::throttler_acq_t *throttler_acq);

    // Resets the *throttler_acq parameter.
    static void pulse_and_inform_tracker(cache_t *cache,
                                         alt::throttler_acq_t *throttler_acq,
                                         cond_t *pulsee);


    void help_construct(int64_t expected_change_count, cache_conn_t *cache_conn);

    cache_t *const cache_;

    // Initialized to cache()->page_cache_.default_cache_account(), and modified by
    // set_account().
    cache_account_t *cache_account_;

    const access_t access_;

    // Only applicable if access_ == write.
    const write_durability_t durability_;

    scoped_ptr_t<alt::page_txn_t> page_txn_;

    DISABLE_COPYING(txn_t);
};

class buf_parent_t;

class buf_lock_t {
public:
    buf_lock_t();

    // buf_parent_t is a type that either points at a buf_lock_t (its parent) or
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
               access_t access);

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
               access_t access);

    // Creates a block, a new child of the given parent.  It gets assigned a block id
    // from one of the unused block id's.
    buf_lock_t(buf_parent_t parent,
               alt_create_t create,
               block_type_t block_type = block_type_t::normal);

    // Creates a block, a new child of the given parent.  It gets assigned a block id
    // from one of the unused block id's.
    buf_lock_t(buf_lock_t *parent,
               alt_create_t create,
               block_type_t block_type = block_type_t::normal);

    ~buf_lock_t();

    buf_lock_t(buf_lock_t &&movee);
    buf_lock_t &operator=(buf_lock_t &&movee);

    void swap(buf_lock_t &other);
    void reset_buf_lock();
    bool empty() const {
        return txn_ == nullptr;
    }

    bool is_snapshotted() const;
    void snapshot_subdag();

    void detach_child(block_id_t child_id);

    block_id_t block_id() const {
        guarantee(txn_ != nullptr);
        return current_page_acq()->block_id();
    }

    // It is illegal to call this on a buf lock that has been mark_deleted, or that
    // is a lock on an aux block.  This never returns repli_timestamp_t::invalid.
    repli_timestamp_t get_recency() const;

    // Sets the buf's recency to `superceding_recency`, which must be greater than or
    // equal to its current recency. Operations that add or modify entries to the leaf
    // nodes of the B-tree should call this for every node in the path from the root to
    // the leaf.
    void set_recency(repli_timestamp_t superceding_recency);

    access_t access() const {
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
    void help_construct(buf_parent_t parent, block_id_t block_id, access_t access);
    void help_construct(
        buf_parent_t parent,
        alt_create_t create,
        block_type_t block_type);
    void help_construct(buf_parent_t parent, block_id_t block_id, alt_create_t create);

    static alt_snapshot_node_t *help_make_child(cache_t *cache, block_id_t child_id);


    static void wait_for_parent(buf_parent_t parent, access_t access);
    static alt_snapshot_node_t *
    get_or_create_child_snapshot_node(cache_t *cache,
                                      alt_snapshot_node_t *parent,
                                      block_id_t child_id);
    static void create_empty_child_snapshot_attachments(
            cache_t *cache,
            alt::block_version_t parent_version,
            block_id_t parent_id,
            block_id_t child_id);
    static void create_child_snapshot_attachments(cache_t *cache,
                                                  alt::block_version_t parent_version,
                                                  block_id_t parent_id,
                                                  block_id_t child_id);
    alt::current_page_acq_t *current_page_acq() const;

    friend class buf_read_t;  // for get_held_page_for_read, access_ref_count_.
    friend class buf_write_t;  // for get_held_page_for_write, access_ref_count_.

    alt::page_t *get_held_page_for_read();
    alt::page_t *get_held_page_for_write();

    txn_t *txn_;

    scoped_ptr_t<alt::current_page_acq_t> current_page_acq_;

    alt_snapshot_node_t *snapshot_node_;

    // Keeps track of how many alt_buf_{read|write}_t have been created for
    // this lock, for assertion/guarantee purposes.
    intptr_t access_ref_count_;

    DISABLE_COPYING(buf_lock_t);
};


class buf_parent_t {
public:
    buf_parent_t() : txn_(nullptr), lock_or_null_(nullptr) { }

    explicit buf_parent_t(buf_lock_t *lock)
        : txn_(lock->txn()), lock_or_null_(lock) {
        guarantee(lock != nullptr);
        guarantee(!lock->empty());
    }

    explicit buf_parent_t(txn_t *_txn)
        : txn_(_txn), lock_or_null_(nullptr) {
        rassert(_txn != NULL);
    }

    void detach_child(block_id_t child_id) {
        if (lock_or_null_ != nullptr) {
            lock_or_null_->detach_child(child_id);
        }
    }

    bool empty() const {
        return txn_ == nullptr;
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
    alt::page_acq_t page_acq_;

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
    alt::page_acq_t page_acq_;

    DISABLE_COPYING(buf_write_t);
};


#endif  // BUFFER_CACHE_ALT_HPP_
