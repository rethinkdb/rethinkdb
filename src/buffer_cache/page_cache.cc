// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "buffer_cache/page_cache.hpp"

#include <algorithm>
#include <functional>
#include <iterator>
#include <stack>

#include "arch/runtime/coroutines.hpp"
#include "arch/runtime/runtime.hpp"
#include "arch/runtime/runtime_utils.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/new_mutex.hpp"
#include "buffer_cache/cache_balancer.hpp"
#include "do_on_thread.hpp"
#include "serializer/serializer.hpp"
#include "stl_utils.hpp"

cache_conn_t::~cache_conn_t() {
    // The user could only be expected to make sure that txn_t objects don't have
    // their lifetime exceed the cache_conn_t's.  Soft durability makes it possible
    // that the inner page_txn_t's lifetime would exceed the cache_conn_t's.  So we
    // need to tell the page_txn_t that we don't exist -- we do so by nullating its
    // cache_conn_ pointer (which it's capable of handling).
    if (newest_txn_ != nullptr) {
        newest_txn_->cache_conns_.remove(this);
        newest_txn_ = nullptr;
    }
}


namespace alt {

class current_page_help_t {
public:
    current_page_help_t(block_id_t _block_id, page_cache_t *_page_cache)
        : block_id(_block_id), page_cache(_page_cache) { }
    block_id_t block_id;
    page_cache_t *page_cache;
};

void throttler_acq_t::update_dirty_page_count(int64_t new_count) {
    rassert(
        block_changes_semaphore_acq_.count() == index_changes_semaphore_acq_.count());
    new_count = std::max<int64_t>(new_count, expected_change_count_);
    if (pre_spawn_flush_ && new_count > block_changes_semaphore_acq_.count()
        && !prevent_updates_) {
        block_changes_semaphore_acq_.change_count(new_count);
        index_changes_semaphore_acq_.change_count(new_count);
    }
}

void throttler_acq_t::mark_dirty_pages_written() {
    block_changes_semaphore_acq_.change_count(0);
}

void throttler_acq_t::set_prevent_updates() {
    prevent_updates_ = true;
}


void throttler_acq_t::merge(throttler_acq_t &&other) {
    expected_change_count_ += other.expected_change_count_;
    other.expected_change_count_ = 0;
    // No need to worry about propagating pre_spawn_flush_.  (The two places we call
    // this are for txn's that could belong in waiting_for_spawn_flush_, and txn's in
    // compute_changes.)
    rassert(pre_spawn_flush_ == other.pre_spawn_flush_);

    // If a soft durability txn is flushing in combination with a hard durability txn, I
    // guess we'll count its pages... if any more are acquired.
    prevent_updates_ &= other.prevent_updates_;

    if (!has_txn_throttler()) {
        block_changes_semaphore_acq_ = std::move(other.block_changes_semaphore_acq_);
        index_changes_semaphore_acq_ = std::move(other.index_changes_semaphore_acq_);
    } else if (other.has_txn_throttler()) {
        block_changes_semaphore_acq_.transfer_in(
            std::move(other.block_changes_semaphore_acq_));
        index_changes_semaphore_acq_.transfer_in(
            std::move(other.index_changes_semaphore_acq_));
    }
}

page_read_ahead_cb_t::page_read_ahead_cb_t(serializer_t *serializer,
                                           page_cache_t *page_cache)
    : serializer_(serializer), page_cache_(page_cache) {
    serializer_->register_read_ahead_cb(this);
}

page_read_ahead_cb_t::~page_read_ahead_cb_t() { }

void page_read_ahead_cb_t::offer_read_ahead_buf(
        block_id_t block_id,
        buf_ptr_t *buf,
        const counted_t<block_token_t> &token) {
    assert_thread();
    buf_ptr_t local_buf = std::move(*buf);

    block_size_t block_size = block_size_t::undefined();
    scoped_device_block_aligned_ptr_t<ser_buffer_t> ptr;
    local_buf.release(&block_size, &ptr);

    // We're going to reconstruct the buf_ptr_t on the other side of this do_on_thread
    // call, so we'd better make sure the block size is right.
    guarantee(block_size == token->block_size());

    // Notably, this code relies on do_on_thread to preserve callback order (which it
    // does do).
    do_on_thread(page_cache_->home_thread(),
                 std::bind(&page_cache_t::add_read_ahead_buf,
                           page_cache_,
                           block_id,
                           copyable_unique_t<scoped_device_block_aligned_ptr_t<ser_buffer_t> >(std::move(ptr)),
                           token));
}

void page_read_ahead_cb_t::destroy_self() {
    serializer_->unregister_read_ahead_cb(this);
    serializer_ = nullptr;

    page_cache_t *page_cache = page_cache_;
    page_cache_ = nullptr;

    do_on_thread(page_cache->home_thread(),
                 std::bind(&page_cache_t::read_ahead_cb_is_destroyed, page_cache));

    // Self-deletion.  Old-school.
    delete this;
}

void page_cache_t::consider_evicting_current_page(block_id_t block_id) {
    ASSERT_NO_CORO_WAITING;
    // We can't do anything until read-ahead is done, because it uses the existence
    // of a current_page_t entry to figure out whether the read-ahead page could be
    // out of date.
    if (read_ahead_cb_ != nullptr) {
        return;
    }

    auto page_it = current_pages_.find(block_id);
    if (page_it == current_pages_.end()) {
        return;
    }

    current_page_t *page_ptr = page_it->second;
    if (page_ptr->should_be_evicted()) {
        current_pages_.erase(block_id);
        page_ptr->reset(this);
        delete page_ptr;
    }
}

block_version_t page_cache_t::gen_block_version() {
    block_version_t ret = next_block_version_;
    next_block_version_ = next_block_version_.subsequent();
    return ret;
}



void page_cache_t::add_read_ahead_buf(block_id_t block_id,
                                      scoped_device_block_aligned_ptr_t<ser_buffer_t> ptr,
                                      const counted_t<block_token_t> &token) {
    assert_thread();

    // We MUST stop if read_ahead_cb_ is NULL because that means current_page_t's
    // could start being destroyed.
    if (read_ahead_cb_ == nullptr) {
        return;
    }

    // We MUST stop if current_pages_[block_id] already exists, because that means
    // the read-ahead page might be out of date.
    if (current_pages_.count(block_id) > 0) {
        return;
    }

    // We know the read-ahead page is not out of date if current_pages_[block_id]
    // doesn't exist and if read_ahead_cb_ still exists -- that means a current_page_t
    // for the block id was never created, and thus the page could not have been
    // modified (not to mention that we've already got the page in memory, so there is
    // no useful work to be done).

    buf_ptr_t buf(token->block_size(), std::move(ptr));
    current_pages_[block_id] = new current_page_t(block_id, std::move(buf), token, this);
}

void page_cache_t::have_read_ahead_cb_destroyed() {
    assert_thread();

    if (read_ahead_cb_ != nullptr) {
        // By setting read_ahead_cb_ to nullptr, we make sure we only tell the read
        // ahead cb to destroy itself exactly once.
        page_read_ahead_cb_t *cb = read_ahead_cb_;
        read_ahead_cb_ = nullptr;

        do_on_thread(cb->home_thread(),
                     std::bind(&page_read_ahead_cb_t::destroy_self, cb));

        coro_t::spawn_sometime(std::bind(&page_cache_t::consider_evicting_all_current_pages, this, drainer_->lock()));
    }
}

void page_cache_t::consider_evicting_all_current_pages(page_cache_t *page_cache,
                                                       auto_drainer_t::lock_t lock) {
    // Atomically grab a list of block IDs that currently exist in current_pages.
    std::vector<block_id_t> current_block_ids;
    current_block_ids.reserve(page_cache->current_pages_.size());
    for (const auto &current_page : page_cache->current_pages_) {
        current_block_ids.push_back(current_page.first);
    }

    // In a separate step, evict current pages that should be evicted.
    // We do this separately so that we can yield between evictions.
    size_t i = 0;
    for (block_id_t id : current_block_ids) {
        page_cache->consider_evicting_current_page(id);
        if (i % 16 == 15) {
            coro_t::yield();
            if (lock.get_drain_signal()->is_pulsed()) {
                return;
            }
        }
        ++i;
    }
}


void page_cache_t::read_ahead_cb_is_destroyed() {
    assert_thread();
    read_ahead_cb_existence_.reset();
}

class page_cache_index_write_sink_t {
public:
    // When sink is acquired, we get in line for mutex_ right away and release the
    // sink.  The serializer_t interface uses new_mutex_t.
    fifo_enforcer_sink_t sink;
    new_mutex_t mutex;
};

page_cache_t::page_cache_t(serializer_t *_serializer,
                           cache_balancer_t *balancer,
                           alt_txn_throttler_t *throttler)
    : max_block_size_(_serializer->max_block_size()),
      serializer_(_serializer),
      // Start the counter at 1 so we can distinguish empty values.
      next_block_version_(block_version_t().subsequent()),
      free_list_(_serializer),
      evicter_(),
      read_ahead_cb_(nullptr),
      drainer_(make_scoped<auto_drainer_t>()) {

    const bool start_read_ahead = balancer->read_ahead_ok_at_start();
    if (start_read_ahead) {
        read_ahead_cb_existence_ = drainer_->lock();
    }

    page_read_ahead_cb_t *local_read_ahead_cb = nullptr;
    {
        on_thread_t thread_switcher(_serializer->home_thread());
        if (start_read_ahead) {
            local_read_ahead_cb = new page_read_ahead_cb_t(_serializer, this);
        }
        default_reads_account_.init(_serializer->home_thread(),
                                    _serializer->make_io_account(CACHE_READS_IO_PRIORITY));
        index_write_sink_.init(new page_cache_index_write_sink_t);
        recencies_ = _serializer->get_all_recencies();
    }

    ASSERT_NO_CORO_WAITING;
    // We don't want to accept read-ahead buffers (or any operations) until the
    // evicter is ready.  So we set read_ahead_cb_ here so that we accept read-ahead
    // buffers at exactly the same time that we initialize the evicter.  We
    // initialize the read_ahead_cb_ after the evicter_ because that way reentrant
    // usage by the balancer (before page_cache_t construction completes) would be
    // more likely to trip an assertion.
    evicter_.initialize(this, balancer, throttler);
    read_ahead_cb_ = local_read_ahead_cb;
}

page_cache_t::~page_cache_t() {
    assert_thread();

    have_read_ahead_cb_destroyed();

    // Flush all pending soft-durability transactions.  All txn's must have had
    // flush_and_destroy_txn called on them before we entered this destructor, so we
    // know the entire set of txn's will be flushed.
    begin_flush_pending_txns(true, ticks_t{0});

    drainer_.reset();
    size_t i = 0;
    for (auto &&page : current_pages_) {
        if (i % 256 == 255) {
            coro_t::yield();
        }
        ++i;
        page.second->reset(this);
        delete page.second;
    }

    {
        /* IO accounts and a few other fields must be destroyed on the serializer
        thread. */
        on_thread_t thread_switcher(serializer_->home_thread());
        // Resetting default_reads_account_ is opportunistically done here, instead
        // of making its destructor switch back to the serializer thread a second
        // time.
        default_reads_account_.reset();
        index_write_sink_.reset();
    }
}

void page_cache_t::begin_flush_pending_txns(bool asap, ticks_t soft_deadline) {
    ASSERT_FINITE_CORO_WAITING;
    std::vector<scoped_ptr_t<page_txn_t>> full_flush_set;

    if (!asap) {
        for (page_txn_t *ptr = waiting_for_spawn_flush_.head(); ptr != nullptr;
             ptr = waiting_for_spawn_flush_.next(ptr)) {
            ptr->throttler_acq_.set_prevent_updates();
        }
    }

    while (page_txn_t *ptr = waiting_for_spawn_flush_.head()) {
        page_txn_t::propagate_pre_spawn_flush(ptr);
        std::vector<scoped_ptr_t<page_txn_t>> flush_set
            = page_cache_t::maximal_flushable_txn_set(ptr);
        page_cache_t::remove_txn_set_from_graph(this, flush_set);
        std::move(flush_set.begin(), flush_set.end(),
                  std::back_inserter(full_flush_set));
    }
    if (!full_flush_set.empty()) {
        spawn_flush_flushables(std::move(full_flush_set), asap, soft_deadline);
    }
}

void page_cache_t::soft_durability_interval_flush(ticks_t soft_deadline) {
    // We only start a soft durability flush if one isn't already running.
    if (num_active_asap_false_flushes_ == 0) {
        begin_flush_pending_txns(false, soft_deadline);
    }
}

void page_cache_t::flush_and_destroy_txn(
        scoped_ptr_t<page_txn_t> &&txn,
        write_durability_t durability,
        page_txn_complete_cb_t *on_complete_or_null) {
    guarantee(txn->live_acqs_ == 0,
              "A current_page_acq_t lifespan exceeds its page_txn_t's.");
    guarantee(!txn->began_waiting_for_flush_);

    rassert(txn->live_acqs_ == 0);
    rassert(!txn->spawned_flush_);

    if (on_complete_or_null != nullptr) {
        txn->flush_complete_waiters_.push_front(on_complete_or_null);
    }

    begin_waiting_for_flush(std::move(txn), durability);
}

void page_cache_t::end_read_txn(scoped_ptr_t<page_txn_t> txn) {
    guarantee(txn->changes_.empty());
    guarantee(txn->live_acqs_ == 0,
        "A current_page_acq_t lifespan exceeds its page_txn_t's.");
    guarantee(!txn->began_waiting_for_flush_);
}


current_page_t *page_cache_t::page_for_block_id(block_id_t block_id) {
    assert_thread();

    auto page_it = current_pages_.find(block_id);
    if (page_it == current_pages_.end()) {
        rassert(is_aux_block_id(block_id) ||
                recency_for_block_id(block_id) != repli_timestamp_t::invalid,
                "Expected block %" PR_BLOCK_ID " not to be deleted "
                "(should you have used alt_create_t::create?).",
                block_id);
        page_it = current_pages_.insert(
            page_it, std::make_pair(block_id, new current_page_t(block_id, this)));
    } else {
        rassert(!page_it->second->is_deleted());
    }

    return page_it->second;
}

current_page_t *page_cache_t::page_for_new_block_id(
        block_type_t block_type,
        block_id_t *block_id_out) {
    assert_thread();
    block_id_t block_id;
    switch (block_type) {
    case block_type_t::aux:
        block_id = free_list_.acquire_aux_block_id();
        break;
    case block_type_t::normal:
        block_id = free_list_.acquire_block_id();
        break;
    default:
        unreachable();
    }
    current_page_t *ret = internal_page_for_new_chosen(block_id);
    *block_id_out = block_id;
    return ret;
}

current_page_t *page_cache_t::page_for_new_chosen_block_id(block_id_t block_id) {
    assert_thread();
    // Tell the free list this block id is taken.
    free_list_.acquire_chosen_block_id(block_id);
    return internal_page_for_new_chosen(block_id);
}

current_page_t *page_cache_t::internal_page_for_new_chosen(block_id_t block_id) {
    assert_thread();
    rassert(is_aux_block_id(block_id) ||
            recency_for_block_id(block_id) == repli_timestamp_t::invalid,
            "expected chosen block %" PR_BLOCK_ID "to be deleted", block_id);
    if (!is_aux_block_id(block_id)) {
        set_recency_for_block_id(block_id, repli_timestamp_t::distant_past);
    }

    buf_ptr_t buf = buf_ptr_t::alloc_uninitialized(max_block_size_);

#if !defined(NDEBUG) || defined(VALGRIND)
    // KSI: This should actually _not_ exist -- we are ignoring legitimate errors
    // where we write uninitialized data to disk.
    memset(buf.cache_data(), 0xCD, max_block_size_.value());
#endif

    auto inserted_page = current_pages_.insert(std::make_pair(
        block_id, new current_page_t(block_id, std::move(buf), this)));
    guarantee(inserted_page.second);

    return inserted_page.first->second;
}

cache_account_t page_cache_t::create_cache_account(int priority) {
    // We assume that a priority of 100 means that the transaction should have the
    // same priority as all the non-accounted transactions together. Not sure if this
    // makes sense.

    // Be aware of rounding errors... (what can be do against those? probably just
    // setting the default io_priority_reads high enough)
    int io_priority = std::max(1, CACHE_READS_IO_PRIORITY * priority / 100);

    // TODO: This is a heuristic. While it might not be evil, it's not really optimal
    // either.
    int outstanding_requests_limit = std::max(1, 16 * priority / 100);

    file_account_t *io_account;
    {
        // Ideally we shouldn't have to switch to the serializer thread.  But that's
        // what the file account API is right now, deep in the I/O layer.
        on_thread_t thread_switcher(serializer_->home_thread());
        io_account = serializer_->make_io_account(io_priority,
                                                  outstanding_requests_limit);
    }

    return cache_account_t(serializer_->home_thread(), io_account);
}


current_page_acq_t::current_page_acq_t()
    : page_cache_(nullptr), the_txn_(nullptr) { }

current_page_acq_t::current_page_acq_t(page_txn_t *txn,
                                       block_id_t _block_id,
                                       access_t _access,
                                       page_create_t create)
    : page_cache_(nullptr), the_txn_(nullptr) {
    init(txn, _block_id, _access, create);
}

current_page_acq_t::current_page_acq_t(page_txn_t *txn,
                                       alt_create_t create,
                                       block_type_t block_type)
    : page_cache_(nullptr), the_txn_(nullptr) {
    init(txn, create, block_type);
}

current_page_acq_t::current_page_acq_t(page_cache_t *_page_cache,
                                       block_id_t _block_id,
                                       read_access_t read)
    : page_cache_(nullptr), the_txn_(nullptr) {
    init(_page_cache, _block_id, read);
}

void current_page_acq_t::init(page_txn_t *txn,
                              block_id_t _block_id,
                              access_t _access,
                              page_create_t create) {
    if (_access == access_t::read) {
        rassert(create == page_create_t::no);
        init(txn->page_cache(), _block_id, read_access_t::read);
    } else {
        txn->page_cache()->assert_thread();
        guarantee(page_cache_ == nullptr);
        page_cache_ = txn->page_cache();
        the_txn_ = (_access == access_t::write ? txn : nullptr);
        access_ = _access;
        declared_snapshotted_ = false;
        block_id_ = _block_id;
        if (create == page_create_t::yes) {
            current_page_ = page_cache_->page_for_new_chosen_block_id(_block_id);
        } else {
            current_page_ = page_cache_->page_for_block_id(_block_id);
        }
        dirtied_page_ = false;
        touched_page_ = false;

        the_txn_->add_acquirer(this);
        current_page_->add_acquirer(this);
    }
}

void current_page_acq_t::init(page_txn_t *txn,
                              alt_create_t,
                              block_type_t block_type) {
    txn->page_cache()->assert_thread();
    guarantee(page_cache_ == nullptr);
    page_cache_ = txn->page_cache();
    the_txn_ = txn;
    access_ = access_t::write;
    declared_snapshotted_ = false;
    current_page_ = page_cache_->page_for_new_block_id(block_type, &block_id_);
    dirtied_page_ = false;
    touched_page_ = false;

    the_txn_->add_acquirer(this);
    current_page_->add_acquirer(this);
}

void current_page_acq_t::init(page_cache_t *_page_cache,
                              block_id_t _block_id,
                              read_access_t) {
    _page_cache->assert_thread();
    guarantee(page_cache_ == nullptr);
    page_cache_ = _page_cache;
    the_txn_ = nullptr;
    access_ = access_t::read;
    declared_snapshotted_ = false;
    block_id_ = _block_id;
    current_page_ = page_cache_->page_for_block_id(_block_id);
    dirtied_page_ = false;
    touched_page_ = false;

    current_page_->add_acquirer(this);
}

current_page_acq_t::~current_page_acq_t() {
    assert_thread();
    // Checking page_cache_ != nullptr makes sure this isn't a default-constructed acq.
    if (page_cache_ != nullptr) {
        if (the_txn_ != nullptr) {
            guarantee(access_ == access_t::write);
            the_txn_->remove_acquirer(this);
        }
        rassert(current_page_ != nullptr);
        if (in_a_list()) {
            // Note that the current_page_acq can be in the current_page_ acquirer
            // list and still be snapshotted. However it will not have a
            // snapshotted_page_.
            rassert(!snapshotted_page_.has());
            current_page_->remove_acquirer(this);
        }
        if (declared_snapshotted_) {
            snapshotted_page_.reset_page_ptr(page_cache_);
            current_page_->remove_keepalive();
        }
        page_cache_->consider_evicting_current_page(block_id_);
    }
}

void current_page_acq_t::declare_readonly() {
    assert_thread();
    access_ = access_t::read;
    if (current_page_ != nullptr) {
        current_page_->pulse_pulsables(this);
    }
}

void current_page_acq_t::declare_snapshotted() {
    assert_thread();
    rassert(access_ == access_t::read);

    // Allow redeclaration of snapshottedness.
    if (!declared_snapshotted_) {
        declared_snapshotted_ = true;
        rassert(current_page_ != nullptr);
        current_page_->add_keepalive();
        current_page_->pulse_pulsables(this);
    }
}

signal_t *current_page_acq_t::read_acq_signal() {
    assert_thread();
    return &read_cond_;
}

signal_t *current_page_acq_t::write_acq_signal() {
    assert_thread();
    rassert(access_ == access_t::write);
    return &write_cond_;
}

page_t *current_page_acq_t::current_page_for_read(cache_account_t *account) {
    assert_thread();
    rassert(snapshotted_page_.has() || current_page_ != nullptr);
    read_cond_.wait();
    if (snapshotted_page_.has()) {
        return snapshotted_page_.get_page_for_read();
    }
    rassert(current_page_ != nullptr);
    return current_page_->the_page_for_read(help(), account);
}

repli_timestamp_t current_page_acq_t::recency() {
    assert_thread();
    rassert(snapshotted_page_.has() || current_page_ != nullptr);

    // We wait for write_cond_ when getting the recency (if we're a write acquirer)
    // so that we can't see the recency change before/after the write_cond_ is
    // pulsed.
    if (access_ == access_t::read) {
        read_cond_.wait();
    } else {
        write_cond_.wait();
    }

    if (snapshotted_page_.has()) {
        return snapshotted_page_.timestamp();
    }
    rassert(current_page_ != nullptr);
    return page_cache_->recency_for_block_id(block_id_);
}

// Doesn't do everything -- caller will need to disconnect dirtier->pages_dirtied_last_.
void page_cache_t::help_take_snapshotted_dirtied_page(
        current_page_t *cp, block_id_t block_id, page_txn_t *dirtier) {
    rassert(cp->last_dirtier_ == dirtier);
    dirtier->add_snapshotted_dirtied_page(
        block_id, cp->last_dirtier_version_,
        cp->last_dirtier_recency_,
        page_ptr_t(cp->the_page_for_read_or_deleted(current_page_help_t(block_id, this))));
}

void current_page_acq_t::dirty_the_page() {
    dirtied_page_ = true;
    page_txn_t *prec = current_page_->last_dirtier_;
    if (prec != the_txn_) {

        if (prec != nullptr) {
            prec->pages_dirtied_last_.remove(current_page_dirtier_t{current_page_});
            if (prec->throttler_acq_.pre_spawn_flush()) {
                page_cache_->help_take_snapshotted_dirtied_page(
                    current_page_, block_id_, prec);
            } else {
                // prec is already a preceder of the_txn_, transitively.  Now prec is a
                // subseqer too, and we have to flush them at the same time.  This is
                // fitting and proper because prec has no snapshot of its buf to flush.
                prec->connect_preceder(the_txn_);
            }
        }
        // We increase the_txn_'s dirty_page_count(), so we update its throttler_acq_
        // first, before we update prec's (which may decrease back down).
        the_txn_->pages_dirtied_last_.add(current_page_dirtier_t{current_page_});
        the_txn_->throttler_acq_.update_dirty_page_count(the_txn_->dirtied_page_count());
        if (prec != nullptr) {
            prec->throttler_acq_.update_dirty_page_count(prec->dirtied_page_count());
        }
    }
    current_page_->last_dirtier_ = the_txn_;
    current_page_->last_dirtier_recency_ = page_cache_->recency_for_block_id(block_id_);
    current_page_->last_dirtier_version_ = block_version_;
}

page_t *current_page_acq_t::current_page_for_write(cache_account_t *account) {
    assert_thread();
    rassert(access_ == access_t::write);
    rassert(current_page_ != nullptr);
    write_cond_.wait();
    rassert(current_page_ != nullptr);
    dirty_the_page();
    return current_page_->the_page_for_write(help(), account);
}

void current_page_acq_t::set_recency(repli_timestamp_t _recency) {
    assert_thread();
    rassert(access_ == access_t::write);
    rassert(current_page_ != nullptr);
    write_cond_.wait();
    rassert(current_page_ != nullptr);
    touched_page_ = true;
    page_cache_->set_recency_for_block_id(block_id_, _recency);
    if (current_page_->last_dirtier_ == the_txn_) {
        current_page_->last_dirtier_recency_ = _recency;
    }
}

void current_page_acq_t::mark_deleted() {
    assert_thread();
    rassert(access_ == access_t::write);
    rassert(current_page_ != nullptr);
    write_cond_.wait();
    rassert(current_page_ != nullptr);
    dirty_the_page();
    current_page_->mark_deleted(help());
    // HSI: This is gross and fragile that we need this knowledge here.
    if (current_page_->last_dirtier_ == the_txn_) {
        current_page_->last_dirtier_recency_ = repli_timestamp_t::invalid;
    }
    // No need to call consider_evicting_current_page here -- there's a
    // current_page_acq_t for it: ourselves.
}

bool current_page_acq_t::dirtied_page() const {
    assert_thread();
    return dirtied_page_;
}

bool current_page_acq_t::touched_page() const {
    assert_thread();
    return touched_page_;
}

current_page_help_t current_page_acq_t::help() const {
    assert_thread();
    return current_page_help_t(block_id(), page_cache_);
}

void current_page_acq_t::pulse_read_available() {
    assert_thread();
    read_cond_.pulse_if_not_already_pulsed();
}

void current_page_acq_t::pulse_write_available() {
    assert_thread();
    write_cond_.pulse_if_not_already_pulsed();
}

current_page_t::current_page_t(block_id_t block_id, page_cache_t *page_cache)
    : block_id_(block_id),
      is_deleted_(false),
      last_write_acquirer_(nullptr),
      last_write_acquirer_version_(page_cache->gen_block_version()),
      last_dirtier_(nullptr),
      num_keepalives_(0) { }

current_page_t::current_page_t(block_id_t block_id,
                               buf_ptr_t buf,
                               page_cache_t *page_cache)
    : block_id_(block_id),
      page_(new page_t(block_id, std::move(buf), page_cache)),
      is_deleted_(false),
      last_write_acquirer_(nullptr),
      last_write_acquirer_version_(page_cache->gen_block_version()),
      last_dirtier_(nullptr),
      num_keepalives_(0) { }

current_page_t::current_page_t(block_id_t block_id,
                               buf_ptr_t buf,
                               const counted_t<block_token_t> &token,
                               page_cache_t *page_cache)
    : block_id_(block_id),
      page_(new page_t(block_id, std::move(buf), token, page_cache)),
      is_deleted_(false),
      last_write_acquirer_(nullptr),
      last_write_acquirer_version_(page_cache->gen_block_version()),
      last_dirtier_(nullptr),
      num_keepalives_(0) { }

current_page_t::~current_page_t() {
    // Check that reset() has been called.
    rassert(last_write_acquirer_version_.debug_value() == 0);

    // An imperfect sanity check.
    rassert(!page_.has());
    rassert(num_keepalives_ == 0);
}

void current_page_t::reset(page_cache_t *page_cache) {
    rassert(acquirers_.empty());
    rassert(num_keepalives_ == 0);

    // last_write_acquirer_ has to be null (flush started) so that we don't lose track
    // of our in-memory block_version_t values that track which version of a buf is
    // newer in compute_changes.  current_page_t::should_be_evicted tests for this being
    // null.
    rassert(last_write_acquirer_ == nullptr);

    // HSI: Should this be null, or might it need to snapshot the dirtied page?
    rassert(last_dirtier_ == nullptr);

    page_.reset_page_ptr(page_cache);
    // No need to call consider_evicting_current_page here -- we're already getting
    // destructed.

    // For the sake of the ~current_page_t assertion.
    last_write_acquirer_version_ = block_version_t();

    if (is_deleted_ && block_id_ != NULL_BLOCK_ID) {
        page_cache->free_list()->release_block_id(block_id_);
        block_id_ = NULL_BLOCK_ID;
    }
}

bool current_page_t::should_be_evicted() const {
    // Consider reasons why the current_page_t should not be evicted.

    // A reason: It still has acquirers.  (Important.)
    if (!acquirers_.empty()) {
        return false;
    }

    // A reason: We still have a connection to last_write_acquirer_.  (Important.)
    if (last_write_acquirer_ != nullptr) {
        return false;
    }

    // A reason: We have a last dirtier.
    if (last_dirtier_ != nullptr) {
        return false;
    }

    // A reason: The current_page_t has snapshotted ex-acquirers.  (Important.)
    if (num_keepalives_ > 0) {
        return false;
    }

    // A reason: Its page_t isn't evicted, or has other snapshotters or waiters
    // anyway.  (Getting this wrong can only hurt performance.  We want to evict
    // current_page_t's with unloaded, otherwise unused page_t's.)
    if (page_.has()) {
        page_t *page = page_.get_page_for_read();
        if (page->is_loading() || page->has_waiters() || page->is_loaded()
            || page->page_ptr_count() != 1) {
            return false;
        }
        // is_loading is false and is_loaded is false -- it must be disk-backed.
        rassert(page->is_disk_backed() || page->is_deferred_loading());
    }

    return true;
}

void current_page_t::add_acquirer(current_page_acq_t *acq) {
    const block_version_t prev_version = last_write_acquirer_version_;

    if (acq->access_ == access_t::write) {
        block_version_t v = acq->page_cache_->gen_block_version();
        acq->block_version_ = v;

        rassert(acq->the_txn_ != nullptr);
        page_txn_t *const acq_txn = acq->the_txn_;

        last_write_acquirer_version_ = v;

        if (last_write_acquirer_ != acq_txn) {
            rassert(!acq_txn->pages_write_acquired_last_.has_element(this));

            if (last_write_acquirer_ != nullptr) {
                page_txn_t *prec = last_write_acquirer_;

                rassert(prec->pages_write_acquired_last_.has_element(this));
                prec->pages_write_acquired_last_.remove(this);

                acq_txn->connect_preceder(prec);
            }

            acq_txn->pages_write_acquired_last_.add(this);
            last_write_acquirer_ = acq_txn;
        }
    } else {
        rassert(acq->the_txn_ == nullptr);
        acq->block_version_ = prev_version;
    }

    acquirers_.push_back(acq);
    pulse_pulsables(acq);
}

void current_page_t::remove_acquirer(current_page_acq_t *acq) {
    current_page_acq_t *next = acquirers_.next(acq);
    acquirers_.remove(acq);
    if (next != nullptr) {
        pulse_pulsables(next);
    }
}

void current_page_t::pulse_pulsables(current_page_acq_t *const acq) {
    const current_page_help_t help = acq->help();

    // First, avoid pulsing when there's nothing to pulse.
    {
        current_page_acq_t *prev = acquirers_.prev(acq);
        if (!(prev == nullptr || (prev->access_ == access_t::read
                               && prev->read_cond_.is_pulsed()))) {
            return;
        }
    }

    // Second, avoid re-pulsing already-pulsed chains.
    if (acq->access_ == access_t::read && acq->read_cond_.is_pulsed()
        && !acq->declared_snapshotted_) {
        // acq was pulsed for read, but it could have been a write acq at that time,
        // so the next node might not have been pulsed for read.  Also we might as
        // well stop if we're at the end of the chain (and have been pulsed).
        current_page_acq_t *next = acquirers_.next(acq);
        if (next == nullptr || next->read_cond_.is_pulsed()) {
            return;
        }
    }

    const repli_timestamp_t current_recency = help.page_cache->recency_for_block_id(help.block_id);

    // It's time to pulse the pulsables.
    current_page_acq_t *cur = acq;
    while (cur != nullptr) {
        // We know that the previous node has read access and has been pulsed as
        // readable, so we pulse the current node as readable.
        cur->pulse_read_available();

        if (cur->access_ == access_t::read) {
            current_page_acq_t *next = acquirers_.next(cur);
            if (cur->declared_snapshotted_) {
                // Snapshotters get kicked out of the queue, to make way for
                // write-acquirers.

                // We treat deleted pages this way because a write-acquirer may
                // downgrade itself to readonly and snapshotted for the sake of
                // flushing its version of the page -- and if it deleted the page,
                // this is how it learns.

                cur->snapshotted_page_.init(
                        current_recency,
                        the_page_for_read_or_deleted(help));
                acquirers_.remove(cur);
            }
            cur = next;
        } else {
            // Even the first write-acquirer gets read access (there's no need for an
            // "intent" mode).  But subsequent acquirers need to wait, because the
            // write-acquirer might modify the value.
            if (acquirers_.prev(cur) == nullptr) {
                // (It gets exclusive write access if there's no preceding reader.)
                guarantee(!is_deleted_);
                cur->pulse_write_available();
            }
            break;
        }
    }
}

void current_page_t::add_keepalive() {
    ++num_keepalives_;
}

void current_page_t::remove_keepalive() {
    guarantee(num_keepalives_ > 0);
    --num_keepalives_;
}

void current_page_t::mark_deleted(current_page_help_t help) {
    rassert(!is_deleted_);
    is_deleted_ = true;

    // Only the last acquirer (the current write-acquirer) of a block may mark it
    // deleted, because subsequent acquirers should not be trying to create a block
    // whose block id hasn't been released to the free list yet.
    rassert(acquirers_.size() == 1);

    help.page_cache->set_recency_for_block_id(help.block_id,
                                              repli_timestamp_t::invalid);
    page_.reset_page_ptr(help.page_cache);
    // It's the caller's responsibility to call consider_evicting_current_page after
    // we return, if that would make sense (it wouldn't though).
}

void current_page_t::convert_from_serializer_if_necessary(current_page_help_t help,
                                                          cache_account_t *account) {
    rassert(!is_deleted_);
    if (!page_.has()) {
        page_.init(new page_t(help.block_id, help.page_cache, account));
    }
}

void current_page_t::convert_from_serializer_if_necessary(current_page_help_t help) {
    rassert(!is_deleted_);
    if (!page_.has()) {
        page_.init(new page_t(help.block_id, help.page_cache));
    }
}

page_t *current_page_t::the_page_for_read(current_page_help_t help,
                                          cache_account_t *account) {
    guarantee(!is_deleted_);
    convert_from_serializer_if_necessary(help, account);
    return page_.get_page_for_read();
}

page_t *current_page_t::the_page_for_read_or_deleted(current_page_help_t help) {
    if (is_deleted_) {
        return nullptr;
    } else {
        convert_from_serializer_if_necessary(help);
        return page_.get_page_for_read();
    }
}

page_t *current_page_t::the_page_for_write(current_page_help_t help,
                                           cache_account_t *account) {
    guarantee(!is_deleted_);
    convert_from_serializer_if_necessary(help, account);
    return page_.get_page_for_write(help.page_cache, account);
}

page_txn_t::page_txn_t(page_cache_t *_page_cache,
                       throttler_acq_t throttler_acq,
                       cache_conn_t *cache_conn)
    : drainer_lock_(_page_cache->drainer_lock()),
      page_cache_(_page_cache),
      throttler_acq_(std::move(throttler_acq)),
      live_acqs_(0),
      dirty_changes_pages_(0),
      began_waiting_for_flush_(false),
      spawned_flush_(false),
      mark_(marked_not),
      flush_complete_waiters_() {
    if (cache_conn != nullptr) {
        page_txn_t *old_newest_txn = cache_conn->newest_txn_;
        cache_conn->newest_txn_ = this;
        if (old_newest_txn != nullptr) {
            old_newest_txn->cache_conns_.remove(cache_conn);
        }
        cache_conns_.push_front(cache_conn);
        if (old_newest_txn != nullptr) {
            connect_preceder(old_newest_txn);
        }
    }
}

bool page_txn_t::set_pre_spawn_flush() {
    ASSERT_NO_CORO_WAITING;
    if (throttler_acq_.pre_spawn_flush()) {
        return false;
    }
    throttler_acq_.set_pre_spawn_flush(dirtied_page_count());
    if (in_a_list()) {
        rassert(began_waiting_for_flush_);
        page_cache_->waiting_for_spawn_flush_.remove(this);
        page_cache_->want_to_spawn_flush_.push_back(this);
    } else {
        rassert(!began_waiting_for_flush_);
    }
    return true;
}

void page_txn_t::propagate_pre_spawn_flush(page_txn_t *base) {
    ASSERT_NO_CORO_WAITING;
    if (!base->set_pre_spawn_flush()) {
        return;
    }
    // All elements of stack have pre_spawn_flush_ freshly set.  (Thus, we never push a
    // page_txn_t onto this stack more than once.)
    std::vector<page_txn_t *> stack = {base};
    while (!stack.empty()) {
        page_txn_t *txn = stack.back();
        stack.pop_back();
        for (page_txn_t *p : txn->preceders_) {
            if (p->set_pre_spawn_flush()) {
                stack.push_back(p);
            }
        }
    }
}

void page_txn_t::connect_preceder(page_txn_t *preceder) {
    page_cache_->assert_thread();
    rassert(preceder->page_cache_ == page_cache_);
    // We can't add ourselves as a preceder, we have to avoid that.
    rassert(preceder != this);
    // spawned_flush_ is set at the same time that this txn is removed entirely from the
    // txn graph, so we can't be adding preceders after that point.
    rassert(!preceder->spawned_flush_);

    // See "PERFORMANCE(preceders_)".
    if (std::find(preceders_.begin(), preceders_.end(), preceder)
        == preceders_.end()) {
        preceders_.push_back(preceder);
        preceder->subseqers_.push_back(this);
        if (throttler_acq_.pre_spawn_flush()) {
            page_txn_t::propagate_pre_spawn_flush(preceder);
        }
    }
}

void page_txn_t::remove_preceder(page_txn_t *preceder) {
    // See "PERFORMANCE(preceders_)".
    auto it = std::find(preceders_.begin(), preceders_.end(), preceder);
    rassert(it != preceders_.end());
    preceders_.erase(it);
}

void page_txn_t::remove_subseqer(page_txn_t *subseqer) {
    // See "PERFORMANCE(subseqers_)".
    auto it = std::find(subseqers_.begin(), subseqers_.end(), subseqer);
    rassert(it != subseqers_.end());
    subseqers_.erase(it);
}

page_txn_t::~page_txn_t() {
    guarantee(flush_complete_waiters_.empty());

    guarantee(preceders_.empty());
    guarantee(subseqers_.empty());

    guarantee(changes_.empty());
}

void page_txn_t::add_acquirer(DEBUG_VAR current_page_acq_t *acq) {
    rassert(acq->access_ == access_t::write);
    ++live_acqs_;
}

void page_txn_t::remove_acquirer(current_page_acq_t *acq) {
    guarantee(acq->access_ == access_t::write);
    // This is called by acq's destructor.
    {
        rassert(live_acqs_ > 0);
        --live_acqs_;
    }

    // It's not snapshotted because you can't snapshot write acqs.  (We
    // rely on this fact solely because we need to grab the block_id_t
    // and current_page_acq_t currently doesn't know it.)
    rassert(acq->current_page_ != nullptr);

    const block_version_t block_version = acq->block_version();

    if (acq->dirtied_page()) {
        // We know we hold an exclusive lock.
        rassert(acq->write_cond_.is_pulsed());

    } else if (acq->touched_page()) {
        add_touched_page(acq->block_id(), block_version, acq->recency());
    }
}

template <class T>
void remove_unique(std::vector<T> *vec, T removee) {
    auto it = std::find(vec->begin(), vec->end(), removee);
    rassert(it != vec->end());
    *it = std::move(vec->back());
    vec->pop_back();
}

// This removes replacee from *vec.  It adds replacement to *vec, unless replacement is
// already present.  Returns true if replacement was not already present.  This can
// reorder *vec however it wants to.
template <class T>
bool merge_replace(std::vector<T> *vec, T replacee, T replacement) {
    // HSI: We could do this in one traversal.
    auto it = std::find(vec->begin(), vec->end(), replacee);
    rassert(it != vec->end());
    auto jt = std::find(vec->begin(), vec->end(), replacement);
    if (jt == vec->end()) {
        *it = replacement;
        return true;
    } else {
        *it = std::move(vec->back());
        vec->pop_back();
        return false;
    }
}

void update_backindex_back_pointer(current_page_t *cp, page_txn_t *txn) {
    cp->last_write_acquirer_ = txn;
}

void update_backindex_back_pointer(current_page_dirtier_t cp, page_txn_t *txn) {
    cp.current_page->last_dirtier_ = txn;
}

template <class T, size_t N>
void move_elements(page_txn_t *ptr,
                   backindex_bag_t<T, N> *dest, backindex_bag_t<T, N> *src) {
    rassert(dest != src);
    while (!src->empty()) {
        T elem = src->access_random(0);
        src->remove(elem);
        dest->add(elem);
        update_backindex_back_pointer(elem, ptr);
    }
}

void page_txn_t::merge(scoped_ptr_t<page_txn_t> &&scoped_other) {
    page_txn_t *other = scoped_other.get();

    // Skip drainer lock -- nothing to merge there.

    // Merge page cache?  No need.
    rassert(page_cache_ == other->page_cache_);

    // Merge cache conns.
    while (cache_conn_t *conn = other->cache_conns_.head()) {
        other->cache_conns_.pop_front();
        rassert(conn->newest_txn_ == other);
        conn->newest_txn_ = this;
        cache_conns_.push_front(conn);
    }

    // Merge throttler acqs.
    throttler_acq_.merge(std::move(other->throttler_acq_));

    // HSI: Add back/forward indices to preceders_/subseqers_ instead.
    for (page_txn_t *o_prec : other->preceders_) {
        if (o_prec == this) {
            remove_unique(&subseqers_, other);
        } else {
            if (merge_replace(&o_prec->subseqers_, other, this)) {
                preceders_.push_back(o_prec);
            }
        }
    }
    other->preceders_.clear();

    for (page_txn_t *o_subseq : other->subseqers_) {
        if (o_subseq == this) {
            remove_unique(&preceders_, other);
        } else {
            if (merge_replace(&o_subseq->preceders_, other, this)) {
                subseqers_.push_back(o_subseq);
            }
        }
    }
    other->subseqers_.clear();

    move_elements(this, &pages_write_acquired_last_,
                  &other->pages_write_acquired_last_);
    move_elements(this, &pages_dirtied_last_, &other->pages_dirtied_last_);

    rassert(live_acqs_ == 0 && other->live_acqs_ == 0);
    live_acqs_ += other->live_acqs_;
    other->live_acqs_ = 0;

    // net_dirty is 0 or -1.
    int net_dirty = page_cache_t::merge_changes(
        page_cache_, &changes_, std::move(other->changes_));

    dirty_changes_pages_ += other->dirty_changes_pages_ + net_dirty;
    other->dirty_changes_pages_ = 0;

    // No need to worry about propagating began_waiting_for_flush_, spawned_flush_, or
    // mark_.
    rassert(began_waiting_for_flush_ && other->began_waiting_for_flush_);
    rassert(!spawned_flush_ && !other->spawned_flush_);
    rassert(mark_ == marked_not && other->mark_ == marked_not);

    // Now the waiters wait on us...
    flush_complete_waiters_.append_and_clear(&other->flush_complete_waiters_);

    scoped_other.reset();

    throttler_acq_.update_dirty_page_count(dirtied_page_count());
}

block_change_t make_block_change(block_version_t version,
                                 repli_timestamp_t tstamp,
                                 page_ptr_t &&ptr) {
    return block_change_t(version, true, std::move(ptr), tstamp);
}

void page_txn_t::add_snapshotted_dirtied_page(
        block_id_t block_id, block_version_t version,
        repli_timestamp_t tstamp, page_ptr_t &&ptr) {
    rassert(!ptr.has() || tstamp != repli_timestamp_t::invalid
        || is_aux_block_id(block_id));

    auto res = changes_.emplace(block_id, block_change_t());
    auto const jt = res.first;
    if (res.second) {
        dirty_changes_pages_ += ptr.has();
        jt->second = make_block_change(version, tstamp, std::move(ptr));
    } else {
        dirty_changes_pages_ += jt->second.merge(page_cache_,
            make_block_change(version, tstamp, std::move(ptr)));
    }
}

void page_txn_t::add_touched_page(
        block_id_t block_id, block_version_t version, repli_timestamp_t tstamp) {
    auto res = changes_.emplace(block_id, block_change_t(version, tstamp));
    if (!res.second) {
        auto jt = res.first;
        rassert(jt->second.version != version);
        if (jt->second.version < version) {
            jt->second.version = version;
            jt->second.tstamp = tstamp;
        }
    }
}

int block_change_t::merge(page_cache_t *page_cache, block_change_t &&other) {
    rassert(version != other.version);
    int old_pages_count = page.has() + other.page.has();
    // This function is commutative -- changes from the later version supercede those of
    // the earlier version.  (Both branches here do the same thing.)
    if (version < other.version) {
        if (other.modified) {
            modified = true;
            page.reset_page_ptr(page_cache);
            page = std::move(other.page);
        }
        tstamp = other.tstamp;
    } else {
        // Move in other's page unless we supercede.
        if (!modified) {
            modified = other.modified;
            page = std::move(other.page);
        } else {
            other.page.reset_page_ptr(page_cache);
        }
    }
    return page.has() - old_pages_count;
}

int64_t page_cache_t::merge_changes(
        page_cache_t *page_cache,
        std::unordered_map<block_id_t, block_change_t> *onto,
        std::unordered_map<block_id_t, block_change_t> &&from) {
    int64_t total_net_dirty = 0;
    for (auto &p : from) {
        auto res = onto->emplace(p.first, block_change_t());
        auto const jt = res.first;
        if (res.second) {
            jt->second = std::move(p.second);
        } else {
            total_net_dirty += jt->second.merge(page_cache, std::move(p.second));
        }
    }
    from.clear();
    return total_net_dirty;
}

page_cache_t::collapsed_txns_t
page_cache_t::compute_changes(page_cache_t *page_cache,
                              std::vector<scoped_ptr_t<page_txn_t>> &&txns) {
    ASSERT_NO_CORO_WAITING;
    // We combine changes, using the block_version_t value to see which change
    // happened later.  This even works if a single transaction acquired the same
    // block twice.

    rassert(!txns.empty());

    page_txn_t *first = txns.front().get();

    collapsed_txns_t ret {
        std::move(first->drainer_lock_),
        std::move(first->throttler_acq_),
        std::move(first->changes_),
        std::move(first->flush_complete_waiters_)
    };

    // We merge out the throttler_acq's of the txn's.
    int64_t dirty_changes_pages = first->dirty_changes_pages_;

    for (auto it = txns.begin() + 1; it != txns.end(); ++it) {
        page_txn_t *txn = it->get();
        ret.acq.merge(std::move(txn->throttler_acq_));

        int64_t net_dirty = page_cache_t::merge_changes(
            page_cache, &ret.changes, std::move(txn->changes_));
        dirty_changes_pages += net_dirty;
        ret.acq.update_dirty_page_count(dirty_changes_pages);
        ret.flush_complete_waiters.append_and_clear(&txn->flush_complete_waiters_);
    }

    txns.clear();

    return ret;
}

void page_cache_t::remove_txn_set_from_graph(
        page_cache_t *page_cache,
        const std::vector<scoped_ptr_t<page_txn_t>> &txns) {
    // We want detaching the subseqers and preceders to happen at the same time
    // spawned_flush_ is set.  That way connect_preceder can use it to check it's not
    // called on an already disconnected part of the graph.
    ASSERT_FINITE_CORO_WAITING;
    page_cache->assert_thread();

    for (auto it = txns.begin(); it != txns.end(); ++it) {
        page_txn_t *txn = it->get();
        {
            for (auto jt = txn->subseqers_.begin(); jt != txn->subseqers_.end(); ++jt) {
                (*jt)->remove_preceder(txn);
            }
            txn->subseqers_.clear();
        }

        // We could have preceders outside this txn set, because transactions that
        // don't make any modifications don't get flushed, and they don't wait for
        // their preceding transactions to get flushed and then removed from the
        // graph.
        for (auto jt = txn->preceders_.begin(); jt != txn->preceders_.end(); ++jt) {
            (*jt)->remove_subseqer(txn);
        }
        txn->preceders_.clear();

        while (txn->pages_write_acquired_last_.size() != 0) {
            current_page_t *current_page
                = txn->pages_write_acquired_last_.access_random(0);
            rassert(current_page->last_write_acquirer_ == txn);

#ifndef NDEBUG
            // All existing acquirers should be read acquirers, since this txn _was_ the
            // last write acquirer.  (Preceding write acquirers must have unacquired the
            // page.)
            for (current_page_acq_t *acq = current_page->acquirers_.head();
                 acq != nullptr;
                 acq = current_page->acquirers_.next(acq)) {
                rassert(acq->access() == access_t::read);
            }
#endif

            txn->pages_write_acquired_last_.remove(current_page);
            current_page->last_write_acquirer_ = nullptr;
            page_cache->consider_evicting_current_page(current_page->block_id_);
        }

        while (txn->pages_dirtied_last_.size() != 0) {
            current_page_dirtier_t dirtier
                = txn->pages_dirtied_last_.access_random(0);

            page_cache->help_take_snapshotted_dirtied_page(
                dirtier.current_page, dirtier.current_page->block_id_, txn);

            txn->pages_dirtied_last_.remove(dirtier);
            dirtier.current_page->last_dirtier_ = nullptr;

            page_cache->consider_evicting_current_page(dirtier.current_page->block_id_);
        }

        while (!txn->cache_conns_.empty()) {
            cache_conn_t *conn = txn->cache_conns_.head();
            rassert(conn->newest_txn_ == txn);
            txn->cache_conns_.remove(conn);
            conn->newest_txn_ = nullptr;
        }

        // We only do this propagation to keep down the number of possible states a
        // txn's variables can be in.  This way spawned_flush_ is true only if both
        // began_waiting_for_flush_ and pre_spawn_flush_ are true.  It doesn't actually
        // propagate anywhere, either, because we just disconnected the txn from the
        // graph.
        if (!txn->throttler_acq_.pre_spawn_flush()) {
            page_txn_t::propagate_pre_spawn_flush(txn);
        }
        rassert(!txn->spawned_flush_);
        txn->spawned_flush_ = true;
        page_cache->want_to_spawn_flush_.remove(txn);
    }
}

struct block_token_tstamp_t {
    block_token_tstamp_t(block_id_t _block_id,
                         bool _is_deleted,
                         counted_t<block_token_t> _block_token,
                         repli_timestamp_t _tstamp,
                         page_t *_page)
        : block_id(_block_id), is_deleted(_is_deleted),
          block_token(std::move(_block_token)), tstamp(_tstamp),
          page(_page) { }
    block_id_t block_id;
    bool is_deleted;
    counted_t<block_token_t> block_token;
    repli_timestamp_t tstamp;
    // The page, or nullptr, if we don't know it.
    page_t *page;
};

struct ancillary_info_t {
    explicit ancillary_info_t(repli_timestamp_t _tstamp)
        : tstamp(_tstamp) { }
    repli_timestamp_t tstamp;
    page_acq_t page_acq;
};

struct flush_prep_t {
    std::vector<block_token_tstamp_t> blocks_by_tokens;

    // ancillary_infos holds a page_acq_t for any page we need to write, to prevent its
    // buf from getting freed out from under us (by a force-eviction operation, or
    // anything else).
    std::vector<ancillary_info_t> ancillary_infos;
    std::vector<buf_write_info_t> write_infos;

    void reserve(size_t size) {
        blocks_by_tokens.reserve(size);
        ancillary_infos.reserve(size);
        write_infos.reserve(size);
    }
};

flush_prep_t page_cache_t::prep_flush_changes(
        page_cache_t *page_cache,
        const std::unordered_map<block_id_t, block_change_t> &changes) {
    ASSERT_NO_CORO_WAITING;

    flush_prep_t prep;
    prep.reserve(changes.size());

    for (auto it = changes.begin(); it != changes.end(); ++it) {
        if (it->second.modified) {
            if (!it->second.page.has()) {
                // The block is deleted.
                prep.blocks_by_tokens.emplace_back(
                    it->first,
                    true,
                    counted_t<block_token_t>(),
                    repli_timestamp_t::invalid,
                    nullptr);
            } else {
                page_t *page = it->second.page.get_page_for_read();
                if (page->block_token().has()) {
                    // It's already on disk, we're not going to flush it.
                    prep.blocks_by_tokens.emplace_back(
                        it->first,
                        false,
                        page->block_token(),
                        it->second.tstamp,
                        page);
                } else {
                    // We can't be in the process of loading a block we're going
                    // to write for which we don't have a block token.  That's
                    // because we _actually dirtied the page_.  We had to have
                    // acquired the buf, and the only way to get rid of the buf
                    // is for it to be evicted, in which case the block token
                    // would be non-empty.

                    rassert(page->is_loaded());

                    prep.write_infos.emplace_back(
                        page->get_loaded_ser_buffer(),
                        page->get_page_buf_size(),
                        it->first);
                    prep.ancillary_infos.emplace_back(it->second.tstamp);
                    // The account doesn't matter because the page is already
                    // loaded.
                    prep.ancillary_infos.back().page_acq.init(
                        page, page_cache, page_cache->default_reads_account());
                }
            }
        } else {
            // We only touched the page.
            prep.blocks_by_tokens.emplace_back(
                it->first,
                false,
                counted_t<block_token_t>(),
                it->second.tstamp,
                nullptr);
        }
    }

    return prep;
}

template <class T>
void vec_move_append(std::vector<T> *onto, std::vector<T> &&from) {
    if (onto->empty()) {
        *onto = std::move(from);
    } else {
        std::move(from.begin(), from.end(), std::back_inserter(*onto));
    }
}

size_t decent_sized_write(const std::vector<buf_write_info_t> &write_infos, size_t pos) {
    const size_t bound = 4 * MEGABYTE;
    const size_t n = write_infos.size();
    size_t acc = 0;
    while (pos < n) {
        size_t bs = write_infos[pos].block_size.ser_value();
        if (bound - acc < bs) {
            return pos;
        } else {
            acc += bs;
            ++pos;
        }
    }
    return pos;
}

struct iocallback_cond_t : public iocallback_t, public cond_t {
    void on_io_complete() {
        pulse();
    }
};

std::vector<counted_t<block_token_t>> page_cache_t::do_write_blocks(
        page_cache_t *page_cache,
        const std::vector<buf_write_info_t> &write_infos,
        state_timestamp_t our_write_number,
        ticks_t soft_deadline) {

    size_t pos = 0;

    std::vector<counted_t<block_token_t> > tokens;

    while (pos < write_infos.size() &&
           our_write_number > page_cache->ser_thread_max_asap_write_token_timestamp_) {

        ticks_t before = get_ticks();

        size_t end_pos = decent_sized_write(write_infos, pos);

        iocallback_cond_t blocks_written_cb;
        std::vector<counted_t<block_token_t> > tmp
            = page_cache->serializer_->block_writes(write_infos.data() + pos,
                                                    end_pos - pos,
                                                    /* disk account is overridden
                                                     * by merger_serializer_t */
                                                    DEFAULT_DISK_ACCOUNT,
                                                    &blocks_written_cb);
        vec_move_append(&tokens, std::move(tmp));
        tokens.reserve(write_infos.size());
        blocks_written_cb.wait();

        ticks_t after = get_ticks();
        ticks_t duration{after.nanos - before.nanos};

        if (after.nanos < soft_deadline.nanos) {
            /* Our naptime algo is kind of hacky.
            (Assuming equal block sizes, because whatever.)
            - Proportion written is (end_pos - pos) / (size - pos).
            - We want this to equal (after + naptime - before) / (soft_deadline -
            before).
            So:

            (end_pos - pos) / (size - pos) = (after + naptime - before) /
                (soft_deadline - before)

            (soft_deadline - before) * (end_pos - pos) / (size - pos)
                 = (after + naptime - before)

            (soft_deadline - before) * (end_pos - pos) / (size - pos) - (after - before)
                 = naptime

            The units match.
            */

            ticks_t wakeup_time{int64_t((soft_deadline.nanos - before.nanos) * (end_pos - pos) / (write_infos.size() - pos))};

            if (wakeup_time.nanos > duration.nanos) {
                int64_t naptime = wakeup_time.nanos - duration.nanos;
                // But don't nap more than 7/8 of the time.  We'll finish our smear early.
                naptime = std::min<int64_t>(naptime, duration.nanos * 7);

                // This smearing logic is so bad.  Much nicer would be if we could
                // update the priority of writes in-flight.  I don't know if anything is
                // stopping that, it would take some engineering time.

                nap(naptime / MILLION);
            } else {
                // We're not flushing fast enough to keep up with our smear interval.
                // That's okay.
            }


        }

        pos = end_pos;
    }

    if (pos < write_infos.size()) {
        iocallback_cond_t blocks_written_cb;
        std::vector<counted_t<block_token_t> > tmp
            = page_cache->serializer_->block_writes(write_infos.data() + pos,
                                                    write_infos.size() - pos,
                                                    /* disk account is overridden
                                                     * by merger_serializer_t */
                                                    DEFAULT_DISK_ACCOUNT,
                                                    &blocks_written_cb);

        vec_move_append(&tokens, std::move(tmp));
        blocks_written_cb.wait();
    }

    // Note: There is some reason related to fixing issue 4545 (see efec93e092c1)
    // why we don't just update pages' block tokens during writing, or after, and
    // instead wait for index writes to be reflected below.
    return tokens;
}

void page_cache_t::do_flush_changes(
        page_cache_t *page_cache,
        collapsed_txns_t *coltx,
        fifo_enforcer_write_token_t index_write_token,
        bool asap,
        ticks_t soft_deadline) {
    std::unordered_map<block_id_t, block_change_t> &changes = coltx->changes;
    rassert(!changes.empty());
    flush_prep_t prep = page_cache_t::prep_flush_changes(page_cache, changes);

    cond_t blocks_released_cond;
    {
        on_thread_t th(page_cache->serializer_->home_thread());

        if (asap) {
            page_cache->ser_thread_max_asap_write_token_timestamp_
                = index_write_token.timestamp;
        }

        std::vector<counted_t<block_token_t>> tokens
            = page_cache_t::do_write_blocks(page_cache, prep.write_infos,
                                            index_write_token.timestamp,
                                            soft_deadline);

        rassert(tokens.size() == prep.write_infos.size());
        rassert(prep.write_infos.size() == prep.ancillary_infos.size());
        for (size_t i = 0; i < prep.write_infos.size(); ++i) {
            prep.blocks_by_tokens.emplace_back(
                prep.write_infos[i].block_id,
                false,
                std::move(tokens[i]),
                prep.ancillary_infos[i].tstamp,
                prep.ancillary_infos[i].page_acq.page());
        }

        // KSI: Unnecessary copying between blocks_by_tokens and write_ops, inelegant
        // representation of deletion/touched blocks in blocks_by_tokens.
        std::vector<index_write_op_t> write_ops;
        write_ops.reserve(prep.blocks_by_tokens.size());

        for (auto it = prep.blocks_by_tokens.begin(); it != prep.blocks_by_tokens.end();
             ++it) {
            if (it->is_deleted) {
                write_ops.emplace_back(
                    it->block_id,
                    make_optional(counted_t<block_token_t>()),
                    make_optional(repli_timestamp_t::invalid));
            } else if (it->block_token.has()) {
                write_ops.emplace_back(it->block_id,
                                       make_optional(it->block_token),
                                       make_optional(it->tstamp));
            } else {
                write_ops.emplace_back(it->block_id,
                                       r_nullopt,
                                       make_optional(it->tstamp));
            }
        }

        fifo_enforcer_sink_t::exit_write_t exiter(&page_cache->index_write_sink_->sink,
                                                  index_write_token);
        exiter.wait();
        new_mutex_in_line_t mutex_acq(&page_cache->index_write_sink_->mutex);
        exiter.end();

        rassert(!write_ops.empty());
        mutex_acq.acq_signal()->wait();
        page_cache->serializer_->index_write(
            &mutex_acq,
            [&]() {
                // Update the block tokens and free the associated snapshots once the
                // serializer's in-memory index has been updated (we don't need to wait
                // until the index changes have been written to disk).
                coro_t::spawn_on_thread([&]() {
                    // Update the block tokens of the written blocks
                    for (auto &block : prep.blocks_by_tokens) {
                        if (block.block_token.has() && block.page != nullptr) {
                            // We know page is still a valid pointer because of the
                            // page_ptr_t in changes.

                            // HSI: This assertion would fail if we try to force-evict
                            // the page simultaneously as this write.
                            rassert(!block.page->block_token().has());
                            eviction_bag_t *old_bag
                                = page_cache->evicter().correct_eviction_category(
                                    block.page);
                            block.page->init_block_token(
                                std::move(block.block_token),
                                page_cache);
                            page_cache->evicter().change_to_correct_eviction_bag(
                                old_bag,
                                block.page);
                        }
                    }

                    // Clear the page acqs before we reset their associated page ptr's
                    // below.
                    prep.ancillary_infos.clear();

                    // Really this is what clears the changes -- the txn->changes_
                    // should already be reset.
                    for (auto &change_pair : changes) {
                        change_pair.second.page.reset_page_ptr(page_cache);
                        page_cache->consider_evicting_current_page(change_pair.first);
                    }
                    changes.clear();
                    coltx->acq.mark_dirty_pages_written();

                    blocks_released_cond.pulse();
                }, page_cache->home_thread());
            }, write_ops);
    }

    // Wait until the block release coroutine has finished to we can safely
    // continue (this is important because once we return, a page transaction
    // or even the whole page cache might get destructed).
    blocks_released_cond.wait();
}

void page_cache_t::pulse_flush_complete(collapsed_txns_t &&coltx) {
    for (page_txn_complete_cb_t *p = coltx.flush_complete_waiters.head();
         p != nullptr; ) {
        page_txn_complete_cb_t *tmp = p;
            p = coltx.flush_complete_waiters.next(p);
        coltx.flush_complete_waiters.remove(tmp);
        tmp->cond.pulse();
    }
}

void page_cache_t::do_flush_txn_set(
        page_cache_t *page_cache,
        collapsed_txns_t *coltx_ptr,
        bool asap,
        ticks_t soft_deadline) {
    // This is called with spawn_now_dangerously!  The reason is partly so that we
    // don't put a zillion coroutines on the message loop when doing a bunch of
    // reads.  The other reason is that passing changes through a std::bind without
    // copying it would be very annoying.
    page_cache->assert_thread();

    // We're going to flush these transactions.  First we need to figure out what the
    // set of changes we're actually doing is, since any transaction may have touched
    // the same blocks.

    collapsed_txns_t coltx = std::move(*coltx_ptr);

    rassert(!coltx.changes.empty());

    fifo_enforcer_write_token_t index_write_token
        = page_cache->index_write_source_.enter_write();

    page_cache->num_active_asap_false_flushes_ += (asap ? 0 : 1);

    // Okay, yield, thank you.
    coro_t::yield();

    do_flush_changes(page_cache, &coltx, index_write_token, asap, soft_deadline);

    page_cache->num_active_asap_false_flushes_ -= (asap ? 0 : 1);

    // Flush complete.
    page_cache_t::pulse_flush_complete(std::move(coltx));
}

std::vector<scoped_ptr_t<page_txn_t>>
page_cache_t::maximal_flushable_txn_set(page_txn_t *base) {
    // Returns all transactions that can presently be flushed, given the newest
    // transaction that has had began_waiting_for_flush_ set.  (We assume all
    // previous such sets of transactions had flushing begin on them.)
    //
    // page_txn_t's `mark` fields can be in the following states:
    //  - not: the page has not yet been considered for processing
    //  - blue: the page is going to be considered for processing
    //  - green: the page _has_ been considered for processing, nothing bad so far
    //  - red: the page _has_ been considered for processing, and it is unflushable.
    //
    // By the end of the function (before we construct the return value), no
    // page_txn_t's are blue, and all subseqers of red pages are either red or not
    // marked.  All flushable page_txn_t's are green.
    //
    // Here are all possible transitions of the mark.  The states blue(1) and blue(2)
    // both have a blue mark, but the latter is known to have a red parent.
    //
    // not -> blue(1)
    // blue(1) -> red
    // blue(1) -> green
    // green -> blue(2)
    // blue(2) -> red
    //
    // From this transition table you can see that every page_txn_t is processed at
    // most twice.


    ASSERT_NO_CORO_WAITING;
    // An element is marked blue iff it's in `blue`.
    std::vector<page_txn_t *> blue;
    // All elements marked red, green, or blue are in `colored` -- we unmark them and
    // construct the return vector at the end of the function.  Those which aren't
    // included in the return value are only "temporarily" owned by the scoped_ptr_t --
    // they'll get .release()d.
    std::vector<scoped_ptr_t<page_txn_t>> colored;

    rassert(!base->spawned_flush_);
    rassert(base->began_waiting_for_flush_);
    rassert(base->mark_ == page_txn_t::marked_not);
    base->mark_ = page_txn_t::marked_blue;
    blue.push_back(base);
    colored.emplace_back(base);

    while (!blue.empty()) {
        page_txn_t *txn = blue.back();
        blue.pop_back();

        rassert(!txn->spawned_flush_);
        rassert(txn->began_waiting_for_flush_);
        rassert(txn->mark_ == page_txn_t::marked_blue);

        bool poisoned = false;
        for (auto it = txn->preceders_.begin(); it != txn->preceders_.end(); ++it) {
            page_txn_t *prec = *it;
            rassert(!prec->spawned_flush_);
            if (!prec->began_waiting_for_flush_
                || prec->mark_ == page_txn_t::marked_red) {
                poisoned = true;
            } else if (prec->mark_ == page_txn_t::marked_not) {
                prec->mark_ = page_txn_t::marked_blue;
                blue.push_back(prec);
                colored.emplace_back(prec);
            } else {
                rassert(prec->mark_ == page_txn_t::marked_green
                        || prec->mark_ == page_txn_t::marked_blue);
            }
        }

        txn->mark_ = poisoned ? page_txn_t::marked_red : page_txn_t::marked_green;

        for (auto it = txn->subseqers_.begin(); it != txn->subseqers_.end(); ++it) {
            page_txn_t *subs = *it;
            rassert(!subs->spawned_flush_);
            if (!subs->began_waiting_for_flush_) {
                rassert(subs->mark_ == page_txn_t::marked_not);
            } else if (subs->mark_ == page_txn_t::marked_not) {
                if (!poisoned) {
                    subs->mark_ = page_txn_t::marked_blue;
                    blue.push_back(subs);
                    colored.emplace_back(subs);
                }
            } else if (subs->mark_ == page_txn_t::marked_green) {
                if (poisoned) {
                    subs->mark_ = page_txn_t::marked_blue;
                    blue.push_back(subs);
                }
            } else {
                rassert(subs->mark_ == page_txn_t::marked_red
                        || subs->mark_ == page_txn_t::marked_blue);
            }
        }
    }

    auto it = colored.begin();
    auto jt = it;

    while (jt != colored.end()) {
        page_txn_t::mark_state_t mark = (*jt)->mark_;
        (*jt)->mark_ = page_txn_t::marked_not;
        if (mark == page_txn_t::marked_green) {
            *it++ = std::move(*jt++);
        } else {
            rassert(mark == page_txn_t::marked_red);
            // Release page_txn_t that we don't really own yet.
            UNUSED page_txn_t *ignore = jt->release();
            ++jt;
        }
    }

    colored.erase(it, colored.end());
    return colored;
}

void page_cache_t::spawn_flush_flushables(
        std::vector<scoped_ptr_t<page_txn_t>> &&flush_set,
        bool asap,
        ticks_t soft_deadline) {
    rassert(!flush_set.empty());
    // The flush set's txn's are already disconnected from the graph.

    collapsed_txns_t coltx
        = page_cache_t::compute_changes(this, std::move(flush_set));

    if (!coltx.changes.empty()) {
        coro_t::spawn_now_dangerously(std::bind(&page_cache_t::do_flush_txn_set,
                                                this,
                                                &coltx,
                                                asap,
                                                soft_deadline));
    } else {
        // Flush complete.  do_flush_txn_set does this in the write case.
        page_cache_t::pulse_flush_complete(std::move(coltx));
    }
}

void page_cache_t::merge_into_waiting_for_spawn_flush(scoped_ptr_t<page_txn_t> &&base) {
    if (waiting_for_spawn_flush_.empty()) {
        waiting_for_spawn_flush_.push_back(base.release());
        return;
    }

    page_txn_t *last = waiting_for_spawn_flush_.tail();
    last->merge(std::move(base));
}

void page_cache_t::begin_waiting_for_flush(
        scoped_ptr_t<page_txn_t> &&base, write_durability_t durability) {
    assert_thread();
    ASSERT_FINITE_CORO_WAITING;
    rassert(!base->began_waiting_for_flush_);
    rassert(!base->spawned_flush_);

    // This is redundant because we pass the durability in the throttler_acq_
    // constructor anyway.
    if (durability == write_durability_t::HARD) {
        page_txn_t::propagate_pre_spawn_flush(base.get());
    }

    base->began_waiting_for_flush_ = true;
    if (!base->throttler_acq_.pre_spawn_flush()) {
        merge_into_waiting_for_spawn_flush(std::move(base));
    } else {
        page_txn_t *base_unscoped = base.release();
        want_to_spawn_flush_.push_back(base_unscoped);

        std::vector<scoped_ptr_t<page_txn_t>> flush_set
            = page_cache_t::maximal_flushable_txn_set(base_unscoped);

        if (!flush_set.empty()) {
            page_cache_t::remove_txn_set_from_graph(this, flush_set);
            spawn_flush_flushables(std::move(flush_set), true, ticks_t{0} /* no soft deadline */);
        }
    }
}


}  // namespace alt
