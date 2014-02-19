// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "buffer_cache/alt/page_cache.hpp"

#include <algorithm>
#include <stack>

#include "arch/runtime/coroutines.hpp"
#include "concurrency/auto_drainer.hpp"
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

void tracker_acq_t::update_dirty_page_count(int64_t new_count) {
    if (new_count > semaphore_acq_.count()) {
        semaphore_acq_.change_count(new_count);
    }
}

page_read_ahead_cb_t::page_read_ahead_cb_t(serializer_t *serializer,
                                           page_cache_t *page_cache,
                                           uint64_t bytes_to_send)
    : serializer_(serializer), page_cache_(page_cache),
      bytes_remaining_(bytes_to_send) {
    guarantee(bytes_to_send > 0);
    serializer_->register_read_ahead_cb(this);
}

page_read_ahead_cb_t::~page_read_ahead_cb_t() { }

void page_read_ahead_cb_t::offer_read_ahead_buf(
        block_id_t block_id,
        scoped_malloc_t<ser_buffer_t> *buf_ptr,
        const counted_t<standard_block_token_t> &token) {
    assert_thread();
    scoped_malloc_t<ser_buffer_t> buf = std::move(*buf_ptr);

    if (bytes_remaining_ == 0) {
        return;
    }

    // Notably, this code relies on do_on_thread to preserve callback order (which it
    // does do).

    uint32_t size = token->block_size().ser_value();
    if (bytes_remaining_ < size) {
        bytes_remaining_ = 0;
    } else {
        bytes_remaining_ -= size;
        do_on_thread(page_cache_->home_thread(),
                     std::bind(&page_cache_t::add_read_ahead_buf,
                               page_cache_,
                               block_id,
                               buf.release(),
                               token));
    }

    if (bytes_remaining_ == 0) {
        // We know bytes_remaining_ was just set to 0, so this is the only time we
        // call destroy_read_ahead_cb.
        do_on_thread(page_cache_->home_thread(),
                     std::bind(&page_cache_t::have_read_ahead_cb_destroyed,
                               page_cache_));
    }
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

void page_cache_t::resize_current_pages_to_id(block_id_t block_id) {
    if (current_pages_.size() <= block_id) {
        current_pages_.resize(block_id + 1, NULL);
    }
}

void page_cache_t::add_read_ahead_buf(block_id_t block_id,
                                      ser_buffer_t *buf_ptr,
                                      const counted_t<standard_block_token_t> &token) {
    assert_thread();

    scoped_malloc_t<ser_buffer_t> buf(buf_ptr);

    if (!evicter_.interested_in_read_ahead_block(token->block_size().ser_value())) {
        have_read_ahead_cb_destroyed();
        return;
    }

    resize_current_pages_to_id(block_id);
    if (current_pages_[block_id] != NULL) {
        return;
    }

    current_pages_[block_id] = new current_page_t(std::move(buf), token, this);
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
    }
}

void page_cache_t::read_ahead_cb_is_destroyed() {
    assert_thread();
    read_ahead_cb_existence_.reset();
}


page_cache_t::page_cache_t(serializer_t *serializer,
                           const page_cache_config_t &config,
                           memory_tracker_t *tracker)
    : dynamic_config_(config),
      serializer_(serializer),
      free_list_(serializer),
      evicter_(tracker, config.memory_limit),
      read_ahead_cb_(NULL),
      drainer_(make_scoped<auto_drainer_t>()) {

    const bool start_read_ahead = config.memory_limit > 0;
    if (start_read_ahead) {
        read_ahead_cb_existence_ = drainer_->lock();
    }

    {
        on_thread_t thread_switcher(serializer->home_thread());
        if (start_read_ahead) {
            read_ahead_cb_ = new page_read_ahead_cb_t(serializer, this,
                                                      config.memory_limit);
        }
        default_reads_account_.init(serializer->home_thread(),
                                    serializer->make_io_account(config.io_priority_reads));
        writes_account_.init(serializer->home_thread(),
                             serializer->make_io_account(config.io_priority_writes));
        index_write_sink_.init(new fifo_enforcer_sink_t);
        recencies_ = serializer->get_all_recencies();
    }
}

page_cache_t::~page_cache_t() {
    assert_thread();

    have_read_ahead_cb_destroyed();

    drainer_.reset();
    for (auto it = current_pages_.begin(); it != current_pages_.end(); ++it) {
        delete *it;
    }

    {
        /* IO accounts must be destroyed on the thread they were created on */
        on_thread_t thread_switcher(serializer_->home_thread());
        // Resetting default_reads_account_ and writes_account_ is opportunistically
        // done here, instead of making their destructors switch back to the
        // serializer thread a second and third time.
        default_reads_account_.reset();
        writes_account_.reset();
        index_write_sink_.reset();
    }
}

// We go a bit old-school, with a self-destroying callback.
class flush_and_destroy_txn_waiter_t : public signal_t::subscription_t {
public:
    flush_and_destroy_txn_waiter_t(auto_drainer_t::lock_t &&lock,
                                   page_txn_t *txn,
                                   std::function<void(tracker_acq_t)> on_flush_complete)
        : lock_(std::move(lock)),
          txn_(txn),
          on_flush_complete_(std::move(on_flush_complete)) { }

private:
    void run() {
        // Tell everybody without delay that the flush is complete.
        on_flush_complete_(std::move(txn_->tracker_acq_));

        // We have to do the rest _later_ because of signal_t::subscription_t not
        // allowing reentrant signal_t::subscription_t::reset() calls, and the like,
        // even though it would be valid.
        coro_t::spawn_sometime(std::bind(&flush_and_destroy_txn_waiter_t::kill_ourselves,
                                         this));
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
    std::function<void(tracker_acq_t)> on_flush_complete_;

    DISABLE_COPYING(flush_and_destroy_txn_waiter_t);
};

void page_cache_t::flush_and_destroy_txn(
        scoped_ptr_t<page_txn_t> txn,
        std::function<void(tracker_acq_t)> on_flush_complete) {
    rassert(txn->live_acqs_.empty(),
            "current_page_acq_t lifespan exceeds its page_txn_t's");
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
                "Expected block %" PR_BLOCK_ID "not to be deleted "
                "(should you have used alt_create_t::create?).",
                block_id);
        current_pages_[block_id] = new current_page_t();
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

    scoped_malloc_t<ser_buffer_t> buf = serializer_->malloc();

#if !defined(NDEBUG) || defined(VALGRIND)
    // KSI: This should actually _not_ exist -- we are ignoring legitimate errors
    // where we write uninitialized data to disk.
    memset(buf.get()->cache_data, 0xCD, serializer_->max_block_size().value());
#endif

    set_recency_for_block_id(block_id, repli_timestamp_t::distant_past);
    resize_current_pages_to_id(block_id);
    if (current_pages_[block_id] == NULL) {
        current_pages_[block_id] =
            new current_page_t(serializer_->max_block_size(),
                               std::move(buf),
                               this);
    } else {
        current_pages_[block_id]->make_non_deleted(serializer_->max_block_size(),
                                                   std::move(buf),
                                                   this);
    }

    return current_pages_[block_id];
}

block_size_t page_cache_t::max_block_size() const {
    assert_thread();
    return serializer_->max_block_size();
}

cache_account_t page_cache_t::create_cache_account(int priority) {
    // We assume that a priority of 100 means that the transaction should have the
    // same priority as all the non-accounted transactions together. Not sure if this
    // makes sense.

    // Be aware of rounding errors... (what can be do against those? probably just
    // setting the default io_priority_reads high enough)
    int io_priority = std::max(1, dynamic_config_.io_priority_reads * priority / 100);

    // TODO: This is a heuristic. While it might not be evil, it's not really optimal
    // either.
    int outstanding_requests_limit = std::max(1, 16 * priority / 100);

    file_account_t *io_account;
    {
        // KSI: We shouldn't have to switch to the serializer home thread.
        on_thread_t thread_switcher(serializer_->home_thread());
        io_account = serializer_->make_io_account(io_priority,
                                                  outstanding_requests_limit);
    }

    return cache_account_t(serializer_->home_thread(), io_account);
}


struct current_page_help_t {
    current_page_help_t(block_id_t _block_id, page_cache_t *_page_cache)
        : block_id(_block_id), page_cache(_page_cache) { }
    block_id_t block_id;
    page_cache_t *page_cache;
};

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
        if (current_page_ != NULL) {
            current_page_->remove_acquirer(this);
        }
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

repli_timestamp_t current_page_acq_t::recency() const {
    assert_thread();
    return recency_;
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

void current_page_acq_t::mark_deleted() {
    assert_thread();
    rassert(access_ == access_t::write);
    rassert(current_page_ != NULL);
    write_cond_.wait();
    rassert(current_page_ != NULL);
    dirtied_page_ = true;
    current_page_->mark_deleted(help());
}

bool current_page_acq_t::dirtied_page() const {
    assert_thread();
    return dirtied_page_;
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

current_page_t::current_page_t()
    : is_deleted_(false),
      last_modifier_(NULL) {
    // Increment the block version so that we can distinguish between unassigned
    // current_page_acq_t::block_version_ values (which are 0) and assigned ones.
    rassert(block_version_.debug_value() == 0);
    block_version_ = block_version_.subsequent();
}

current_page_t::current_page_t(block_size_t block_size,
                               scoped_malloc_t<ser_buffer_t> buf,
                               page_cache_t *page_cache)
    : page_(new page_t(block_size, std::move(buf), page_cache), page_cache),
      is_deleted_(false),
      last_modifier_(NULL) {
    // Increment the block version so that we can distinguish between unassigned
    // current_page_acq_t::block_version_ values (which are 0) and assigned ones.
    rassert(block_version_.debug_value() == 0);
    block_version_ = block_version_.subsequent();
}

current_page_t::current_page_t(scoped_malloc_t<ser_buffer_t> buf,
                               const counted_t<standard_block_token_t> &token,
                               page_cache_t *page_cache)
    : page_(new page_t(std::move(buf), token, page_cache), page_cache),
      is_deleted_(false),
      last_modifier_(NULL) {
    // Increment the block version so that we can distinguish between unassigned
    // current_page_acq_t::block_version_ values (which are 0) and assigned ones.
    rassert(block_version_.debug_value() == 0);
    block_version_ = block_version_.subsequent();
}

current_page_t::~current_page_t() {
    rassert(acquirers_.empty());
    rassert(last_modifier_ == NULL);
}

void current_page_t::make_non_deleted(block_size_t block_size,
                                      scoped_malloc_t<ser_buffer_t> buf,
                                      page_cache_t *page_cache) {
    rassert(is_deleted_);
    rassert(!page_.has());
    is_deleted_ = false;
    page_.init(new page_t(block_size, std::move(buf), page_cache), page_cache);
}

void current_page_t::add_acquirer(current_page_acq_t *acq) {
    current_page_acq_t *back = acquirers_.tail();
    const block_version_t prev_version
        = (back == NULL ? block_version_ : back->block_version_);
    const repli_timestamp_t prev_recency
        = (back == NULL
           ? acq->page_cache()->recency_for_block_id(acq->block_id())
           : back->recency_);

    acq->block_version_ = acq->access_ == access_t::write ?
        prev_version.subsequent() : prev_version;

    if (acq->access_ == access_t::read) {
        rassert(acq->the_txn_ == NULL);
        acq->recency_ = prev_recency;
    } else {
        rassert(acq->the_txn_ != NULL);
        // KSI: We pass invalid timestamps for some set_metainfo calls and such.
        // Shouldn't we never pass invalid timestamps?  It would be simpler.

        // KSI: We do this (for now) to play nice with the current stats block
        // code.  But ideally there would never be an out-of-order recency.
        acq->recency_ = superceding_recency(prev_recency,
                                            acq->the_txn_->this_txn_recency_);
    }

    acquirers_.push_back(acq);
    pulse_pulsables(acq);
}

void current_page_t::remove_acquirer(current_page_acq_t *acq) {
    current_page_acq_t *next = acquirers_.next(acq);
    acquirers_.remove(acq);
    if (next != NULL) {
        pulse_pulsables(next);
    } else {
        if (is_deleted_) {
            acq->page_cache()->free_list()->release_block_id(acq->block_id());
        }
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

                // We use the default_reads_account() here because ugh.

                // LSI: We should just gather block tokens up front.
                cur->snapshotted_page_.init(
                        the_page_for_read_or_deleted(help,
                                                     help.page_cache->default_reads_account()),
                        help.page_cache);
                cur->current_page_ = NULL;
                acquirers_.remove(cur);
                // KSI: Dedup this with remove_acquirer.
                if (is_deleted_) {
                    cur->page_cache()->free_list()->release_block_id(acq->block_id());
                }
            }
            cur = next;
        } else {
            // Even the first write-acquirer gets read access (there's no need for an
            // "intent" mode).  But subsequent acquirers need to wait, because the
            // write-acquirer might modify the value.
            if (acquirers_.prev(cur) == NULL) {
                // (It gets exclusive write access if there's no preceding reader.)
                if (is_deleted_) {
                    // Also, if the block is in an "is_deleted_" state right now, we
                    // need to put it into a non-deleted state.  We initialize the
                    // page to a full-sized page.
                    // TODO: We should consider whether we really want this behavior.

                    scoped_malloc_t<ser_buffer_t> buf
                        = help.page_cache->serializer()->malloc();

#if !defined(NDEBUG) || defined(VALGRIND)
                    // KSI: This should actually _not_ exist -- we are ignoring
                    // legitimate errors where we write uninitialized data to disk.
                    memset(buf.get()->cache_data, 0xCD,
                           help.page_cache->serializer()->max_block_size().value());
#endif

                    page_.init(new page_t(help.page_cache->serializer()->max_block_size(),
                                          std::move(buf),
                                          help.page_cache),
                               help.page_cache);
                    is_deleted_ = false;
                }
                block_version_ = cur->block_version_;
                help.page_cache->set_recency_for_block_id(acq->block_id(),
                                                            cur->recency_);
                cur->pulse_write_available();
            }
            break;
        }
    }
}

void current_page_t::mark_deleted(current_page_help_t help) {
    rassert(!is_deleted_);
    is_deleted_ = true;
    help.page_cache->set_recency_for_block_id(help.block_id,
                                              repli_timestamp_t::invalid);
    page_.reset();
}

void current_page_t::convert_from_serializer_if_necessary(current_page_help_t help,
                                                          cache_account_t *account) {
    rassert(!is_deleted_);
    if (!page_.has()) {
        page_.init(new page_t(help.block_id, help.page_cache, account),
                   help.page_cache);
    }
}

page_t *current_page_t::the_page_for_read(current_page_help_t help,
                                          cache_account_t *account) {
    guarantee(!is_deleted_);
    convert_from_serializer_if_necessary(help, account);
    return page_.get_page_for_read();
}

page_t *current_page_t::the_page_for_read_or_deleted(current_page_help_t help,
                                                     cache_account_t *account) {
    if (is_deleted_) {
        return NULL;
    } else {
        return the_page_for_read(help, account);
    }
}

page_t *current_page_t::the_page_for_write(current_page_help_t help,
                                           cache_account_t *account) {
    guarantee(!is_deleted_);
    convert_from_serializer_if_necessary(help, account);
    return page_.get_page_for_write(help.page_cache);
}

page_txn_t *current_page_t::change_last_modifier(page_txn_t *new_last_modifier) {
    rassert(new_last_modifier != NULL);
    page_txn_t *ret = last_modifier_;
    last_modifier_ = new_last_modifier;
    return ret;
}

page_txn_t::page_txn_t(page_cache_t *page_cache,
                       repli_timestamp_t txn_recency,
                       tracker_acq_t tracker_acq,
                       cache_conn_t *cache_conn)
    : page_cache_(page_cache),
      cache_conn_(cache_conn),
      tracker_acq_(std::move(tracker_acq)),
      this_txn_recency_(txn_recency),
      began_waiting_for_flush_(false),
      spawned_flush_(false) {
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
    // We can't add ourselves as a preceder, we have to avoid that.
    rassert(preceder != this);
    // The flush_complete_cond_ is pulsed at the same time that this txn is removed
    // entirely from the txn graph, so we can't be adding preceders after that point.
    rassert(!preceder->flush_complete_cond_.is_pulsed());

    // RSP: performance
    if (std::find(preceders_.begin(), preceders_.end(), preceder)
        == preceders_.end()) {
        preceders_.push_back(preceder);
        preceder->subseqers_.push_back(this);
    }
}

void page_txn_t::remove_preceder(page_txn_t *preceder) {
    // RSP: performance
    auto it = std::find(preceders_.begin(), preceders_.end(), preceder);
    rassert(it != preceders_.end());
    preceders_.erase(it);
}

void page_txn_t::remove_subseqer(page_txn_t *subseqer) {
    // RSP: performance
    auto it = std::find(subseqers_.begin(), subseqers_.end(), subseqer);
    rassert(it != subseqers_.end());
    subseqers_.erase(it);
}

page_txn_t::~page_txn_t() {
    guarantee(flush_complete_cond_.is_pulsed());

    guarantee(preceders_.empty());
    guarantee(subseqers_.empty());
}

void page_txn_t::add_acquirer(current_page_acq_t *acq) {
    rassert(acq->access_ == access_t::write);
    live_acqs_.push_back(acq);
}

void page_txn_t::remove_acquirer(current_page_acq_t *acq) {
    guarantee(acq->access_ == access_t::write);
    // This is called by acq's destructor.
    {
        auto it = std::find(live_acqs_.begin(), live_acqs_.end(), acq);
        rassert(it != live_acqs_.end());
        live_acqs_.erase(it);
    }

    // We check if acq->read_cond_.is_pulsed() so that if we delete the acquirer
    // before we got any kind of access to the block, then we can't have dirtied the
    // page or touched the page.

    if (acq->read_cond_.is_pulsed()) {
        // It's not snapshotted because you can't snapshot write acqs.  (We
        // rely on this fact solely because we need to grab the block_id_t
        // and current_page_acq_t currently doesn't know it.)
        rassert(acq->current_page_ != NULL);

        const block_version_t block_version = acq->block_version();

        if (acq->dirtied_page()) {
            // We know we hold an exclusive lock.
            rassert(acq->write_cond_.is_pulsed());

            // Set the last modifier while current_page_ is non-null (and while we're the
            // exclusive holder).
            page_txn_t *const previous_modifier
                = acq->current_page_->change_last_modifier(this);

            if (previous_modifier != this) {
                // RSP: Performance (in the assertion).
                rassert(pages_modified_last_.end()
                        == std::find(pages_modified_last_.begin(),
                                     pages_modified_last_.end(),
                                     acq->current_page_));
                pages_modified_last_.push_back(acq->current_page_);

                if (previous_modifier != NULL) {
                    // RSP: Performance.
                    auto it = std::find(previous_modifier->pages_modified_last_.begin(),
                                        previous_modifier->pages_modified_last_.end(),
                                        acq->current_page_);
                    rassert(it != previous_modifier->pages_modified_last_.end());
                    previous_modifier->pages_modified_last_.erase(it);

                    connect_preceder(previous_modifier);
                }
            }

            // Declare readonly (so that we may declare acq snapshotted).
            acq->declare_readonly();
            acq->declare_snapshotted();

            // Since we snapshotted the lead acquirer, it gets detached.
            rassert(acq->current_page_ == NULL);
            // Steal the snapshotted page_ptr_t.
            page_ptr_t local = std::move(acq->snapshotted_page_);
            // It's okay to have two dirtied_page_t's or touched_page_t's for the
            // same block id -- compute_changes handles this.
            snapshotted_dirtied_pages_.push_back(dirtied_page_t(block_version,
                                                                acq->block_id(),
                                                                std::move(local),
                                                                acq->recency()));
            // If you keep writing and reacquiring the same page, though, the count
            // might be off and you could excessively throttle new operations.
            tracker_acq_.update_dirty_page_count(snapshotted_dirtied_pages_.size());
        } else {
            // It's okay to have two dirtied_page_t's or touched_page_t's for the
            // same block id -- compute_changes handles this.
            touched_pages_.push_back(touched_page_t(block_version, acq->block_id(),
                                                    acq->recency()));
        }
    }
}

void page_txn_t::announce_waiting_for_flush() {
    rassert(live_acqs_.empty());
    rassert(!began_waiting_for_flush_);
    began_waiting_for_flush_ = true;
    std::set<page_txn_t *> txns;
    txns.insert(this);
    page_cache_->im_waiting_for_flush(std::move(txns));
}

std::map<block_id_t, page_cache_t::block_change_t>
page_cache_t::compute_changes(const std::set<page_txn_t *> &txns) {
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
                                  d.tstamp);

            auto res = changes.insert(std::make_pair(d.block_id, change));

            if (!res.second) {
                // The insertion failed -- we need to use the newer version.
                auto const jt = res.first;
                // The versions can't be the same for different write operations.
                rassert(jt->second.version != d.block_version,
                        "equal versions on block %" PRIi64 ": %" PRIu64,
                        d.block_id,
                        d.block_version.debug_value());
                if (jt->second.version < d.block_version) {
                    rassert(d.tstamp ==
                            superceding_recency(jt->second.tstamp, d.tstamp));
                    jt->second = change;
                }
            }
        }

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
                }
            }
        }
    }

    return changes;
}

std::set<page_txn_t *>
page_cache_t::remove_txn_set_from_graph(page_cache_t *page_cache,
                                        const std::set<page_txn_t *> &txns) {
    page_cache->assert_thread();

    std::set<page_txn_t *> unblocked;

    for (auto it = txns.begin(); it != txns.end(); ++it) {
        // We want detaching the subsequers and preceders to happen at the same time
        // that the flush_complete_cond_ is pulsed.  That way connect_preceder can
        // check if flush_complete_cond_ has been pulsed.
        ASSERT_FINITE_CORO_WAITING;
        page_txn_t *txn = *it;
        {
            for (auto jt = txn->subseqers_.begin(); jt != txn->subseqers_.end(); ++jt) {
                (*jt)->remove_preceder(txn);
                if (txns.find(*jt) == txns.end()) {
                    if ((*jt)->began_waiting_for_flush_ && !(*jt)->spawned_flush_) {
                        unblocked.insert(*jt);
                    }
                }
            }
            txn->subseqers_.clear();
        }

        for (auto jt = txn->preceders_.begin(); jt != txn->preceders_.end(); ++jt) {
            // All our preceders should be from our own txn set.
            rassert(txns.find(*jt) != txns.end());
            (*jt)->remove_subseqer(txn);
        }
        txn->preceders_.clear();

        // KSI: Maybe we could remove pages_modified_last_ earlier?  Like when we
        // begin the index write? (but that's the wrong thread) Or earlier?
        for (auto jt = txn->pages_modified_last_.begin();
             jt != txn->pages_modified_last_.end();
             ++jt) {
            current_page_t *current_page = *jt;
            rassert(current_page->last_modifier_ == txn);
            current_page->last_modifier_ = NULL;
        }
        txn->pages_modified_last_.clear();

        if (txn->cache_conn_ != NULL) {
            rassert(txn->cache_conn_->newest_txn_ == txn);
            txn->cache_conn_->newest_txn_ = NULL;
            txn->cache_conn_ = NULL;
        }

        txn->flush_complete_cond_.pulse();
    }

    return unblocked;
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
                    if (page->block_token_.has()) {
                        // It's already on disk, we're not going to flush it.
                        blocks_by_tokens.push_back(block_token_tstamp_t(it->first,
                                                                        false,
                                                                        page->block_token_,
                                                                        it->second.tstamp,
                                                                        page));
                    } else {
                        // We can't be in the process of loading a block we're going
                        // to write for which we don't have a block token.  That's
                        // because we _actually dirtied the page_.  We had to have
                        // acquired the buf, and the only way to get rid of the buf
                        // is for it to be evicted, in which case the block token
                        // would be non-empty.
                        rassert(page->destroy_ptr_ == NULL);
                        rassert(page->buf_.has());

                        // KSI: Is there a page_acq_t for this buf we're writing?  Is it
                        // possible that we might be trying to do an unbacked eviction
                        // for this page right now?  (No, we don't do that yet.)
                        write_infos.push_back(buf_write_info_t(page->buf_.get(),
                                                               block_size_t::unsafe_make(page->ser_buf_size_),
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

    {
        on_thread_t th(page_cache->serializer_->home_thread());

        struct : public iocallback_t, public cond_t {
            void on_io_complete() {
                pulse();
            }
        } blocks_releasable_cb;

        std::vector<counted_t<standard_block_token_t> > tokens
            = page_cache->serializer_->block_writes(write_infos,
                                                    page_cache->writes_account_.get(),
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

        // RSP: Unnecessary copying between blocks_by_tokens and write_ops, inelegant
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
                                                     std::move(it->block_token),
                                                     it->tstamp));
            } else {
                write_ops.push_back(index_write_op_t(it->block_id,
                                                     boost::none,
                                                     it->tstamp));
            }
        }

        blocks_releasable_cb.wait();

        // LSI: Pass in the exiter to index_write, so that subsequent index_write
        // operations don't have to wait for one another to finish.  (Note: Doing
        // this requires some sort of semaphore to prevent a zillion index_writes to
        // queue up.)
        fifo_enforcer_sink_t::exit_write_t exiter(page_cache->index_write_sink_.get(),
                                                  index_write_token);
        exiter.wait();

        rassert(!write_ops.empty());
        page_cache->serializer_->index_write(write_ops,
                                             page_cache->writes_account_.get());
    }

    // Set the page_t's block token field to their new block tokens.  KSI: Can we
    // do this earlier?  Do we have to wait for blocks_releasable_cb?  It doesn't
    // matter that much as long as we have some way to prevent parallel forced
    // eviction from happening, though.  (That doesn't happen yet.)

    // KSI: We could pass these block tokens as a message back to the cache thread
    // and set them earlier -- at least we could after blocks_releasable_cb.wait()
    // and before the index_write.
    for (auto it = blocks_by_tokens.begin(); it != blocks_by_tokens.end(); ++it) {
        if (it->block_token.has() && it->page != NULL) {
            // We know page is still a valid pointer because of the page_ptr_t in
            // snapshotted_dirtied_pages_.

            // KSI: This assertion would fail if we try to force-evict the page
            // simultaneously as this write.
            rassert(!it->page->block_token_.has());
            eviction_bag_t *old_bag
                = page_cache->evicter().correct_eviction_category(it->page);
            it->page->block_token_ = std::move(it->block_token);
            page_cache->evicter().change_to_correct_eviction_bag(old_bag, it->page);
        }
    }
}

void page_cache_t::do_flush_txn_set(page_cache_t *page_cache,
                                    std::map<block_id_t, block_change_t> *changes_ptr,
                                    const std::set<page_txn_t *> &txns) {
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
    do_flush_changes(page_cache, changes, index_write_token);

    // Flush complete.

    // KSI: Can't we remove_txn_set_from_graph before flushing?  Make pulsing
    // flush_complete_cond_ a separate thing to do?
    std::set<page_txn_t *> unblocked
        = page_cache_t::remove_txn_set_from_graph(page_cache, txns);

    page_cache->im_waiting_for_flush(std::move(unblocked));
}

bool page_cache_t::exists_flushable_txn_set(page_txn_t *txn,
                                            std::set<page_txn_t *> *flush_set_out) {
    assert_thread();
    std::set<page_txn_t *> builder;
    std::stack<page_txn_t *> stack;
    stack.push(txn);

    while (!stack.empty()) {
        page_txn_t *t = stack.top();
        stack.pop();

        if (!t->began_waiting_for_flush_) {
            // We can't flush this txn or the others if there's a preceder that
            // hasn't begun waiting for flush.
            return false;
        }

        if (t->spawned_flush_) {
            // We ignore (and don't continue to traverse) nodes that are already
            // spawned flushing.  They aren't part of our txn set, and they didn't
            // depend on txn before they started flushing.
            continue;
        }

        auto res = builder.insert(t);
        if (res.second) {
            // An insertion actually took place -- it's newly processed!  Add its
            // preceders to the stack so that we can process them.
            for (auto it = t->preceders_.begin(); it != t->preceders_.end(); ++it) {
                stack.push(*it);
            }
        }
    }

    // We built up some txn set.  We know it's non-empty because at least txn is in
    // there.  Success!
    *flush_set_out = std::move(builder);
    return true;
}

void page_cache_t::im_waiting_for_flush(std::set<page_txn_t *> queue) {
    assert_thread();
    ASSERT_FINITE_CORO_WAITING;

    while (!queue.empty()) {
        // The only reason we know this page_txn_t isn't destructed is because we
        // know the coroutine doesn't block, and destructing the page_txn_t doesn't
        // happen after some condition variable gets pulsed.
        page_txn_t *txn;
        {
            auto it = queue.begin();
            txn = *it;
            queue.erase(it);
        }

        rassert(txn->began_waiting_for_flush_);
        rassert(!txn->spawned_flush_);
        rassert(txn->live_acqs_.empty());

        // We have a new node that's waiting for flush.  Before this node is flushed, we
        // will have started to flush all preceders (recursively) of this node (that are
        // waiting for flush) that this node doesn't itself precede (recursively).  Now
        // we need to ask: Are all this node's preceders (recursively) (that are ready to
        // flush and haven't started flushing yet) ready to flush?  If so, this node must
        // have been the one that pushed them over the line (since they haven't started
        // flushing yet).  So we begin flushing this node and all of its preceders
        // (recursively) in one atomic flush.


        // LSI: Every transaction is getting its own flush, basically, the way things are
        // right now.

        std::set<page_txn_t *> flush_set;
        if (exists_flushable_txn_set(txn, &flush_set)) {
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
                std::set<page_txn_t *> unblocked
                    = page_cache_t::remove_txn_set_from_graph(this, flush_set);

                for (auto it = unblocked.begin(); it != unblocked.end(); ++it) {
                    queue.insert(*it);
                }
            }
        }
    }
}


}  // namespace alt
