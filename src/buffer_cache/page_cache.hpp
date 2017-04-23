// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef BUFFER_CACHE_PAGE_CACHE_HPP_
#define BUFFER_CACHE_PAGE_CACHE_HPP_

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "buffer_cache/block_version.hpp"
#include "buffer_cache/cache_account.hpp"
#include "buffer_cache/evicter.hpp"
#include "buffer_cache/free_list.hpp"
#include "buffer_cache/page.hpp"
#include "buffer_cache/types.hpp"
#include "concurrency/access.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/cond_var.hpp"
#include "concurrency/fifo_enforcer.hpp"
#include "concurrency/new_semaphore.hpp"
#include "containers/backindex_bag.hpp"
#include "containers/intrusive_list.hpp"
#include "containers/segmented_vector.hpp"
#include "repli_timestamp.hpp"
#include "serializer/types.hpp"
#include "time.hpp"

// HSI: unordered_map allocates too much, use a different hash table type.

class alt_txn_throttler_t;
class cache_balancer_t;
class auto_drainer_t;
class cache_t;
class file_account_t;

namespace alt {
class current_page_acq_t;
class page_cache_t;
class page_txn_t;
struct current_page_dirtier_t;
struct flush_prep_t;

enum class page_create_t { no, yes };

}  // namespace alt

enum class alt_create_t { create };

enum class block_type_t { normal, aux };

class page_txn_complete_cb_t
        : public intrusive_list_node_t<page_txn_complete_cb_t> {
public:
    cond_t cond;
};


class cache_conn_t : public half_intrusive_list_node_t<cache_conn_t> {
public:
    explicit cache_conn_t(cache_t *_cache)
        : cache_(_cache),
          newest_txn_(nullptr) { }
    ~cache_conn_t();

    cache_t *cache() const { return cache_; }
private:
    friend class alt::page_cache_t;
    friend class alt::page_txn_t;
    // Here for convenience, because otherwise you'd be passing around a cache_t with
    // every cache_conn_t parameter.
    cache_t *cache_;

    // The most recent unflushed txn, or NULL.  This gets set back to NULL when
    // newest_txn_ finishes.  We're an element of its cache_conns_ list.
    alt::page_txn_t *newest_txn_;

    DISABLE_COPYING(cache_conn_t);
};


namespace alt {


// Has information necessary for the current_page_t to do certain things -- it's
// known by the current_page_acq_t.
class current_page_help_t;

class current_page_t {
public:
    current_page_t(block_id_t block_id, buf_ptr_t buf, page_cache_t *page_cache);
    current_page_t(block_id_t block_id, buf_ptr_t buf,
                   const counted_t<block_token_t> &token,
                   page_cache_t *page_cache);
    // Constructs a page to be loaded from the serializer.
    current_page_t(block_id_t block_id, page_cache_t *page_cache);

    // You MUST call reset() before destructing a current_page_t!
    ~current_page_t();

    // You can only call this when it's safe to do so!  (Beware of
    // current_page_acq_t's, last write acquirer page_txn_t's, and read-ahead logic.)
    void reset(page_cache_t *page_cache);

    bool should_be_evicted() const;

private:
    // current_page_acq_t should not access our fields directly.
    friend class current_page_acq_t;
    void add_acquirer(current_page_acq_t *acq);
    void remove_acquirer(current_page_acq_t *acq);
    void pulse_pulsables(current_page_acq_t *acq);
    void add_keepalive();
    void remove_keepalive();

    page_t *the_page_for_write(current_page_help_t help, cache_account_t *account);
    page_t *the_page_for_read(current_page_help_t help, cache_account_t *account);

    // Initializes page_ if necessary, providing an account because we know we'd like
    // to load it ASAP.
    void convert_from_serializer_if_necessary(current_page_help_t help,
                                              cache_account_t *account);

    // Initializes page_ if necessary, deferring loading of the actual block.
    void convert_from_serializer_if_necessary(current_page_help_t help);

    void mark_deleted(current_page_help_t help);

    // page_txn_t should not access our fields directly.
    friend class page_txn_t;

    // Returns NULL if the page was deleted.
    page_t *the_page_for_read_or_deleted(current_page_help_t help);

    // Has access to our fields.
    friend class page_cache_t;

    friend backindex_bag_index_t *access_backindex(current_page_t *current_page);
    friend backindex_bag_index_t *access_backindex(current_page_dirtier_t dirtier);
    friend void update_backindex_back_pointer(current_page_t *cp, page_txn_t *txn);
    friend void update_backindex_back_pointer(
        current_page_dirtier_t cp, page_txn_t *txn);


    bool is_deleted() const { return is_deleted_; }

    // KSI: We could get rid of this variable if
    // page_txn_t::pages_write_acquired_last_ noted each page's block_id_t.  Other
    // space reductions are more important.
    block_id_t block_id_;

    // page_ can be null if we haven't tried loading the page yet.  We don't want to
    // prematurely bother loading the page if it's going to be deleted.
    // the_page_for_write, the_page_for_read, or the_page_for_read_or_deleted should
    // be used to access this variable.
    // KSI: Could we encapsulate that rule?
    page_ptr_t page_;
    // True if the block is in a deleted state.  page_ will be null.
    bool is_deleted_;

    // The last write acquirer for this page.
    page_txn_t *last_write_acquirer_;
    // Our index into the last_write_acquirer_->pages_write_acquired_last_.
    backindex_bag_index_t last_write_acquirer_index_;

    // The version of the page, that the last write acquirer had.
    block_version_t last_write_acquirer_version_;

    page_txn_t *last_dirtier_;
    backindex_bag_index_t last_dirtier_index_;

    // The version and recency of the page, that the last dirtier had.  We merely set
    // and read these values, the only thing it affects is compute_changes, later.
    // Maybe we should replace last_dirtier_ with last_toucher_or_dirtier_.  Then we
    // wouldn't need this recency field.
    block_version_t last_dirtier_version_;
    repli_timestamp_t last_dirtier_recency_;

    // Instead of storing the recency here, we store it page_cache_t::recencies_.

    // All list elements have current_page_ != NULL, snapshotted_page_ == NULL.
    intrusive_list_t<current_page_acq_t> acquirers_;

    // Avoids eviction if > 0. This is used by snapshotted current_page_acq_t's
    // that have a snapshotted version of this block. If the current_page_t
    // would be evicted that would mess with the block version.
    intptr_t num_keepalives_;

    DISABLE_COPYING(current_page_t);
};

inline backindex_bag_index_t *access_backindex(current_page_t *current_page) {
    return &current_page->last_write_acquirer_index_;
}

// Distinguishes type for access_backindex overloading.
struct current_page_dirtier_t {
    current_page_t *current_page;
};
inline backindex_bag_index_t *access_backindex(current_page_dirtier_t dirtier) {
    return &dirtier.current_page->last_dirtier_index_;
}


class current_page_acq_t : public intrusive_list_node_t<current_page_acq_t>,
                           public home_thread_mixin_debug_only_t {
public:
    // KSI: Right now we support a default constructor but buf_lock_t actually
    // uses a scoped pointer now, because getting this type to be swappable was too
    // hard.  Make this type be swappable or remove the default constructor.  (Remove
    // the page_cache_ != NULL check in the destructor we remove the default
    // constructor.)
    current_page_acq_t();
    current_page_acq_t(page_txn_t *txn,
                       block_id_t block_id,
                       access_t access,
                       page_create_t create = page_create_t::no);
    current_page_acq_t(page_txn_t *txn,
                       alt_create_t create,
                       block_type_t block_type);
    current_page_acq_t(page_cache_t *cache,
                       block_id_t block_id,
                       read_access_t read);
    ~current_page_acq_t();

    // Declares ourself snapshotted.  (You must be readonly to do this.)
    void declare_snapshotted();

    signal_t *read_acq_signal();
    signal_t *write_acq_signal();

    page_t *current_page_for_read(cache_account_t *account);
    repli_timestamp_t recency();

    page_t *current_page_for_write(cache_account_t *account);
    void set_recency(repli_timestamp_t recency);

    block_id_t block_id() const { return block_id_; }
    access_t access() const { return access_; }

    void mark_deleted();

    block_version_t block_version() const {
        assert_thread();
        return block_version_;
    }

    page_cache_t *page_cache() const {
        assert_thread();
        return page_cache_;
    }

private:
    void init(page_txn_t *txn,
              block_id_t block_id,
              access_t access,
              page_create_t create);
    void init(page_txn_t *txn,
              alt_create_t create,
              block_type_t block_type);
    void init(page_cache_t *page_cache,
              block_id_t block_id,
              read_access_t read);
    friend class page_txn_t;
    friend class current_page_t;

    // Returns true if the page has been created, edited, or deleted.
    bool dirtied_page() const;

    // Returns true if the page's recency has been modified.
    bool touched_page() const;

    // Declares ourself readonly.  Only page_txn_t::remove_acquirer can do this!
    void declare_readonly();

    current_page_help_t help() const;

    void pulse_read_available();
    void pulse_write_available();

    void dirty_the_page();

    page_cache_t *page_cache_;
    page_txn_t *the_txn_;
    access_t access_;
    bool declared_snapshotted_;
    // The block id of the page we acquired.
    block_id_t block_id_;
    // At most one of current_page_ is null or snapshotted_page_ is null, unless the
    // acquired page has been deleted, in which case both are null.
    current_page_t *current_page_;
    timestamped_page_ptr_t snapshotted_page_;
    cond_t read_cond_;
    cond_t write_cond_;

    // The block version for our acquisition of the page -- every write acquirer sees
    // a greater block version than the previous acquirer.  The current page's block
    // version will be less than or equal to this value if we have not yet acquired
    // the page.  It could be greater than this value if we're snapshotted (since
    // we're holding an old version of the page).  These values are for internal
    // cache bookkeeping only.
    block_version_t block_version_;

    bool dirtied_page_, touched_page_;

    DISABLE_COPYING(current_page_acq_t);
};

// This object lives on the serializer thread.
class page_read_ahead_cb_t : public home_thread_mixin_t,
                             public serializer_read_ahead_callback_t {
public:
    page_read_ahead_cb_t(serializer_t *serializer,
                         page_cache_t *cache);

    void offer_read_ahead_buf(block_id_t block_id,
                              buf_ptr_t *buf,
                              const counted_t<block_token_t> &token);

    void destroy_self();

private:
    ~page_read_ahead_cb_t();

    serializer_t *serializer_;
    page_cache_t *page_cache_;

    DISABLE_COPYING(page_read_ahead_cb_t);
};

class throttler_acq_t {
public:
    explicit throttler_acq_t(write_durability_t durability,
                             int64_t expected_change_count)
        : prevent_updates_(false),
          expected_change_count_(expected_change_count),
          pre_spawn_flush_(durability == write_durability_t::HARD) { }
    ~throttler_acq_t() { }
    throttler_acq_t(throttler_acq_t &&movee)
        : prevent_updates_(movee.prevent_updates_),
          expected_change_count_(movee.expected_change_count_),
          pre_spawn_flush_(movee.pre_spawn_flush_),
          block_changes_semaphore_acq_(std::move(movee.block_changes_semaphore_acq_)),
          index_changes_semaphore_acq_(std::move(movee.index_changes_semaphore_acq_)) {
        movee.prevent_updates_ = false;
        movee.pre_spawn_flush_ = false;
        movee.expected_change_count_ = 0;
        movee.block_changes_semaphore_acq_.reset();
        movee.index_changes_semaphore_acq_.reset();
    }

    bool has_txn_throttler() const {
        // Either semaphore would do.
        return block_changes_semaphore_acq_.has_semaphore();
    }

    // See below:  this can update how much *_changes_semaphore_acq_ holds.
    void update_dirty_page_count(int64_t new_count);

    // Sets block_changes_semaphore_acq_ to 0, but keeps index_changes_semaphore_acq_
    // as it is.
    void mark_dirty_pages_written();

    void merge(throttler_acq_t &&other);

    // This is set by other algorithms.
    bool pre_spawn_flush() const { return pre_spawn_flush_; }

    // sets pre_spawn_flush to true, updates the dirty page count.
    void set_pre_spawn_flush(int64_t dirty_page_count) {
        if (!pre_spawn_flush_) {
            pre_spawn_flush_ = true;
            if (has_txn_throttler()) {
                update_dirty_page_count(dirty_page_count);
            }
        }
    }

    void set_prevent_updates();

private:
    // Prevents pages the semaphores from updating their dirty page count when true,
    // despite update_dirty_page_count getting called.  (Allows
    // mark_dirty_pages_written.)
    bool prevent_updates_;

    // The original expected_change_count value.  Once pre_spawn_flush_ is true and we
    // start to block the semaphores, we jump up to this value.
    int64_t expected_change_count_;

    // True if we want to flush the txn's values ASAP.  (True for hard-durability txn's,
    // true for soft-durability txn's that we've decided to flush.)
    // We don't acquire anything on the semaphore until this is true.
    bool pre_spawn_flush_;

    friend class ::alt_txn_throttler_t;
    // At first, the number of dirty pages is 0 and *_changes_semaphore_acq_.count() >=
    // dirtied_count_.  Once the number of dirty pages gets bigger than the original
    // value of *_changes_semaphore_acq_.count(), we use
    // *_changes_semaphore_acq_.change_count() to keep the numbers equal.
    new_semaphore_in_line_t block_changes_semaphore_acq_;
    new_semaphore_in_line_t index_changes_semaphore_acq_;

    DISABLE_COPYING(throttler_acq_t);
};

class page_cache_index_write_sink_t;

struct block_change_t {
    block_change_t() = default;
    block_change_t(block_version_t _version, repli_timestamp_t _tstamp)
        : version(_version), modified(false), page(), tstamp(_tstamp) { }
    block_change_t(block_version_t _version, bool _modified,
                   page_ptr_t &&_page, repli_timestamp_t _tstamp)
        : version(_version), modified(_modified), page(std::move(_page)),
          tstamp(_tstamp) { }

    // This function has a specific obligation to leave other in an arbitrary
    // safe-to-destruct state.  (other's page must be reset.)  Returns the net
    // difference in the number of dirty pages.  (Either 0 or -1.)
    MUST_USE int merge(page_cache_t *page_cache, block_change_t &&other);

    block_version_t version;

    // True if the value of the block was modified (or the block was deleted), false
    // if the block was only touched.
    bool modified;
    // If modified == true, the new value for the block, or empty if the block was
    // deleted.
    page_ptr_t page;
    repli_timestamp_t tstamp;
};


class page_cache_t : public home_thread_mixin_t {
public:
    page_cache_t(serializer_t *serializer,
                 cache_balancer_t *balancer,
                 alt_txn_throttler_t *throttler);
    ~page_cache_t();

    // Begins to flush pending txn's.
    void begin_flush_pending_txns(bool asap, ticks_t soft_deadline /* 0 is okay */);
    // Starts an official soft durability interval flush, if one isn't running already.
    void soft_durability_interval_flush(ticks_t soft_deadline);

    // Takes a txn to be flushed.  Pulses on_complete_or_null when done.
    void flush_and_destroy_txn(
            scoped_ptr_t<page_txn_t> &&txn,
            write_durability_t durability,
            page_txn_complete_cb_t *on_complete_or_null);
    // More efficient version of `flush_and_destroy_txn` for read transactions.
    void end_read_txn(scoped_ptr_t<page_txn_t> txn);

    current_page_t *page_for_block_id(block_id_t block_id);
    current_page_t *page_for_new_block_id(
        block_type_t block_type,
        block_id_t *block_id_out);
    current_page_t *page_for_new_chosen_block_id(block_id_t block_id);

    // Returns how much memory is being used by all the pages in the cache at this
    // moment in time.
    size_t total_page_memory() const;
    size_t evictable_page_memory() const;

    max_block_size_t max_block_size() const { return max_block_size_; }

    cache_account_t create_cache_account(int priority);

    cache_account_t *default_reads_account() {
        return &default_reads_account_;
    }

    // Considers wiping out the current_page_t (and its page_t pointee) for a
    // particular block id, to save memory, if the right conditions are met.  (This
    // should only be called by things "outside" of current_page_t, like
    // current_page_acq_t and page_txn_t, not page_t and page_ptr_t -- that way we
    // know it's not called while somebody up the stack is expecting their
    // `current_page_t *` to remain valid.)
    void consider_evicting_current_page(block_id_t block_id);

    void have_read_ahead_cb_destroyed();

    evicter_t &evicter() { return evicter_; }

    auto_drainer_t::lock_t drainer_lock() { return drainer_->lock(); }
    serializer_t *serializer() { return serializer_; }

private:
    void help_take_snapshotted_dirtied_page(
        current_page_t *cp, block_id_t block_id, page_txn_t *dirtier);

    friend class page_read_ahead_cb_t;
    void add_read_ahead_buf(block_id_t block_id,
                            scoped_device_block_aligned_ptr_t<ser_buffer_t> ptr,
                            const counted_t<block_token_t> &token);

    void read_ahead_cb_is_destroyed();


    current_page_t *internal_page_for_new_chosen(block_id_t block_id);

    struct collapsed_txns_t {
        auto_drainer_t::lock_t drainer_lock;
        throttler_acq_t acq;
        std::unordered_map<block_id_t, block_change_t> changes;
        intrusive_list_t<page_txn_complete_cb_t> flush_complete_waiters;
    };

    friend class page_txn_t;
    static std::vector<counted_t<block_token_t>> do_write_blocks(
        page_cache_t *page_cache,
        const std::vector<buf_write_info_t> &write_infos,
        state_timestamp_t our_write_number,
        ticks_t soft_deadline /* 0 is okay */);
    static void do_flush_changes(
        page_cache_t *page_cache,
        collapsed_txns_t *coltx,
        fifo_enforcer_write_token_t index_write_token,
        bool asap,
        ticks_t soft_deadline /* 0 is okay */);
    static void do_flush_txn_set(
        page_cache_t *page_cache,
        collapsed_txns_t *coltx_ptr,
        bool asap,
        ticks_t soft_deadline);

    static void remove_txn_set_from_graph(
        page_cache_t *page_cache,
        const std::vector<scoped_ptr_t<page_txn_t>> &txns);

    static void pulse_flush_complete(collapsed_txns_t &&txns);

    // We only pass the cache to reset the page ptr.
    static collapsed_txns_t
    compute_changes(page_cache_t *page_cache,
                    std::vector<scoped_ptr_t<page_txn_t>> &&txns);

    static MUST_USE int64_t merge_changes(
        page_cache_t *page_cache,
        std::unordered_map<block_id_t, block_change_t> *onto,
        std::unordered_map<block_id_t, block_change_t> &&from);

    static std::vector<scoped_ptr_t<page_txn_t>>
    maximal_flushable_txn_set(page_txn_t *base);

    void begin_waiting_for_flush(scoped_ptr_t<page_txn_t> &&txn,
                                 write_durability_t durability);

    // "asap = true" says to flush immediately, a hard durability transaction (or
    // otherwise high priority) is waiting.  flush_set must not be empty.
    void spawn_flush_flushables(std::vector<scoped_ptr_t<page_txn_t>> &&flush_set,
                                bool asap,
                                ticks_t soft_deadline);

    friend class current_page_acq_t;
    repli_timestamp_t recency_for_block_id(block_id_t id) {
        // This `if` is redundant, since `recencies_.size()` will always be smaller
        // than any aux block ID. It's probably a good idea to be explicit about this
        // though.
        if (is_aux_block_id(id)) {
            return repli_timestamp_t::invalid;
        }
        return recencies_.size() <= id
            ? repli_timestamp_t::invalid
            : recencies_[id];
    }

    void set_recency_for_block_id(block_id_t id, repli_timestamp_t recency) {
        if (is_aux_block_id(id)) {
            guarantee(recency == repli_timestamp_t::invalid);
            return;
        }
        while (recencies_.size() <= id) {
            recencies_.push_back(repli_timestamp_t::invalid);
        }
        recencies_[id] = recency;
    }

    friend class current_page_t;
    free_list_t *free_list() { return &free_list_; }

    static void consider_evicting_all_current_pages(page_cache_t *page_cache,
                                                    auto_drainer_t::lock_t lock);

    // Returns next_block_version_ and increments it.
    block_version_t gen_block_version();

    void merge_into_waiting_for_spawn_flush(scoped_ptr_t<page_txn_t> &&base);

    static flush_prep_t prep_flush_changes(
        page_cache_t *page_cache,
        const std::unordered_map<block_id_t, block_change_t> &changes);

    const max_block_size_t max_block_size_;

    // We use a separate I/O account for reads in each page cache.
    // Note that block writes use a shared I/O account that sits in the
    // merger_serializer_t (as long as you use one, otherwise they use the
    // default account).
    cache_account_t default_reads_account_;

    // This fifo enforcement pair ensures ordering of index_write operations after we
    // move to the serializer thread and get a bunch of blocks written.
    // index_write_sink's pointee's home thread is on the serializer.
    fifo_enforcer_source_t index_write_source_;
    // (Has nothing to do with repli_timestamp_t.)  The fifo_enforcer_write_token_t of
    // the most recently spawned "asap" write had this timestamp.  (If this is bigger
    // than an ongoing write, the ongoing write becomes "asap" too.)
    state_timestamp_t ser_thread_max_asap_write_token_timestamp_;
    // Number of flushes started, not yet completed, that are asap=false.
    bool num_active_asap_false_flushes_;

    scoped_ptr_t<page_cache_index_write_sink_t> index_write_sink_;

    serializer_t *serializer_;
    segmented_vector_t<repli_timestamp_t> recencies_;

    std::unordered_map<block_id_t, current_page_t *> current_pages_;

    // An incrementing 64-bit counter used to generate block versions, with values 1, 2,
    // 3, ...  This makes sure that block writes never get overwritten by previous block
    // writes.  alt_snapshot_node_t's will still hold a current_page_acq_t though --
    // this stops them from destroying and recreating a snapshot node, and getting
    // multiple copies of the same page.
    block_version_t next_block_version_;

    // The difference between these two lists is that waiting_for_spawn_flush_ txn's
    // might be capable of flushing.  want_to_spawn_flush_ txn's can't disconnect from
    // the graph and flush just yet.

    // Txns that have began_waiting_for_flush_ true, spawned_flush_ false,
    // pre_spawn_flush_ false.  Right now this has 0 or 1 elements because we merge
    // _all_ the page_txn's together.
    intrusive_list_t<page_txn_t> waiting_for_spawn_flush_;
    // Txns that have began_waiting_for_flush_ true, spawned_flush_ false,
    // pre_spawn_flush_ true.
    intrusive_list_t<page_txn_t> want_to_spawn_flush_;

    free_list_t free_list_;

    evicter_t evicter_;

    // KSI: I bet this read_ahead_cb_ and read_ahead_cb_existence_ type could be
    // packaged in some new cross_thread_ptr type.
    page_read_ahead_cb_t *read_ahead_cb_;

    // Holds a lock on *drainer_ is until shortly after the page_read_ahead_cb_t is
    // destroyed and all possible read-ahead operations have completed.
    auto_drainer_t::lock_t read_ahead_cb_existence_;

    scoped_ptr_t<auto_drainer_t> drainer_;

    DISABLE_COPYING(page_cache_t);
};

// page_txn_t's exist for the purpose of writing to disk.  The rules are as follows:
//
//  - When a page_txn_t gets "committed" (written to disk), all blocks modified with
//    a given page_txn_t must be committed to disk at the same time.  (That is, they
//    all go in the same index_write operation.)
//
//  - For all blocks N and page_txn_t S and T, if S modifies N before T modifies N,
//    then S must be committed to disk before or at the same time as T.
//
//  - For all page_txn_t S and T, if S is the preceding_txn of T then S must be
//    committed to disk before or at the same time as T.
//
// As a result, we form a graph of txns, which gets modified on the fly, and we
// commit them in topological order.  Cycles can happen (for example, if (a) two
// transactions modify the same physical memory, or (b) they modify blocks in
// opposite orders), and transactions that depend on one another (forming a cycle)
// must get flushed simultaneously (in the same serializer->index_write operation).
// Situation '(a)' can happen as a matter of course, assuming transactions don't
// greedily save their modified copy of a page.  Situation '(b)' can happen if
// transactions apply a commutative operation on a block, like with the stats block.
// Right now, situation '(a)' doesn't happen because transactions do greedily keep
// their copies of the block.
//
// HSI: Check this comment.
class page_txn_t : public intrusive_list_node_t<page_txn_t> {
public:
    // Our transaction has to get committed to disk _after_ or at the same time as
    // preceding transactions on cache_conn, if that parameter is not NULL.  (The
    // parameter's NULL for read txns, for now.)
    page_txn_t(page_cache_t *page_cache,
               throttler_acq_t throttler_acq,
               cache_conn_t *cache_conn);

    // KSI: This is only to be called by the page cache -- should txn_t really use a
    // scoped_ptr_t?
    ~page_txn_t();

    page_cache_t *page_cache() const { return page_cache_; }

private:
    // To set cache_conn_ to NULL.
    friend class ::cache_conn_t;

    // page cache has access to all of this type's innards, including fields.
    friend class page_cache_t;

    // To access `pages_write_acquired_last_` and `connect_preceder()`.
    friend class current_page_t;

    // Adds and connects a preceder.
    void connect_preceder(page_txn_t *preceder);

    // Removes a preceder, which is already half-way disconnected.
    void remove_preceder(page_txn_t *preceder);

    // Removes a subseqer, which is already half-way disconnected.
    void remove_subseqer(page_txn_t *subseqer);

    // current_page_acq should only call add_acquirer and remove_acquirer.
    friend class current_page_acq_t;
    void add_acquirer(current_page_acq_t *acq);
    void remove_acquirer(current_page_acq_t *acq);

    // Sets base->pre_spawn_flush_ to true, and propagates to preceders.
    static void propagate_pre_spawn_flush(page_txn_t *base);

    size_t dirtied_page_count() const {
        return pages_dirtied_last_.size() + dirty_changes_pages_;
    }

    // Sets pre_spawn_flush to be true, if not already set.  handles presence in
    // waiting_for_spawn_flush_/want_to_spawn_flush_.  Returns false if pre_spawn_flush
    // was already set, in which case there was no side effect.
    bool set_pre_spawn_flush();

    // If ptr is empty, that means the block was deleted.
    void add_snapshotted_dirtied_page(
        block_id_t block_id, block_version_t version, repli_timestamp_t tstamp,
        page_ptr_t &&ptr);
    void add_touched_page(
        block_id_t block_id, block_version_t version, repli_timestamp_t tstamp);

    // Destroys other, merges it into this page txn.
    void merge(scoped_ptr_t<page_txn_t> &&other);

    auto_drainer_t::lock_t drainer_lock_;

    page_cache_t *page_cache_;
    // Contains cache_conn_t's for which we are the newest txn.
    half_intrusive_list_t<cache_conn_t> cache_conns_;

    // An acquisition object for the memory tracker.
    throttler_acq_t throttler_acq_;

    // page_txn_t's form a directed graph.  preceders_ and subseqers_ represent the
    // inward-pointing and outward-pointing arrows.  (I'll let you decide which
    // direction should be inward and which should be outward.)  Each page_txn_t
    // pointed at by subseqers_ has an entry in its preceders_ that is back-pointing
    // at this page_txn_t.  (And vice versa for each page_txn_t pointed at by
    // preceders_.)

    // PERFORMANCE(preceders_): PERFORMANCE(subseqers_):
    //
    // Performance on operations linear in the number of preceders_ and subseqers_
    // should be _okay_ in any case, because we throttle transactions based on the
    // number of dirty blocks.  But also, the number of preceders_ and subseqers_
    // would generally be very low, because relationships for users of the same block
    // or cache connection form a chain, not a giant clique.

    // The transactions that must be committed before or at the same time as this
    // transaction.
    std::vector<page_txn_t *> preceders_;

    // txn's that we precede -- preceders_[i]->subseqers_ always contains us once.
    std::vector<page_txn_t *> subseqers_;

    // Pages for which this page_txn_t is the last_write_acquirer_ of that page.  We
    // wouldn't mind a std::vector inside the backindex_bag_t, but it's a
    // segmented_vector_t -- we give it a segment size big enough to not be obnoxious
    // about memory usage.
    backindex_bag_t<current_page_t *, 16> pages_write_acquired_last_;

    // Pages for which this page_txn_t is the last_dirtier_ of that page.
    backindex_bag_t<current_page_dirtier_t, 16> pages_dirtied_last_;

    // How many current_page_acq_t's for this transaction that are currently alive.
    size_t live_acqs_;

    // You have to update throttler_acq_ when you add/remove from this.
    std::unordered_map<block_id_t, block_change_t> changes_;
    // How many pages are held by changes_.  (Always non-negative.)
    int64_t dirty_changes_pages_;

    // Valid states of (began_waiting_for_flush_, pre_spawn_flush_, spawned_flush_) are:
    //   (    *,     *, false)
    //   ( true,  true,  true)

    // Tells whether this page_txn_t has announced itself (to the cache) to be
    // waiting for a flush.
    bool began_waiting_for_flush_;

    // We "want" to spawn a flush, so you shouldn't gratuitously add any preceders by
    // stealing dirtied pages.  If this is true, then it must also be true for any
    // preceders.
    // bool pre_spawn_flush_;
    // Actually, this field is stored in throttler_acq_.pre_spawn_flush_.

    // spawned_flush_ gets set true when we have removed the txn from the graph (just
    // before we actually start flushing).
    bool spawned_flush_;

    enum mark_state_t : uint8_t {
        marked_not,
        marked_red,
        marked_blue,
        marked_green,
    };
    // Always `marked_not`, except temporarily, during ASSERT_NO_CORO_WAITING graph
    // algorithms.
    mark_state_t mark_;

    intrusive_list_t<page_txn_complete_cb_t> flush_complete_waiters_;

    DISABLE_COPYING(page_txn_t);
};

}  // namespace alt


#endif  // BUFFER_CACHE_PAGE_CACHE_HPP_
