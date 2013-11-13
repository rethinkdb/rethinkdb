#define __STDC_LIMIT_MACROS
#include "buffer_cache/alt/page.hpp"

#include <algorithm>
#include <stack>

#include "arch/runtime/coroutines.hpp"
#include "concurrency/auto_drainer.hpp"
#include "serializer/serializer.hpp"
#include "stl_utils.hpp"

// RSI: temporary debugging macro
// #define pagef debugf
#define pagef(...) do { } while (0)

namespace alt {

page_cache_t::page_cache_t(serializer_t *serializer, uint64_t memory_limit)
    : serializer_(serializer),
      free_list_(serializer),
      evicter_(memory_limit),
      drainer_(new auto_drainer_t) {
    {
        on_thread_t thread_switcher(serializer->home_thread());
        reads_io_account.init(serializer->make_io_account(CACHE_READS_IO_PRIORITY));
        writes_io_account.init(serializer->make_io_account(CACHE_WRITES_IO_PRIORITY));
        index_write_sink.init(new fifo_enforcer_sink_t);
    }
}

page_cache_t::~page_cache_t() {
    drainer_.reset();
    for (auto it = current_pages_.begin(); it != current_pages_.end(); ++it) {
        delete *it;
    }

    {
        /* IO accounts must be destroyed on the thread they were created on */
        on_thread_t thread_switcher(serializer_->home_thread());
        reads_io_account.reset();
        writes_io_account.reset();
        index_write_sink.reset();
    }
}

current_page_t *page_cache_t::page_for_block_id(block_id_t block_id) {
    if (current_pages_.size() <= block_id) {
        current_pages_.resize(block_id + 1, NULL);
    }

    if (current_pages_[block_id] == NULL) {
        current_pages_[block_id] = new current_page_t();
    } else {
        rassert(!current_pages_[block_id]->is_deleted());
    }

    return current_pages_[block_id];
}

current_page_t *page_cache_t::page_for_new_block_id(block_id_t *block_id_out) {
    block_id_t block_id = free_list_.acquire_block_id();
    current_page_t *ret = page_for_new_chosen_block_id(block_id);
    *block_id_out = block_id;
    return ret;
}

current_page_t *page_cache_t::page_for_new_chosen_block_id(block_id_t block_id) {
    if (current_pages_.size() <= block_id) {
        current_pages_.resize(block_id + 1, NULL);
    }
    if (current_pages_[block_id] == NULL) {
        current_pages_[block_id] =
            new current_page_t(serializer_->get_block_size(),
                               serializer_->malloc(),
                               this);
    } else {
        current_pages_[block_id]->make_non_deleted(serializer_->get_block_size(),
                                                   serializer_->malloc(),
                                                   this);
    }

    return current_pages_[block_id];
}

block_size_t page_cache_t::max_block_size() const {
    return serializer_->get_block_size();
}

struct current_page_help_t {
    current_page_help_t(block_id_t _block_id, page_cache_t *_page_cache)
        : block_id(_block_id), page_cache(_page_cache) { }
    block_id_t block_id;
    page_cache_t *page_cache;
};

current_page_acq_t::current_page_acq_t()
    : txn_(NULL) { }

current_page_acq_t::current_page_acq_t(page_txn_t *txn,
                                       block_id_t block_id,
                                       alt_access_t access,
                                       bool create)
    : txn_(NULL) {
    init(txn, block_id, access, create);
}

current_page_acq_t::current_page_acq_t(page_txn_t *txn,
                                       alt_access_t access)
    : txn_(NULL) {
    init(txn, access);
}

void current_page_acq_t::init(page_txn_t *txn,
                              block_id_t block_id,
                              alt_access_t access,
                              bool create) {
    guarantee(txn_ == NULL);
    txn_ = txn;
    access_ = access;
    declared_snapshotted_ = false;
    block_id_ = block_id;
    if (create) {
        current_page_ = txn->page_cache()->page_for_new_chosen_block_id(block_id);
    } else {
        current_page_ = txn->page_cache()->page_for_block_id(block_id);
    }
    dirtied_page_ = false;

    txn_->add_acquirer(this);
    current_page_->add_acquirer(this);
}

void current_page_acq_t::init(page_txn_t *txn,
                              alt_access_t access) {
    guarantee(txn_ == NULL);
    txn_ = txn;
    access_ = access;
    declared_snapshotted_ = false;
    current_page_ = txn->page_cache()->page_for_new_block_id(&block_id_);
    dirtied_page_ = false;
    rassert(access == alt_access_t::write);

    txn_->add_acquirer(this);
    current_page_->add_acquirer(this);
}

current_page_acq_t::~current_page_acq_t() {
    if (txn_ != NULL) {
        txn_->remove_acquirer(this);
        if (current_page_ != NULL) {
            current_page_->remove_acquirer(this);
        }
    }
}

void current_page_acq_t::declare_readonly() {
    access_ = alt_access_t::read;
    if (current_page_ != NULL) {
        current_page_->pulse_pulsables(this);
    }
}

void current_page_acq_t::declare_snapshotted() {
    rassert(access_ == alt_access_t::read);

    // Allow redeclaration of snapshottedness.
    if (!declared_snapshotted_) {
        declared_snapshotted_ = true;
        rassert(current_page_ != NULL);
        current_page_->pulse_pulsables(this);
    }
}

signal_t *current_page_acq_t::read_acq_signal() {
    return &read_cond_;
}

signal_t *current_page_acq_t::write_acq_signal() {
    rassert(access_ == alt_access_t::write);
    return &write_cond_;
}

page_t *current_page_acq_t::current_page_for_read() {
    rassert(snapshotted_page_.has() || current_page_ != NULL);
    read_cond_.wait();
    if (snapshotted_page_.has()) {
        return snapshotted_page_.get_page_for_read();
    }
    rassert(current_page_ != NULL);
    return current_page_->the_page_for_read(help());
}

page_t *current_page_acq_t::current_page_for_write() {
    rassert(access_ == alt_access_t::write);
    rassert(current_page_ != NULL);
    write_cond_.wait();
    rassert(current_page_ != NULL);
    dirtied_page_ = true;
    return current_page_->the_page_for_write(help());
}

void current_page_acq_t::mark_deleted() {
    rassert(access_ == alt_access_t::write);
    rassert(current_page_ != NULL);
    write_cond_.wait();
    rassert(current_page_ != NULL);
    dirtied_page_ = true;
    current_page_->mark_deleted();
}

bool current_page_acq_t::dirtied_page() const {
    return dirtied_page_;
}

block_version_t current_page_acq_t::block_version() const {
    rassert(read_cond_.is_pulsed());
    return block_version_;
}


page_cache_t *current_page_acq_t::page_cache() const {
    return txn_->page_cache();
}

current_page_help_t current_page_acq_t::help() const {
    return current_page_help_t(block_id(), page_cache());
}

void current_page_acq_t::pulse_read_available(block_version_t block_version) {
    block_version_ = block_version;
    read_cond_.pulse_if_not_already_pulsed();
}

void current_page_acq_t::pulse_write_available() {
    write_cond_.pulse_if_not_already_pulsed();
}

current_page_t::current_page_t()
    : is_deleted_(false),
      last_modifier_(NULL) {
    // Increment the block version so that we can distinguish between unassigned
    // current_page_acq_t::block_version_ values (which are 0) and assigned ones.
    rassert(block_version_.debug_value() == 0);
    block_version_.increment();
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
    block_version_.increment();
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
        if (!(prev == NULL || (prev->access_ == alt_access_t::read
                               && prev->read_cond_.is_pulsed()))) {
            return;
        }
    }

    // Second, avoid re-pulsing already-pulsed chains.
    if (acq->access_ == alt_access_t::read && acq->read_cond_.is_pulsed()
        && !acq->declared_snapshotted_) {
        return;
    }

    // It's time to pulse the pulsables.
    current_page_acq_t *cur = acq;
    while (cur != NULL) {
        // We know that the previous node has read access and has been pulsed as
        // readable, so we pulse the current node as readable.
        cur->pulse_read_available(block_version_);

        if (cur->access_ == alt_access_t::read) {
            current_page_acq_t *next = acquirers_.next(cur);
            if (cur->declared_snapshotted_) {
                // Snapshotters get kicked out of the queue, to make way for
                // write-acquirers.

                // We treat deleted pages this way because a write-acquirer may
                // downgrade itself to readonly and snapshotted for the sake of
                // flushing its version of the page -- and if it deleted the page,
                // this is how it learns.
                cur->snapshotted_page_.init(the_page_for_read_or_deleted(help),
                                            cur->page_cache());
                cur->current_page_ = NULL;
                acquirers_.remove(cur);
                // RSI: Dedup this with remove_acquirer.
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
                    page_.init(new page_t(help.page_cache->serializer()->get_block_size(),
                                          help.page_cache->serializer()->malloc(),
                                          help.page_cache),
                               help.page_cache);
                    is_deleted_ = false;
                }
                pagef("block version increment on %" PRIi64 " from %" PRIu64 "\n",
                      acq->block_id(), block_version_.debug_value());
                block_version_.increment();
                cur->pulse_write_available();
            }
            break;
        }
    }
}

void current_page_t::mark_deleted() {
    rassert(!is_deleted_);
    is_deleted_ = true;
    page_.reset();
}

void current_page_t::convert_from_serializer_if_necessary(current_page_help_t help) {
    rassert(!is_deleted_);
    if (!page_.has()) {
        page_.init(new page_t(help.block_id, help.page_cache), help.page_cache);
    }
}

page_t *current_page_t::the_page_for_read(current_page_help_t help) {
    guarantee(!is_deleted_);
    convert_from_serializer_if_necessary(help);
    return page_.get_page_for_read();
}

page_t *current_page_t::the_page_for_read_or_deleted(current_page_help_t help) {
    if (is_deleted_) {
        return NULL;
    } else {
        return the_page_for_read(help);
    }
}

page_t *current_page_t::the_page_for_write(current_page_help_t help) {
    guarantee(!is_deleted_);
    convert_from_serializer_if_necessary(help);
    return page_.get_page_for_write(help.page_cache);
}

page_txn_t *current_page_t::change_last_modifier(page_txn_t *new_last_modifier) {
    rassert(new_last_modifier != NULL);
    page_txn_t *ret = last_modifier_;
    last_modifier_ = new_last_modifier;
    return ret;
}

page_t::page_t(block_id_t block_id, page_cache_t *page_cache)
    : destroy_ptr_(NULL),
      ser_buf_size_(0),
      snapshot_refcount_(0) {
    page_cache->evicter().add_not_yet_loaded(this);
    coro_t::spawn_now_dangerously(std::bind(&page_t::load_with_block_id,
                                            this,
                                            block_id,
                                            page_cache));
}

page_t::page_t(block_size_t block_size, scoped_malloc_t<ser_buffer_t> buf,
               page_cache_t *page_cache)
    : destroy_ptr_(NULL),
      ser_buf_size_(block_size.ser_value()),
      buf_(std::move(buf)),
      snapshot_refcount_(0) {
    rassert(buf_.has());
    page_cache->evicter().add_to_evictable_unbacked(this);
}

page_t::page_t(page_t *copyee, page_cache_t *page_cache)
    : destroy_ptr_(NULL),
      ser_buf_size_(0),
      snapshot_refcount_(0) {
    page_cache->evicter().add_not_yet_loaded(this);
    coro_t::spawn_now_dangerously(std::bind(&page_t::load_from_copyee,
                                            this,
                                            copyee,
                                            page_cache));
}

page_t::~page_t() {
    if (destroy_ptr_ != NULL) {
        *destroy_ptr_ = true;
    }
}

void page_t::load_from_copyee(page_t *page, page_t *copyee,
                              page_cache_t *page_cache) {
    // This is called using spawn_now_dangerously.  We need to atomically set
    // destroy_ptr_ and do some other things.
    bool page_destroyed = false;
    rassert(page->destroy_ptr_ == NULL);
    page->destroy_ptr_ = &page_destroyed;

    auto_drainer_t::lock_t lock(page_cache->drainer_.get());
    page_ptr_t copyee_ptr(copyee, page_cache);

    // Okay, it's safe to block.
    {
        page_acq_t acq;
        acq.init(copyee, page_cache);
        acq.buf_ready_signal()->wait();

        ASSERT_FINITE_CORO_WAITING;
        if (!page_destroyed) {
            // RSP: If somehow there are no snapshotters of copyee now (besides
            // ourself), maybe we could avoid copying this memory.  We need to
            // carefully track snapshotters anyway, once we're comfortable with that,
            // we could do it.

            uint32_t ser_buf_size = copyee->ser_buf_size_;
            rassert(copyee->buf_.has());
            scoped_malloc_t<ser_buffer_t> buf = page_cache->serializer_->malloc();

            memcpy(buf.get(), copyee->buf_.get(), ser_buf_size);

            page->ser_buf_size_ = ser_buf_size;
            page->buf_ = std::move(buf);
            page->destroy_ptr_ = NULL;

            page_cache->evicter().add_now_loaded_size(page->ser_buf_size_);

            page->pulse_waiters_or_make_evictable(page_cache);
        }
    }
}


void page_t::load_with_block_id(page_t *page, block_id_t block_id,
                                page_cache_t *page_cache) {
    // This is called using spawn_now_dangerously.  We need to set
    // destroy_ptr_ before blocking the coroutine.
    bool page_destroyed = false;
    rassert(page->destroy_ptr_ == NULL);
    page->destroy_ptr_ = &page_destroyed;

    auto_drainer_t::lock_t lock(page_cache->drainer_.get());

    scoped_malloc_t<ser_buffer_t> buf;
    counted_t<standard_block_token_t> block_token;
    {
        serializer_t *const serializer = page_cache->serializer_;
        buf = serializer->malloc();  // Call malloc() on our home thread because
                                     // we'll destroy it on our home thread and
                                     // tcmalloc likes that.
        on_thread_t th(serializer->home_thread());
        block_token = serializer->index_read(block_id);
        rassert(block_token.has());
        serializer->block_read(block_token,
                               buf.get(),
                               page_cache->reads_io_account.get());
    }

    ASSERT_FINITE_CORO_WAITING;
    if (page_destroyed) {
        return;
    }

    rassert(!page->block_token_.has());
    rassert(!page->buf_.has());
    rassert(block_token.has());
    page->ser_buf_size_ = block_token->block_size().ser_value();
    page->buf_ = std::move(buf);
    page->block_token_ = std::move(block_token);
    page->destroy_ptr_ = NULL;
    page_cache->evicter().add_now_loaded_size(page->ser_buf_size_);

    page->pulse_waiters_or_make_evictable(page_cache);
}

void page_t::add_snapshotter() {
    // This may not block, because it's called at the beginning of
    // page_t::load_from_copyee.
    ASSERT_NO_CORO_WAITING;
    ++snapshot_refcount_;
}

void page_t::remove_snapshotter(page_cache_t *page_cache) {
    rassert(snapshot_refcount_ > 0);
    --snapshot_refcount_;
    if (snapshot_refcount_ == 0) {
        // Every page_acq_t is bounded by the lifetime of some page_ptr_t: either the
        // one in current_page_acq_t or its current_page_t or the one in
        // load_from_copyee.
        rassert(waiters_.empty());

        page_cache->evicter().remove_page(this);
        delete this;
    }
}

size_t page_t::num_snapshot_references() {
    return snapshot_refcount_;
}

page_t *page_t::make_copy(page_cache_t *page_cache) {
    page_t *ret = new page_t(this, page_cache);
    return ret;
}

void page_t::pulse_waiters_or_make_evictable(page_cache_t *page_cache) {
    rassert(page_cache->evicter().page_is_in_unevictable_bag(this));
    if (waiters_.empty()) {
        page_cache->evicter().move_unevictable_to_evictable(this);
    } else {
        for (page_acq_t *p = waiters_.head(); p != NULL; p = waiters_.next(p)) {
            // The waiter's not already going to have been pulsed.
            p->buf_ready_signal_.pulse();
        }
    }
}

void page_t::add_waiter(page_acq_t *acq) {
    eviction_bag_t *old_bag
        = acq->page_cache()->evicter().correct_eviction_category(this);
    waiters_.push_back(acq);
    acq->page_cache()->evicter().change_eviction_bag(old_bag, this);
    if (buf_.has()) {
        acq->buf_ready_signal_.pulse();
    } else if (destroy_ptr_ == NULL) {
        rassert(block_token_.has());
        coro_t::spawn_now_dangerously(std::bind(&page_t::load_using_block_token,
                                                this,
                                                acq->page_cache()));
    }
}

// Unevicts page.
void page_t::load_using_block_token(page_t *page, page_cache_t *page_cache) {
    // This is called using spawn_now_dangerously.  We need to set
    // destroy_ptr_ before blocking the coroutine.
    bool page_destroyed = false;
    rassert(page->destroy_ptr_ == NULL);
    page->destroy_ptr_ = &page_destroyed;

    auto_drainer_t::lock_t lock(page_cache->drainer_.get());

    counted_t<standard_block_token_t> block_token = page->block_token_;
    rassert(block_token.has());

    scoped_malloc_t<ser_buffer_t> buf;
    {
        serializer_t *const serializer = page_cache->serializer_;
        buf = serializer->malloc();  // Call malloc() on our home thread because
                                     // we'll destroy it on our home thread and
                                     // tcmalloc likes that.
        on_thread_t th(serializer->home_thread());
        serializer->block_read(block_token,
                               buf.get(),
                               page_cache->reads_io_account.get());
    }

    ASSERT_FINITE_CORO_WAITING;
    if (page_destroyed) {
        return;
    }

    rassert(page->block_token_.get() == block_token.get());
    rassert(!page->buf_.has());
    rassert(page->ser_buf_size_ == block_token->block_size().ser_value());
    block_token.reset();
    page->buf_ = std::move(buf);
    page->destroy_ptr_ = NULL;

    page->pulse_waiters_or_make_evictable(page_cache);
}

uint32_t page_t::get_page_buf_size() {
    rassert(buf_.has());
    rassert(ser_buf_size_ != 0);
    return block_size_t::unsafe_make(ser_buf_size_).value();
}

void *page_t::get_page_buf() {
    rassert(buf_.has());
    return buf_->cache_data;
}

void page_t::reset_block_token() {
    // The page is supposed to have its buffer acquired in reset_block_token -- it's
    // the thing modifying the page.  We thus assume that the page is unevictable and
    // resetting block_token_ doesn't change that.
    rassert(!waiters_.empty());
    block_token_.reset();
}


void page_t::remove_waiter(page_acq_t *acq) {
    eviction_bag_t *old_bag
        = acq->page_cache()->evicter().correct_eviction_category(this);
    waiters_.remove(acq);
    acq->page_cache()->evicter().change_eviction_bag(old_bag, this);

    // page_acq_t always has a lesser lifetime than some page_ptr_t.
    rassert(snapshot_refcount_ > 0);
}

void page_t::evict_self() {
    // A page_t can only self-evict if it has a block token.
    rassert(waiters_.empty());
    rassert(block_token_.has());
    rassert(buf_.has());
    buf_.reset();
}


page_acq_t::page_acq_t() : page_(NULL), page_cache_(NULL) {
}

void page_acq_t::init(page_t *page, page_cache_t *page_cache) {
    rassert(page_ == NULL);
    rassert(page_cache_ == NULL);
    rassert(!buf_ready_signal_.is_pulsed());
    page_ = page;
    page_cache_ = page_cache;
    page_->add_waiter(this);
}

page_acq_t::~page_acq_t() {
    if (page_ != NULL) {
        rassert(page_cache_ != NULL);
        page_->remove_waiter(this);
    }
}

bool page_acq_t::has() const {
    return page_ != NULL;
}

signal_t *page_acq_t::buf_ready_signal() {
    return &buf_ready_signal_;
}

uint32_t page_acq_t::get_buf_size() {
    buf_ready_signal_.wait();
    return page_->get_page_buf_size();
}

void *page_acq_t::get_buf_write() {
    buf_ready_signal_.wait();
    page_->reset_block_token();
    return page_->get_page_buf();
}

const void *page_acq_t::get_buf_read() {
    buf_ready_signal_.wait();
    return page_->get_page_buf();
}

free_list_t::free_list_t(serializer_t *serializer) {
    on_thread_t th(serializer->home_thread());

    next_new_block_id_ = serializer->max_block_id();
    for (block_id_t i = 0; i < next_new_block_id_; ++i) {
        if (serializer->get_delete_bit(i)) {
            free_ids_.push_back(i);
        }
    }
}

free_list_t::~free_list_t() { }

block_id_t free_list_t::acquire_block_id() {
    if (free_ids_.empty()) {
        block_id_t ret = next_new_block_id_;
        ++next_new_block_id_;
        return ret;
    } else {
        block_id_t ret = free_ids_.back();
        free_ids_.pop_back();
        return ret;
    }
}

void free_list_t::release_block_id(block_id_t block_id) {
    free_ids_.push_back(block_id);
}

page_ptr_t::page_ptr_t() : page_(NULL), page_cache_(NULL) {
}

page_ptr_t::~page_ptr_t() {
    reset();
}

page_ptr_t::page_ptr_t(page_ptr_t &&movee)
    : page_(movee.page_), page_cache_(movee.page_cache_) {
    movee.page_ = NULL;
    movee.page_cache_ = NULL;
}

page_ptr_t &page_ptr_t::operator=(page_ptr_t &&movee) {
    page_ptr_t tmp(std::move(movee));
    std::swap(page_, tmp.page_);
    std::swap(page_cache_, tmp.page_cache_);
    return *this;
}

void page_ptr_t::init(page_t *page, page_cache_t *page_cache) {
    rassert(page_ == NULL && page_cache_ == NULL);
    page_ = page;
    page_cache_ = page_cache;
    if (page_ != NULL) {
        page_->add_snapshotter();
    }
}

void page_ptr_t::reset() {
    if (page_ != NULL) {
        page_t *ptr = page_;
        page_cache_t *cache = page_cache_;
        page_ = NULL;
        page_cache_ = NULL;
        ptr->remove_snapshotter(cache);
    }
}

page_t *page_ptr_t::get_page_for_read() const {
    rassert(page_ != NULL);
    return page_;
}

page_t *page_ptr_t::get_page_for_write(page_cache_t *page_cache) {
    rassert(page_ != NULL);
    if (page_->num_snapshot_references() > 1) {
        page_ptr_t tmp(page_->make_copy(page_cache), page_cache);
        *this = std::move(tmp);
    }
    return page_;
}

page_txn_t::page_txn_t(page_cache_t *page_cache, page_txn_t *preceding_txn_or_null)
    : page_cache_(page_cache),
      began_waiting_for_flush_(false) {
    if (preceding_txn_or_null != NULL) {
        connect_preceder(preceding_txn_or_null);
    }
}

void page_txn_t::connect_preceder(page_txn_t *preceder) {
    if (preceder != this) {
        // RSI: Is this the right condition to check?
        if (!preceder->flush_complete_cond_.is_pulsed()) {
            // RSP: performance
            if (std::find(preceders_.begin(), preceders_.end(), preceder)
                == preceders_.end()) {
                preceders_.push_back(preceder);
                preceder->subseqers_.push_back(this);
            }
        }
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
    rassert(live_acqs_.empty(), "current_page_acq_t lifespan exceeds its page_txn_t's");

    // RSI: Remove this assertion when we support manually starting txn flushes
    // sooner.
    rassert(!began_waiting_for_flush_);

    if (!began_waiting_for_flush_) {
        pagef("in ~page_txn_t, going to announce waiting for flush\n");
        announce_waiting_for_flush_if_we_should();
    }

    // RSI: Do we want to wait for this here?  Or should the page_cache_t be the
    // thing that waits and destroys this object?

    // RSI: Do whatever else is necessary to implement this.

    pagef("in ~page_txn_t, waiting for flush cond\n");
    flush_complete_cond_.wait();
    pagef("in ~page_txn_t, flush cond complete\n");
}

void page_txn_t::add_acquirer(current_page_acq_t *acq) {
    live_acqs_.push_back(acq);
}

void page_txn_t::remove_acquirer(current_page_acq_t *acq) {
    // This is called by acq's destructor.
    {
        auto it = std::find(live_acqs_.begin(), live_acqs_.end(), acq);
        rassert(it != live_acqs_.end());
        live_acqs_.erase(it);
    }

    // We check if acq->read_cond_.is_pulsed() so that if we delete the acquirer
    // before we got any kind of access to the block, then we can't have dirtied the
    // page or touched the page.

    if (acq->read_cond_.is_pulsed() && acq->access_ == alt_access_t::write) {
        // It's not snapshotted because you can't snapshot write acqs.  (We
        // rely on this fact solely because we need to grab the block_id_t
        // and current_page_acq_t currently doesn't know it.)
        rassert(acq->current_page_ != NULL);

        // Get the block id while current_page_ is non-null.  (It'll become
        // null once we're snapshotted.)
        // RSI: what's up with this comment -- is the block_id() function fragile?
        const block_id_t block_id = acq->block_id();

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
                                                                block_id,
                                                                std::move(local),
                                                                repli_timestamp_t::invalid /* RSI: handle recency */));
        } else {
            // It's okay to have two dirtied_page_t's or touched_page_t's for the
            // same block id -- compute_changes handles this.
            touched_pages_.push_back(touched_page_t(block_version, block_id,
                                                    repli_timestamp_t::invalid /* RSI: handle recency */));
        }
    }
}

void page_txn_t::announce_waiting_for_flush_if_we_should() {
    if (live_acqs_.empty()) {
        rassert(!began_waiting_for_flush_);
        began_waiting_for_flush_ = true;
        page_cache_->im_waiting_for_flush(this);
    }
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
            const page_txn_t::dirtied_page_t &d = txn->snapshotted_dirtied_pages_[i];

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
                    // RSI: What if jt->second.tstamp > d.tstamp?
                    // Should we take the max of both tstamps?  Ugghh.
                    jt->second = change;
                }
            }
        }

        for (size_t i = 0, e = txn->touched_pages_.size(); i < e; ++i) {
            const page_txn_t::touched_page_t &t = txn->touched_pages_[i];

            auto res = changes.insert(std::make_pair(t.first,
                                                     block_change_t(t.block_version,
                                                                    false,
                                                                    NULL,
                                                                    t.second)));
            if (!res.second) {
                // The insertion failed.  We need to combine the versions.
                auto const jt = res.first;
                // The versions can't be the same for different write operations.
                rassert(jt->second.version != t.block_version);
                if (jt->second.version < t.block_version) {
                    // RSI: What if jt->second.tstamp > t.second?  Just like above,
                    // with the dirtied_page_t, should we take the max of both
                    // tstamps?  Ugghh.
                    jt->second.tstamp = t.second;
                }
            }
        }
    }

    return changes;
}

void page_cache_t::remove_txn_set_from_graph(page_cache_t *page_cache,
                                             const std::set<page_txn_t *> &txns) {
    for (auto it = txns.begin(); it != txns.end(); ++it) {
        page_txn_t *txn = *it;
        for (auto jt = txn->subseqers_.begin(); jt != txn->subseqers_.end(); ++jt) {
            (*jt)->remove_preceder(txn);
            if (txns.find(*jt) != txns.end()) {
                if ((*jt)->began_waiting_for_flush_ && !(*jt)->spawned_flush_) {
                    page_cache->im_waiting_for_flush(*jt);
                }
            }
        }
        for (auto jt = txn->preceders_.begin(); jt != txn->preceders_.end(); ++jt) {
            (*jt)->remove_subseqer(txn);
        }

        // RSI: Maybe we could remove pages_modified_last_ earlier?  Like when we
        // begin the index write? (but that's the wrong thread) Or earlier?
        for (auto jt = txn->pages_modified_last_.begin();
             jt != txn->pages_modified_last_.end();
             ++jt) {
            current_page_t *current_page = *jt;
            rassert(current_page->last_modifier_ == txn);
            current_page->last_modifier_ = NULL;
        }
        txn->pages_modified_last_.clear();

        // RSI: Is this the right place?  I guess -- since we remove each txn
        // from the graph individually.  Maybe rename this function?
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
    ancillary_info_t(block_id_t _block_id,
                     repli_timestamp_t _tstamp,
                     page_t *_page)
        : block_id(_block_id), tstamp(_tstamp), page(_page) { }
    block_id_t block_id;
    repli_timestamp_t tstamp;
    page_t *page;
};

void page_cache_t::do_flush_txn_set(page_cache_t *page_cache,
                                    const std::set<page_txn_t *> &txns,
                                    fifo_enforcer_write_token_t index_write_token) {
    pagef("do_flush_txn_set (pc=%p, tset=%p) from %s\n", page_cache, &txns,
          debug_strprint(txns).c_str());

    // RSI: We need some way to wait for the preceding txns to have their flushes
    // stay before our txn's.

    // We're going to flush these transactions.  First we need to figure out what the
    // set of changes we're actually doing is, since any transaction may have touched
    // the same blocks.

    std::map<block_id_t, block_change_t> changes
        = page_cache_t::compute_changes(txns);

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
                                                                    it->second.tstamp,  // RSI: Deleted blocks have tstamps?
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
                        // We can't be in the process of loading a block we're going to
                        // write that we don't have a block token.  That's because we
                        // _actually dirtied the page_.  We had to have acquired the buf,
                        // and the only way to get rid of the buf is for it to be
                        // evicted, in which case the block token would be non-empty.
                        rassert(page->destroy_ptr_ == NULL);
                        rassert(page->buf_.has());

                        // RSI: Is there a page_acq-t for this buf we're writing?  Is it
                        // possible that we might be trying to do an unbacked eviction
                        // for this page right now?
                        write_infos.push_back(buf_write_info_t(page->buf_.get(),
                                                               block_size_t::unsafe_make(page->ser_buf_size_),
                                                               it->first));
                        // RSI: Does ancillary_infos actually need it->first again?  Do
                        // we actually use that field?
                        ancillary_infos.push_back(ancillary_info_t(it->first,
                                                                   it->second.tstamp,
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
        pagef("do_flush_txn_set about to thread switch (pc=%p, tset=%p)\n", page_cache, &txns);

        on_thread_t th(page_cache->serializer_->home_thread());

        pagef("do_flush_txn_set switched threads (pc=%p, tset=%p)\n", page_cache, &txns);

        struct : public iocallback_t, public cond_t {
            void on_io_complete() {
                pulse();
            }
        } blocks_releasable_cb;

        std::vector<counted_t<standard_block_token_t> > tokens
            = page_cache->serializer_->block_writes(write_infos,
                                                    page_cache->writes_io_account.get(),
                                                    &blocks_releasable_cb);

        rassert(tokens.size() == write_infos.size());
        rassert(write_infos.size() == ancillary_infos.size());
        for (size_t i = 0; i < write_infos.size(); ++i) {
            blocks_by_tokens.push_back(block_token_tstamp_t(ancillary_infos[i].block_id,
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

        pagef("do_flush_txn_set blocks_releasable_cb.wait() (pc=%p, tset=%p)\n", page_cache, &txns);

        blocks_releasable_cb.wait();

        // Set the page_t's block token field to their new block tokens.  RSI: Can we
        // do this earlier?  Do we have to wait for blocks_releasable_cb?  It doesn't
        // matter that much as long as we have some way to prevent parallel forced
        // eviction from happening, though.
        for (auto it = blocks_by_tokens.begin(); it != blocks_by_tokens.end(); ++it) {
            if (it->block_token.has() && it->page != NULL) {
                // We know page is still a valid pointer because of the page_ptr_t in
                // snapshotted_dirtied_pages_.

                // RSI: This assertion would fail if we try to force-evict the page
                // simultaneously as this write.
                rassert(!it->page->block_token_.has());
                eviction_bag_t *old_bag
                    = page_cache->evicter().correct_eviction_category(it->page);
                it->page->block_token_ = std::move(it->block_token);
                page_cache->evicter().change_eviction_bag(old_bag, it->page);
            }
        }

        pagef("do_flush_txn_set blocks_releasable_cb waited (pc=%p, tset=%p)\n", page_cache, &txns);

        // RSI: Pass in the exiter to index_write, so that
        // subsequent index_write operations don't have to wait for one another to
        // finish.
        fifo_enforcer_sink_t::exit_write_t exiter(page_cache->index_write_sink.get(),
                                                  index_write_token);
        exiter.wait();

        // RSI: This blocks?  Is there any way to set the began_index_write_ fields?
        // RSI: Does the serializer guarantee that index_write operations happen in
        // exactly the order that they're specified in?
        page_cache->serializer_->index_write(write_ops,
                                             page_cache->writes_io_account.get());
        pagef("do_flush_txn_set index write returned (pc=%p, tset=%p)\n", page_cache, &txns);
    }
    pagef("exited scope after do_flush_txn_set index write returned (pc=%p, tset=%p)\n", page_cache, &txns);

    // Flush complete, and we're back on the page cache's thread.

    // RSI: connect_preceder uses flush_complete_cond_ to see whether it should
    // connect.  It should probably use began_index_write_, when that variable
    // exists?? We had some comment about that?  Or it should use something else?

    page_cache_t::remove_txn_set_from_graph(page_cache, txns);
    pagef("remove_txn_set_from_graph returned (pc=%p, tset=%p)\n", page_cache, &txns);
}

bool page_cache_t::exists_flushable_txn_set(page_txn_t *txn,
                                            std::set<page_txn_t *> *flush_set_out) {
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

void page_cache_t::im_waiting_for_flush(page_txn_t *txn) {
    pagef("im_waiting_for_flush (txn=%p)\n", txn);
    rassert(txn->began_waiting_for_flush_);
    rassert(!txn->spawned_flush_);
    // rassert(!txn->began_index_write_);  // RSI: This variable doesn't exist.
    rassert(txn->live_acqs_.empty());

    // We have a new node that's waiting for flush.  Before this node is flushed, we
    // will have started to flush all preceders (recursively) of this node (that are
    // waiting for flush) that this node doesn't itself precede (recursively).  Now
    // we need to ask: Are all this node's preceders (recursively) (that are ready to
    // flush and haven't started flushing yet) ready to flush?  If so, this node must
    // have been the one that pushed them over the line (since they haven't started
    // flushing yet).  So we begin flushing this node and all of its preceders
    // (recursively) in one atomic flush.


    std::set<page_txn_t *> flush_set;
    if (exists_flushable_txn_set(txn, &flush_set)) {
        pagef("built flushable txn set from %s\n",
              debug_strprint(flush_set).c_str());
        for (auto it = flush_set.begin(); it != flush_set.end(); ++it) {
            rassert(!(*it)->spawned_flush_);
            (*it)->spawned_flush_ = true;
        }

        fifo_enforcer_write_token_t token = index_write_source.enter_write();

        coro_t::spawn_later_ordered(std::bind(&page_cache_t::do_flush_txn_set,
                                              this,
                                              flush_set,
                                              token));
    }
}

eviction_bag_t::eviction_bag_t()
    : bag_(&page_t::eviction_index), size_(0) { }

eviction_bag_t::~eviction_bag_t() {
    guarantee(bag_.size() == 0);
    guarantee(size_ == 0, "size was %" PRIu64, size_);
}

void eviction_bag_t::add_without_size(page_t *page) {
    bag_.add(page);
}

void eviction_bag_t::add_size(uint32_t ser_buf_size) {
    size_ += ser_buf_size;
}

void eviction_bag_t::add(page_t *page, uint32_t ser_buf_size) {
    bag_.add(page);
    size_ += ser_buf_size;
}

void eviction_bag_t::remove(page_t *page, uint32_t ser_buf_size) {
    bag_.remove(page);
    uint64_t value = ser_buf_size;
    rassert(value <= size_, "value = %" PRIu64 ", size_ = %" PRIu64,
            value, size_);
    size_ -= value;
}

bool eviction_bag_t::has_page(page_t *page) const {
    return bag_.has_element(page);
}

bool eviction_bag_t::remove_random(page_t **page_out) {
    if (bag_.size() == 0) {
        return false;
    } else {
        page_t *page = bag_.access_random(randsize(bag_.size()));
        remove(page, page->ser_buf_size_);
        *page_out = page;
        return true;
    }
}

evicter_t::evicter_t(uint64_t memory_limit)
    : memory_limit_(memory_limit) { }

evicter_t::~evicter_t() { }


void evicter_t::add_not_yet_loaded(page_t *page) {
    unevictable_.add_without_size(page);
}

void evicter_t::add_now_loaded_size(uint32_t ser_buf_size) {
    unevictable_.add_size(ser_buf_size);
    evict_if_necessary();
}

bool evicter_t::page_is_in_unevictable_bag(page_t *page) const {
    return unevictable_.has_page(page);
}

void evicter_t::add_to_evictable_unbacked(page_t *page) {
    evictable_unbacked_.add(page, page->ser_buf_size_);
    evict_if_necessary();
}

void evicter_t::move_unevictable_to_evictable(page_t *page) {
    rassert(unevictable_.has_page(page));
    unevictable_.remove(page, page->ser_buf_size_);
    eviction_bag_t *new_bag = correct_eviction_category(page);
    rassert(new_bag == &evictable_disk_backed_
            || new_bag == &evictable_unbacked_);
    new_bag->add(page, page->ser_buf_size_);
    evict_if_necessary();
}

void evicter_t::change_eviction_bag(eviction_bag_t *current_bag,
                                    page_t *page) {
    rassert(current_bag->has_page(page));
    current_bag->remove(page, page->ser_buf_size_);
    eviction_bag_t *new_bag = correct_eviction_category(page);
    new_bag->add(page, page->ser_buf_size_);
    evict_if_necessary();
}

eviction_bag_t *evicter_t::correct_eviction_category(page_t *page) {
    if (page->destroy_ptr_ != NULL || !page->waiters_.empty()) {
        return &unevictable_;
    } else if (!page->buf_.has()) {
        return &evicted_;
    } else if (page->block_token_.has()) {
        return &evictable_disk_backed_;
    } else {
        return &evictable_unbacked_;
    }
}

void evicter_t::remove_page(page_t *page) {
    rassert(page->waiters_.empty());
    rassert(page->snapshot_refcount_ == 0);
    eviction_bag_t *bag = correct_eviction_category(page);
    bag->remove(page, page->ser_buf_size_);
    evict_if_necessary();
}

uint64_t evicter_t::in_memory_size() const {
    return unevictable_.size()
        + evictable_disk_backed_.size()
        + evictable_unbacked_.size();
}

void evicter_t::evict_if_necessary() {
    // RSI: Implement eviction of unbacked evictables too.  When flushing, you
    // could use the page_t::eviction_index_ field to identify pages that are
    // currently in the process of being evicted, to avoid reflushing a page
    // currently being written for the purpose of eviction.

    page_t *page;
    while (in_memory_size() > memory_limit_
           && evictable_disk_backed_.remove_random(&page)) {
        evicted_.add(page, page->ser_buf_size_);
        page->evict_self();
    }
}

}  // namespace alt

