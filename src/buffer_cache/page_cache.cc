// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "buffer_cache/page_cache.hpp"

#include <algorithm>
#include <functional>
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
    if (newest_txn_ != NULL) {
        newest_txn_->cache_conn_ = NULL;
        newest_txn_ = NULL;
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
    if (new_count > semaphore_acq_.count()) {
        semaphore_acq_.change_count(new_count);
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
        const counted_t<standard_block_token_t> &token) {
    assert_thread();
    buf_ptr_t local_buf = std::move(*buf);

    block_size_t block_size = block_size_t::undefined();
    scoped_malloc_t<ser_buffer_t> ptr;
    local_buf.release(&block_size, &ptr);

    // We're going to reconstruct the buf_ptr_t on the other side of this do_on_thread
    // call, so we'd better make sure the block size is right.
    guarantee(block_size.value() == token->block_size().value());

    // Notably, this code relies on do_on_thread to preserve callback order (which it
    // does do).
    do_on_thread(page_cache_->home_thread(),
                 std::bind(&page_cache_t::add_read_ahead_buf,
                           page_cache_,
                           block_id,
                           ptr.release(),
                           token));
}

void page_read_ahead_cb_t::destroy_self() {
    serializer_->unregister_read_ahead_cb(this);
    serializer_ = NULL;

    page_cache_t *page_cache = page_cache_;
    page_cache_ = NULL;

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
    if (read_ahead_cb_ != NULL) {
        return;
    }

    current_page_t *current_page = current_pages_.get_sparsely(block_id);
    if (current_page == NULL) {
        return;
    }

    if (current_page->should_be_evicted()) {
        current_pages_[block_id] = NULL;
        current_page->reset(this);
        delete current_page;
    }
}

void page_cache_t::resize_current_pages_to_id(block_id_t block_id) {
    if (current_pages_.size() <= block_id) {
        current_pages_.resize_with_zeros(block_id + 1);
    }
}

void page_cache_t::add_read_ahead_buf(block_id_t block_id,
                                      ser_buffer_t *ser_buffer,
                                      const counted_t<standard_block_token_t> &token) {
    assert_thread();

    scoped_malloc_t<ser_buffer_t> ptr(ser_buffer);

    // We MUST stop if read_ahead_cb_ is NULL because that means current_page_t's
    // could start being destroyed.
    if (read_ahead_cb_ == NULL) {
        return;
    }

    resize_current_pages_to_id(block_id);
    // We MUST stop if current_pages_[block_id] already exists, because that means
    // the read-ahead page might be out of date.
    if (current_pages_[block_id] != NULL) {
        return;
    }

    // We know the read-ahead page is not out of date if current_pages_[block_id] is
    // NULL and if read_ahead_cb_ still exists -- that means a current_page_t for the
    // block id was never created, and thus the page could not have been modified
    // (not to mention that we've already got the page in memory, so there is no
    // useful work to be done).

    buf_ptr_t buf(token->block_size(), std::move(ptr));
    current_pages_[block_id] = new current_page_t(block_id, std::move(buf), token, this);
}

void page_cache_t::have_read_ahead_cb_destroyed() {
    assert_thread();

    if (read_ahead_cb_ != NULL) {
        // By setting read_ahead_cb_ to NULL, we make sure we only tell the read
        // ahead cb to destroy itself exactly once.
        page_read_ahead_cb_t *cb = read_ahead_cb_;
        read_ahead_cb_ = NULL;

        do_on_thread(cb->home_thread(),
                     std::bind(&page_read_ahead_cb_t::destroy_self, cb));

        coro_t::spawn_sometime(std::bind(&page_cache_t::consider_evicting_all_current_pages, this, drainer_->lock()));
    }
}

void page_cache_t::consider_evicting_all_current_pages(page_cache_t *page_cache,
                                                       auto_drainer_t::lock_t lock) {
    for (block_id_t id = 0; id < page_cache->current_pages_.size(); ++id) {
        page_cache->consider_evicting_current_page(id);
        if (id % 16 == 15) {
            coro_t::yield();
            if (lock.get_drain_signal()->is_pulsed()) {
                return;
            }
        }
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

page_cache_t::page_cache_t(serializer_t *serializer,
                           cache_balancer_t *balancer,
                           alt_txn_throttler_t *throttler)
    : max_block_size_(serializer->max_block_size()),
      serializer_(serializer),
      free_list_(serializer),
      evicter_(),
      read_ahead_cb_(NULL),
      drainer_(make_scoped<auto_drainer_t>()) {

    const bool start_read_ahead = balancer->read_ahead_ok_at_start();
    if (start_read_ahead) {
        read_ahead_cb_existence_ = drainer_->lock();
    }

    page_read_ahead_cb_t *local_read_ahead_cb = NULL;
    {
        on_thread_t thread_switcher(serializer->home_thread());
        if (start_read_ahead) {
            local_read_ahead_cb = new page_read_ahead_cb_t(serializer, this);
        }
        default_reads_account_.init(serializer->home_thread(),
                                    serializer->make_io_account(CACHE_READS_IO_PRIORITY));
        index_write_sink_.init(new page_cache_index_write_sink_t);
        recencies_ = serializer->get_all_recencies();
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

    drainer_.reset();
    for (size_t i = 0, e = current_pages_.size(); i < e; ++i) {
        if (i % 256 == 255) {
            coro_t::yield();
        }
        current_page_t *current_page = current_pages_[i];
        if (current_page != NULL) {
            current_page->reset(this);
            delete current_page;
        }
    }

    {
        /* IO accounts must be destroyed on the thread they were created on */
        on_thread_t thread_switcher(serializer_->home_thread());
        // Resetting default_reads_account_ is opportunistically done here, instead
        // of making its destructor switch back to the serializer thread a second
        // time.
        default_reads_account_.reset();
        index_write_sink_.reset();
    }
}

// We go a bit old-school, with a self-destroying callback.
class flush_and_destroy_txn_waiter_t : public signal_t::subscription_t {
public:
    flush_and_destroy_txn_waiter_t(auto_drainer_t::lock_t &&lock,
                                   page_txn_t *txn,
                                   std::function<void(throttler_acq_t *)> on_flush_complete)
        : lock_(std::move(lock)),
          txn_(txn),
          on_flush_complete_(std::move(on_flush_complete)) { }

private:
    void run() {
        // Tell everybody without delay that the flush is complete.
        on_flush_complete_(&txn_->throttler_acq_);

        // We have to do the rest _later_ because of signal_t::subscription_t not
        // allowing reentrant signal_t::subscription_t::reset() calls, and the like,
        // even though it would be valid.
        // We are using `call_later_on_this_thread` instead of spawning a coroutine
        // to reduce memory overhead.
        class kill_later_t : public linux_thread_message_t {
        public:
            explicit kill_later_t(flush_and_destroy_txn_waiter_t *self) :
                self_(self) { }
            void on_thread_switch() {
                self_->kill_ourselves();
                delete this;
            }
        private:
            flush_and_destroy_txn_waiter_t *self_;
        };
        call_later_on_this_thread(new kill_later_t(this));
    }

    void kill_ourselves() {
        // We can't destroy txn_->flush_complete_cond_ until we've reset our
        // subscription, because computers.
        reset();
        delete txn_;
        delete this;
    }

    auto_drainer_t::lock_t lock_;
    page_txn_t *txn_;
    std::function<void(throttler_acq_t *)> on_flush_complete_;

    DISABLE_COPYING(flush_and_destroy_txn_waiter_t);
};

void page_cache_t::flush_and_destroy_txn(
        scoped_ptr_t<page_txn_t> txn,
        std::function<void(throttler_acq_t *)> on_flush_complete) {
    guarantee(txn->live_acqs_ == 0,
              "A current_page_acq_t lifespan exceeds its page_txn_t's.");
    guarantee(!txn->began_waiting_for_flush_);

    txn->announce_waiting_for_flush();

    page_txn_t *page_txn = txn.release();
    flush_and_destroy_txn_waiter_t *sub
        = new flush_and_destroy_txn_waiter_t(drainer_->lock(), page_txn,
                                             std::move(on_flush_complete));

    sub->reset(&page_txn->flush_complete_cond_);
}


current_page_t *page_cache_t::page_for_block_id(block_id_t block_id) {
    assert_thread();

    resize_current_pages_to_id(block_id);
    if (current_pages_[block_id] == NULL) {
        rassert(recency_for_block_id(block_id) != repli_timestamp_t::invalid,
                "Expected block %" PR_BLOCK_ID " not to be deleted "
                "(should you have used alt_create_t::create?).",
                block_id);
        current_pages_[block_id] = new current_page_t(block_id);
    } else {
        rassert(!current_pages_[block_id]->is_deleted());
    }

    return current_pages_[block_id];
}

current_page_t *page_cache_t::page_for_new_block_id(block_id_t *block_id_out) {
    assert_thread();
    block_id_t block_id = free_list_.acquire_block_id();
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
    rassert(recency_for_block_id(block_id) == repli_timestamp_t::invalid,
            "expected chosen block %" PR_BLOCK_ID "to be deleted", block_id);
    set_recency_for_block_id(block_id, repli_timestamp_t::distant_past);

    buf_ptr_t buf = buf_ptr_t::alloc_uninitialized(max_block_size_);

#if !defined(NDEBUG) || defined(VALGRIND)
    // KSI: This should actually _not_ exist -- we are ignoring legitimate errors
    // where we write uninitialized data to disk.
    memset(buf.cache_data(), 0xCD, max_block_size_.value());
#endif

    resize_current_pages_to_id(block_id);
    guarantee(current_pages_[block_id] == NULL);
    current_pages_[block_id] = new current_page_t(block_id, std::move(buf), this);

    return current_pages_[block_id];
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
    : page_cache_(NULL), the_txn_(NULL) { }

current_page_acq_t::current_page_acq_t(page_txn_t *txn,
                                       block_id_t block_id,
                                       access_t access,
                                       page_create_t create)
    : page_cache_(NULL), the_txn_(NULL) {
    init(txn, block_id, access, create);
}

current_page_acq_t::current_page_acq_t(page_txn_t *txn,
                                       alt_create_t create)
    : page_cache_(NULL), the_txn_(NULL) {
    init(txn, create);
}

current_page_acq_t::current_page_acq_t(page_cache_t *page_cache,
                                       block_id_t block_id,
                                       read_access_t read)
    : page_cache_(NULL), the_txn_(NULL) {
    init(page_cache, block_id, read);
}

void current_page_acq_t::init(page_txn_t *txn,
                              block_id_t block_id,
                              access_t access,
                              page_create_t create) {
    if (access == access_t::read) {
        rassert(create == page_create_t::no);
        init(txn->page_cache(), block_id, read_access_t::read);
    } else {
        txn->page_cache()->assert_thread();
        guarantee(page_cache_ == NULL);
        page_cache_ = txn->page_cache();
        the_txn_ = (access == access_t::write ? txn : NULL);
        access_ = access;
        declared_snapshotted_ = false;
        block_id_ = block_id;
        if (create == page_create_t::yes) {
            current_page_ = page_cache_->page_for_new_chosen_block_id(block_id);
        } else {
            current_page_ = page_cache_->page_for_block_id(block_id);
        }
        dirtied_page_ = false;
        touched_page_ = false;

        the_txn_->add_acquirer(this);
        current_page_->add_acquirer(this);
    }
}

void current_page_acq_t::init(page_txn_t *txn,
                              alt_create_t) {
    txn->page_cache()->assert_thread();
    guarantee(page_cache_ == NULL);
    page_cache_ = txn->page_cache();
    the_txn_ = txn;
    access_ = access_t::write;
    declared_snapshotted_ = false;
    current_page_ = page_cache_->page_for_new_block_id(&block_id_);
    dirtied_page_ = false;
    touched_page_ = false;

    the_txn_->add_acquirer(this);
    current_page_->add_acquirer(this);
}

void current_page_acq_t::init(page_cache_t *page_cache,
                              block_id_t block_id,
                              read_access_t) {
    page_cache->assert_thread();
    guarantee(page_cache_ == NULL);
    page_cache_ = page_cache;
    the_txn_ = NULL;
    access_ = access_t::read;
    declared_snapshotted_ = false;
    block_id_ = block_id;
    current_page_ = page_cache_->page_for_block_id(block_id);
    dirtied_page_ = false;
    touched_page_ = false;

    current_page_->add_acquirer(this);
}

current_page_acq_t::~current_page_acq_t() {
    assert_thread();
    // Checking page_cache_ != NULL makes sure this isn't a default-constructed acq.
    if (page_cache_ != NULL) {
        if (the_txn_ != NULL) {
            guarantee(access_ == access_t::write);
            the_txn_->remove_acquirer(this);
        }
        rassert(current_page_ != NULL);
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
    if (current_page_ != NULL) {
        current_page_->pulse_pulsables(this);
    }
}

void current_page_acq_t::declare_snapshotted() {
    assert_thread();
    rassert(access_ == access_t::read);

    // Allow redeclaration of snapshottedness.
    if (!declared_snapshotted_) {
        declared_snapshotted_ = true;
        rassert(current_page_ != NULL);
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
    rassert(snapshotted_page_.has() || current_page_ != NULL);
    read_cond_.wait();
    if (snapshotted_page_.has()) {
        return snapshotted_page_.get_page_for_read();
    }
    rassert(current_page_ != NULL);
    return current_page_->the_page_for_read(help(), account);
}

repli_timestamp_t current_page_acq_t::recency() {
    assert_thread();
    rassert(snapshotted_page_.has() || current_page_ != NULL);

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
    rassert(current_page_ != NULL);
    return page_cache_->recency_for_block_id(block_id_);
}

page_t *current_page_acq_t::current_page_for_write(cache_account_t *account) {
    assert_thread();
    rassert(access_ == access_t::write);
    rassert(current_page_ != NULL);
    write_cond_.wait();
    rassert(current_page_ != NULL);
    dirtied_page_ = true;
    return current_page_->the_page_for_write(help(), account);
}

void current_page_acq_t::set_recency(repli_timestamp_t recency) {
    assert_thread();
    rassert(access_ == access_t::write);
    rassert(current_page_ != NULL);
    write_cond_.wait();
    rassert(current_page_ != NULL);
    touched_page_ = true;
    page_cache_->set_recency_for_block_id(block_id_, recency);
}

void current_page_acq_t::mark_deleted() {
    assert_thread();
    rassert(access_ == access_t::write);
    rassert(current_page_ != NULL);
    write_cond_.wait();
    rassert(current_page_ != NULL);
    dirtied_page_ = true;
    current_page_->mark_deleted(help());
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

block_version_t current_page_acq_t::block_version() const {
    assert_thread();
    return block_version_;
}


page_cache_t *current_page_acq_t::page_cache() const {
    assert_thread();
    return page_cache_;
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

current_page_t::current_page_t(block_id_t block_id)
    : block_id_(block_id),
      is_deleted_(false),
      last_write_acquirer_(NULL),
      num_keepalives_(0) {
    // Increment the block version so that we can distinguish between unassigned
    // current_page_acq_t::block_version_ values (which are 0) and assigned ones.
    rassert(last_write_acquirer_version_.debug_value() == 0);
    last_write_acquirer_version_ = last_write_acquirer_version_.subsequent();
}

current_page_t::current_page_t(block_id_t block_id,
                               buf_ptr_t buf,
                               page_cache_t *page_cache)
    : block_id_(block_id),
      page_(new page_t(block_id, std::move(buf), page_cache)),
      is_deleted_(false),
      last_write_acquirer_(NULL),
      num_keepalives_(0) {
    // Increment the block version so that we can distinguish between unassigned
    // current_page_acq_t::block_version_ values (which are 0) and assigned ones.
    rassert(last_write_acquirer_version_.debug_value() == 0);
    last_write_acquirer_version_ = last_write_acquirer_version_.subsequent();
}

current_page_t::current_page_t(block_id_t block_id,
                               buf_ptr_t buf,
                               const counted_t<standard_block_token_t> &token,
                               page_cache_t *page_cache)
    : block_id_(block_id),
      page_(new page_t(block_id, std::move(buf), token, page_cache)),
      is_deleted_(false),
      last_write_acquirer_(NULL),
      num_keepalives_(0) {
    // Increment the block version so that we can distinguish between unassigned
    // current_page_acq_t::block_version_ values (which are 0) and assigned ones.
    rassert(last_write_acquirer_version_.debug_value() == 0);
    last_write_acquirer_version_ = last_write_acquirer_version_.subsequent();
}

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

    // KSI: Does last_write_acquirer_ even need to be NULL?  Could we not just inform
    // it of our impending destruction?
    rassert(last_write_acquirer_ == NULL);

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
    if (last_write_acquirer_ != NULL) {
        return false;
    }

    // A reason: The current_page_t is kept alive for another reason.  (Important.)
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
        block_version_t v = prev_version.subsequent();
        acq->block_version_ = v;

        rassert(acq->the_txn_ != NULL);
        page_txn_t *const acq_txn = acq->the_txn_;

        last_write_acquirer_version_ = v;

        if (last_write_acquirer_ != acq_txn) {
            rassert(!acq_txn->pages_write_acquired_last_.has_element(this));

            if (last_write_acquirer_ != NULL) {
                page_txn_t *prec = last_write_acquirer_;

                rassert(prec->pages_write_acquired_last_.has_element(this));
                prec->pages_write_acquired_last_.remove(this);

                acq_txn->connect_preceder(prec);
            }

            acq_txn->pages_write_acquired_last_.add(this);
            last_write_acquirer_ = acq_txn;
        }
    } else {
        rassert(acq->the_txn_ == NULL);
        acq->block_version_ = prev_version;
    }

    acquirers_.push_back(acq);
    pulse_pulsables(acq);
}

void current_page_t::remove_acquirer(current_page_acq_t *acq) {
    current_page_acq_t *next = acquirers_.next(acq);
    acquirers_.remove(acq);
    if (next != NULL) {
        pulse_pulsables(next);
    }
}

void current_page_t::pulse_pulsables(current_page_acq_t *const acq) {
    const current_page_help_t help = acq->help();

    // First, avoid pulsing when there's nothing to pulse.
    {
        current_page_acq_t *prev = acquirers_.prev(acq);
        if (!(prev == NULL || (prev->access_ == access_t::read
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
        if (next == NULL || next->read_cond_.is_pulsed()) {
            return;
        }
    }

    const repli_timestamp_t current_recency = help.page_cache->recency_for_block_id(help.block_id);

    // It's time to pulse the pulsables.
    current_page_acq_t *cur = acq;
    while (cur != NULL) {
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
            if (acquirers_.prev(cur) == NULL) {
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
        return NULL;
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

page_txn_t::page_txn_t(page_cache_t *page_cache,
                       throttler_acq_t throttler_acq,
                       cache_conn_t *cache_conn)
    : page_cache_(page_cache),
      cache_conn_(cache_conn),
      throttler_acq_(std::move(throttler_acq)),
      live_acqs_(0),
      began_waiting_for_flush_(false),
      spawned_flush_(false),
      mark_(marked_not) {
    if (cache_conn != NULL) {
        page_txn_t *old_newest_txn = cache_conn->newest_txn_;
        cache_conn->newest_txn_ = this;
        if (old_newest_txn != NULL) {
            rassert(old_newest_txn->cache_conn_ == cache_conn);
            old_newest_txn->cache_conn_ = NULL;
            connect_preceder(old_newest_txn);
        }
    }
}

void page_txn_t::connect_preceder(page_txn_t *preceder) {
    page_cache_->assert_thread();
    rassert(preceder->page_cache_ == page_cache_);
    // We can't add ourselves as a preceder, we have to avoid that.
    rassert(preceder != this);
    // The flush_complete_cond_ is pulsed at the same time that this txn is removed
    // entirely from the txn graph, so we can't be adding preceders after that point.
    rassert(!preceder->flush_complete_cond_.is_pulsed());

    // See "PERFORMANCE(preceders_)".
    if (std::find(preceders_.begin(), preceders_.end(), preceder)
        == preceders_.end()) {
        preceders_.push_back(preceder);
        preceder->subseqers_.push_back(this);
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
    guarantee(flush_complete_cond_.is_pulsed());

    guarantee(preceders_.empty());
    guarantee(subseqers_.empty());

    guarantee(snapshotted_dirtied_pages_.empty());
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
    rassert(acq->current_page_ != NULL);

    const block_version_t block_version = acq->block_version();

    if (acq->dirtied_page()) {
        // We know we hold an exclusive lock.
        rassert(acq->write_cond_.is_pulsed());

        // Declare readonly (so that we may declare acq snapshotted).
        acq->declare_readonly();
        acq->declare_snapshotted();

        // Steal the snapshotted page_ptr_t.
        timestamped_page_ptr_t local = std::move(acq->snapshotted_page_);
        // It's okay to have two dirtied_page_t's or touched_page_t's for the
        // same block id -- compute_changes handles this.
        snapshotted_dirtied_pages_.push_back(dirtied_page_t(block_version,
                                                            acq->block_id(),
                                                            std::move(local)));
        // If you keep writing and reacquiring the same page, though, the count
        // might be off and you could excessively throttle new operations.

        // LSI: We could reacquire the same block and update the dirty page count
        // with a _correct_ value indicating that we're holding redundant dirty
        // pages for the same block id.
        throttler_acq_.update_dirty_page_count(snapshotted_dirtied_pages_.size());
    } else if (acq->touched_page()) {
        // It's okay to have two dirtied_page_t's or touched_page_t's for the
        // same block id -- compute_changes handles this.
        touched_pages_.push_back(touched_page_t(block_version, acq->block_id(),
                                                acq->recency()));
    }
}

void page_txn_t::announce_waiting_for_flush() {
    rassert(live_acqs_ == 0);
    rassert(!began_waiting_for_flush_);
    rassert(!spawned_flush_);
    began_waiting_for_flush_ = true;
    page_cache_->im_waiting_for_flush(this);
}

std::map<block_id_t, page_cache_t::block_change_t>
page_cache_t::compute_changes(const std::vector<page_txn_t *> &txns) {
    // We combine changes, using the block_version_t value to see which change
    // happened later.  This even works if a single transaction acquired the same
    // block twice.

    // The map of changes we make.
    std::map<block_id_t, block_change_t> changes;

    for (auto it = txns.begin(); it != txns.end(); ++it) {
        page_txn_t *txn = *it;
        for (size_t i = 0, e = txn->snapshotted_dirtied_pages_.size(); i < e; ++i) {
            const dirtied_page_t &d = txn->snapshotted_dirtied_pages_[i];

            block_change_t change(d.block_version, true,
                                  d.ptr.has() ? d.ptr.get_page_for_read() : NULL,
                                  d.ptr.has() ? d.ptr.timestamp() : repli_timestamp_t::invalid);

            auto res = changes.insert(std::make_pair(d.block_id, change));

            if (!res.second) {
                // The insertion failed -- we need to use the newer version.
                auto const jt = res.first;
                // The versions can't be the same for different write operations.
                rassert(jt->second.version != change.version,
                        "equal versions on block %" PRIi64 ": %" PRIu64,
                        d.block_id,
                        change.version.debug_value());
                if (jt->second.version < change.version) {
                    jt->second = change;
                }
            }
        }
    }

    for (auto it = txns.begin(); it != txns.end(); ++it) {
        page_txn_t *txn = *it;
        for (size_t i = 0, e = txn->touched_pages_.size(); i < e; ++i) {
            const touched_page_t &t = txn->touched_pages_[i];

            auto res = changes.insert(std::make_pair(t.block_id,
                                                     block_change_t(t.block_version,
                                                                    false,
                                                                    NULL,
                                                                    t.tstamp)));
            if (!res.second) {
                // The insertion failed.  We need to combine the versions.
                auto const jt = res.first;
                // The versions can't be the same for different write operations.
                rassert(jt->second.version != t.block_version);
                if (jt->second.version < t.block_version) {
                    rassert(t.tstamp ==
                            superceding_recency(jt->second.tstamp, t.tstamp));
                    jt->second.tstamp = t.tstamp;
                    jt->second.version = t.block_version;
                }
            }
        }
    }

    return changes;
}

void page_cache_t::remove_txn_set_from_graph(page_cache_t *page_cache,
                                             const std::vector<page_txn_t *> &txns) {
    page_cache->assert_thread();

    for (auto it = txns.begin(); it != txns.end(); ++it) {
        // We want detaching the subsequers and preceders to happen at the same time
        // that the flush_complete_cond_ is pulsed.  That way connect_preceder can
        // check if flush_complete_cond_ has been pulsed.
        ASSERT_FINITE_CORO_WAITING;
        page_txn_t *txn = *it;
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

        // KSI: Maybe we could remove pages_write_acquired_last_ earlier?  Like when
        // we begin the index write (but that's on the wrong thread) or earlier?
        while (txn->pages_write_acquired_last_.size() != 0) {
            current_page_t *current_page
                = txn->pages_write_acquired_last_.access_random(0);
            rassert(current_page->last_write_acquirer_ == txn);

#ifndef NDEBUG
            // All existing acquirers should be read acquirers, since this txn _was_
            // the last write acquirer.
            for (current_page_acq_t *acq = current_page->acquirers_.head();
                 acq != NULL;
                 acq = current_page->acquirers_.next(acq)) {
                rassert(acq->access() == access_t::read);
            }
#endif

            txn->pages_write_acquired_last_.remove(current_page);
            current_page->last_write_acquirer_ = NULL;
            page_cache->consider_evicting_current_page(current_page->block_id_);
        }

        if (txn->cache_conn_ != NULL) {
            rassert(txn->cache_conn_->newest_txn_ == txn);
            txn->cache_conn_->newest_txn_ = NULL;
            txn->cache_conn_ = NULL;
        }

        txn->flush_complete_cond_.pulse();
    }
}

struct block_token_tstamp_t {
    block_token_tstamp_t(block_id_t _block_id,
                         bool _is_deleted,
                         counted_t<standard_block_token_t> _block_token,
                         repli_timestamp_t _tstamp,
                         page_t *_page)
        : block_id(_block_id), is_deleted(_is_deleted),
          block_token(std::move(_block_token)), tstamp(_tstamp),
          page(_page) { }
    block_id_t block_id;
    bool is_deleted;
    counted_t<standard_block_token_t> block_token;
    repli_timestamp_t tstamp;
    // The page, or NULL, if we don't know it.
    page_t *page;
};

struct ancillary_info_t {
    ancillary_info_t(repli_timestamp_t _tstamp,
                     page_t *_page)
        : tstamp(_tstamp), page(_page) { }
    repli_timestamp_t tstamp;
    page_t *page;
};

void page_cache_t::do_flush_changes(page_cache_t *page_cache,
                                    const std::map<block_id_t, block_change_t> &changes,
                                    const std::vector<page_txn_t *> &txns,
                                    fifo_enforcer_write_token_t index_write_token) {
    rassert(!changes.empty());
    std::vector<block_token_tstamp_t> blocks_by_tokens;
    blocks_by_tokens.reserve(changes.size());

    std::vector<ancillary_info_t> ancillary_infos;
    std::vector<buf_write_info_t> write_infos;
    ancillary_infos.reserve(changes.size());
    write_infos.reserve(changes.size());

    {
        ASSERT_NO_CORO_WAITING;

        for (auto it = changes.begin(); it != changes.end(); ++it) {
            if (it->second.modified) {
                if (it->second.page == NULL) {
                    // The block is deleted.
                    blocks_by_tokens.push_back(block_token_tstamp_t(it->first,
                                                                    true,
                                                                    counted_t<standard_block_token_t>(),
                                                                    repli_timestamp_t::invalid,
                                                                    NULL));
                } else {
                    // RSP: We could probably free the resources of
                    // snapshotted_dirtied_pages_ a bit sooner than we do.

                    page_t *page = it->second.page;
                    if (page->block_token().has()) {
                        // It's already on disk, we're not going to flush it.
                        blocks_by_tokens.push_back(block_token_tstamp_t(it->first,
                                                                        false,
                                                                        page->block_token(),
                                                                        it->second.tstamp,
                                                                        page));
                    } else {
                        // We can't be in the process of loading a block we're going
                        // to write for which we don't have a block token.  That's
                        // because we _actually dirtied the page_.  We had to have
                        // acquired the buf, and the only way to get rid of the buf
                        // is for it to be evicted, in which case the block token
                        // would be non-empty.

                        rassert(page->is_loaded());

                        // KSI: Is there a page_acq_t for this buf we're writing?  Is it
                        // possible that we might be trying to do an unbacked eviction
                        // for this page right now?  (No, we don't do that yet.)
                        write_infos.push_back(buf_write_info_t(page->get_loaded_ser_buffer(),
                                                               page->get_page_buf_size(),
                                                               it->first));
                        ancillary_infos.push_back(ancillary_info_t(it->second.tstamp,
                                                                   page));
                    }
                }
            } else {
                // We only touched the page.
                blocks_by_tokens.push_back(block_token_tstamp_t(it->first,
                                                                false,
                                                                counted_t<standard_block_token_t>(),
                                                                it->second.tstamp,
                                                                NULL));
            }
        }
    }

    cond_t blocks_released_cond;
    {
        on_thread_t th(page_cache->serializer_->home_thread());

        struct : public iocallback_t, public cond_t {
            void on_io_complete() {
                pulse();
            }
        } blocks_releasable_cb;

        std::vector<counted_t<standard_block_token_t> > tokens
            = page_cache->serializer_->block_writes(write_infos,
                                                    /* disk account is overridden
                                                     * by merger_serializer_t */
                                                    DEFAULT_DISK_ACCOUNT,
                                                    &blocks_releasable_cb);

        rassert(tokens.size() == write_infos.size());
        rassert(write_infos.size() == ancillary_infos.size());
        for (size_t i = 0; i < write_infos.size(); ++i) {
            blocks_by_tokens.push_back(block_token_tstamp_t(write_infos[i].block_id,
                                                            false,
                                                            std::move(tokens[i]),
                                                            ancillary_infos[i].tstamp,
                                                            ancillary_infos[i].page));
        }

        // KSI: Unnecessary copying between blocks_by_tokens and write_ops, inelegant
        // representation of deletion/touched blocks in blocks_by_tokens.
        std::vector<index_write_op_t> write_ops;
        write_ops.reserve(blocks_by_tokens.size());

        for (auto it = blocks_by_tokens.begin(); it != blocks_by_tokens.end();
             ++it) {
            if (it->is_deleted) {
                write_ops.push_back(index_write_op_t(it->block_id,
                                                     counted_t<standard_block_token_t>(),
                                                     repli_timestamp_t::invalid));
            } else if (it->block_token.has()) {
                write_ops.push_back(index_write_op_t(it->block_id,
                                                     it->block_token,
                                                     it->tstamp));
            } else {
                write_ops.push_back(index_write_op_t(it->block_id,
                                                     boost::none,
                                                     it->tstamp));
            }
        }

        blocks_releasable_cb.wait();

        // All blocks have been written. Update the block tokens and free the
        // associated snapshots on the cache thread.
        coro_t::spawn_on_thread([&]() {
            // Update the block tokens of the written blocks
            for (auto it = blocks_by_tokens.begin(); it != blocks_by_tokens.end(); ++it) {
                if (it->block_token.has() && it->page != NULL) {
                    // We know page is still a valid pointer because of the page_ptr_t in
                    // snapshotted_dirtied_pages_.

                    // KSI: This assertion would fail if we try to force-evict the page
                    // simultaneously as this write.
                    rassert(!it->page->block_token().has());
                    eviction_bag_t *old_bag
                        = page_cache->evicter().correct_eviction_category(it->page);
                    it->page->init_block_token(std::move(it->block_token), page_cache);
                    page_cache->evicter().change_to_correct_eviction_bag(old_bag, it->page);
                }
            }

            for (auto &txn : txns) {
                for (size_t i = 0, e = txn->snapshotted_dirtied_pages_.size(); i < e; ++i) {
                    txn->snapshotted_dirtied_pages_[i].ptr.reset_page_ptr(page_cache);
                    page_cache->consider_evicting_current_page(
                        txn->snapshotted_dirtied_pages_[i].block_id);
                }
                txn->snapshotted_dirtied_pages_.clear();
                txn->throttler_acq_.update_dirty_page_count(0);
            }
            blocks_released_cond.pulse();
        }, page_cache->home_thread());

        fifo_enforcer_sink_t::exit_write_t exiter(&page_cache->index_write_sink_->sink,
                                                  index_write_token);
        exiter.wait();
        new_mutex_in_line_t mutex_acq(&page_cache->index_write_sink_->mutex);
        exiter.end();

        rassert(!write_ops.empty());
        mutex_acq.acq_signal()->wait();
        page_cache->serializer_->index_write(&mutex_acq,
                                             write_ops);
    }

    // Wait until the block release coroutine has finished to we can safely
    // continue (this is important because once we return, a page transaction
    // or even the whole page cache might get destructed).
    blocks_released_cond.wait();
}

void page_cache_t::do_flush_txn_set(page_cache_t *page_cache,
                                    std::map<block_id_t, block_change_t> *changes_ptr,
                                    const std::vector<page_txn_t *> &txns) {
    // This is called with spawn_now_dangerously!  The reason is partly so that we
    // don't put a zillion coroutines on the message loop when doing a bunch of
    // reads.  The other reason is that passing changes through a std::bind without
    // copying it would be very annoying.
    page_cache->assert_thread();

    // We're going to flush these transactions.  First we need to figure out what the
    // set of changes we're actually doing is, since any transaction may have touched
    // the same blocks.

    std::map<block_id_t, block_change_t> changes = std::move(*changes_ptr);
    rassert(!changes.empty());

    fifo_enforcer_write_token_t index_write_token
        = page_cache->index_write_source_.enter_write();

    // Okay, yield, thank you.
    coro_t::yield();
    do_flush_changes(page_cache, changes, txns, index_write_token);

    // Flush complete.

    // KSI: Can't we remove_txn_set_from_graph before flushing?  It would make some
    // data structures smaller.
    page_cache_t::remove_txn_set_from_graph(page_cache, txns);
}

std::vector<page_txn_t *> page_cache_t::maximal_flushable_txn_set(page_txn_t *base) {
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
    // construct the return vector at the end of the function.
    std::vector<page_txn_t *> colored;

    rassert(!base->spawned_flush_);
    rassert(base->began_waiting_for_flush_);
    rassert(base->mark_ == page_txn_t::marked_not);
    base->mark_ = page_txn_t::marked_blue;
    blue.push_back(base);
    colored.push_back(base);

    while (!blue.empty()) {
        page_txn_t *txn = blue.back();
        blue.pop_back();

        rassert(!txn->spawned_flush_);
        rassert(txn->began_waiting_for_flush_);
        rassert(txn->mark_ == page_txn_t::marked_blue);

        bool poisoned = false;
        for (auto it = txn->preceders_.begin(); it != txn->preceders_.end(); ++it) {
            page_txn_t *prec = *it;
            if (prec->spawned_flush_) {
                rassert(prec->mark_ == page_txn_t::marked_not);
            } else if (!prec->began_waiting_for_flush_
                       || prec->mark_ == page_txn_t::marked_red) {
                poisoned = true;
            } else if (prec->mark_ == page_txn_t::marked_not) {
                prec->mark_ = page_txn_t::marked_blue;
                blue.push_back(prec);
                colored.push_back(prec);
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
                    colored.push_back(subs);
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
            *it++ = *jt++;
        } else {
            rassert(mark == page_txn_t::marked_red);
            ++jt;
        }
    }

    colored.erase(it, colored.end());
    return colored;
}

void page_cache_t::im_waiting_for_flush(page_txn_t *base) {
    assert_thread();
    rassert(base->began_waiting_for_flush_);
    rassert(!base->spawned_flush_);
    ASSERT_FINITE_CORO_WAITING;

    std::vector<page_txn_t *> flush_set
        = page_cache_t::maximal_flushable_txn_set(base);
    if (!flush_set.empty()) {
        for (auto it = flush_set.begin(); it != flush_set.end(); ++it) {
            rassert(!(*it)->spawned_flush_);
            (*it)->spawned_flush_ = true;
        }

        std::map<block_id_t, block_change_t> changes
            = page_cache_t::compute_changes(flush_set);

        if (!changes.empty()) {
            coro_t::spawn_now_dangerously(std::bind(&page_cache_t::do_flush_txn_set,
                                                    this,
                                                    &changes,
                                                    flush_set));
        } else {
            // Flush complete.  do_flush_txn_set does this in the write case.
            page_cache_t::remove_txn_set_from_graph(this, flush_set);
        }
    }
}


}  // namespace alt
