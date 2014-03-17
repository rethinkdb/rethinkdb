#include "buffer_cache/alt/page.hpp"

#include "arch/runtime/coroutines.hpp"
#include "buffer_cache/alt/page_cache.hpp"
#include "serializer/serializer.hpp"

namespace alt {

class page_loader_t {
public:
    page_loader_t()
        : abandon_page_(false) { }
    virtual ~page_loader_t() { }

    virtual void added_waiter(page_cache_t *, cache_account_t *) {
        // Do nothing, in the default case.
    }

    bool abandon_page() const { return abandon_page_; }

    void mark_abandon_page() {
        rassert(!abandon_page_);
        abandon_page_ = true;
    }

private:
    bool abandon_page_;
    DISABLE_COPYING(page_loader_t);
};

// We pick a weird that forces the logic and performance to not spaz out if the
// access time counter overflows.  Performance degradation is "smooth" if
// access_time_counter_ loops around past INITIAL_ACCESS_TIME -- which shouldn't be a
// problem for now, as long as we increment it one value at a time.
static const uint64_t READ_AHEAD_ACCESS_TIME = evicter_t::INITIAL_ACCESS_TIME - 1;


page_t::page_t(block_id_t block_id, page_cache_t *page_cache)
    : loader_(NULL),
      max_ser_block_size_(page_cache->max_block_size().ser_value()),
      ser_buf_size_(0),
      access_time_(page_cache->evicter().next_access_time()),
      snapshot_refcount_(0) {
    page_cache->evicter().add_deferred_loaded(this);

    coro_t::spawn_now_dangerously(std::bind(&page_t::deferred_load_with_block_id,
                                            this,
                                            block_id,
                                            page_cache));
}

page_t::page_t(block_id_t block_id, page_cache_t *page_cache,
               cache_account_t *account)
    : loader_(NULL),
      max_ser_block_size_(page_cache->max_block_size().ser_value()),
      ser_buf_size_(0),
      access_time_(page_cache->evicter().next_access_time()),
      snapshot_refcount_(0) {
    page_cache->evicter().add_not_yet_loaded(this);

    coro_t::spawn_now_dangerously(std::bind(&page_t::load_with_block_id,
                                            this,
                                            block_id,
                                            page_cache,
                                            account));
}

page_t::page_t(block_size_t block_size, scoped_malloc_t<ser_buffer_t> buf,
               page_cache_t *page_cache)
    : loader_(NULL),
      max_ser_block_size_(page_cache->max_block_size().ser_value()),
      ser_buf_size_(block_size.ser_value()),
      buf_(std::move(buf)),
      access_time_(page_cache->evicter().next_access_time()),
      snapshot_refcount_(0) {
    rassert(buf_.has());
    page_cache->evicter().add_to_evictable_unbacked(this);
}

page_t::page_t(scoped_malloc_t<ser_buffer_t> buf,
               const counted_t<standard_block_token_t> &block_token,
               page_cache_t *page_cache)
    : loader_(NULL),
      max_ser_block_size_(page_cache->max_block_size().ser_value()),
      ser_buf_size_(block_token->block_size().ser_value()),
      buf_(std::move(buf)),
      block_token_(block_token),
      access_time_(READ_AHEAD_ACCESS_TIME),
      snapshot_refcount_(0) {
    rassert(buf_.has());
    page_cache->evicter().add_to_evictable_disk_backed(this);
}

page_t::page_t(page_t *copyee, page_cache_t *page_cache, cache_account_t *account)
    : loader_(NULL),
      max_ser_block_size_(page_cache->max_block_size().ser_value()),
      ser_buf_size_(0),
      access_time_(page_cache->evicter().next_access_time()),
      snapshot_refcount_(0) {
    page_cache->evicter().add_not_yet_loaded(this);
    coro_t::spawn_now_dangerously(std::bind(&page_t::load_from_copyee,
                                            this,
                                            copyee,
                                            page_cache,
                                            account));
}

page_t::~page_t() {
    rassert(waiters_.empty());
    rassert(snapshot_refcount_ == 0);
    if (loader_ != NULL) {
        loader_->mark_abandon_page();
    }
}

void page_t::load_from_copyee(page_t *page, page_t *copyee,
                              page_cache_t *page_cache,
                              cache_account_t *account) {
    // This is called using spawn_now_dangerously.  We need to atomically set
    // loader_ and do some other things.
    page_loader_t loader;
    rassert(page->loader_ == NULL);
    page->loader_ = &loader;

    auto_drainer_t::lock_t lock(page_cache->drainer_.get());
    page_ptr_t copyee_ptr(copyee, page_cache);

    // Okay, it's safe to block.
    {
        page_acq_t acq;
        acq.init(copyee, page_cache, account);
        acq.buf_ready_signal()->wait();

        ASSERT_FINITE_CORO_WAITING;
        if (!loader.abandon_page()) {
            // RSP: If somehow there are no snapshotters of copyee now (besides
            // ourself), maybe we could avoid copying this memory.  We need to
            // carefully track snapshotters anyway, once we're comfortable with that,
            // we could do it.

            uint32_t ser_buf_size = copyee->ser_buf_size_;
            rassert(copyee->buf_.has());
            scoped_malloc_t<ser_buffer_t> buf
                = serializer_t::allocate_buffer(page_cache->max_block_size());

            memcpy(buf.get(), copyee->buf_.get(), ser_buf_size);

            page->ser_buf_size_ = ser_buf_size;
            page->buf_ = std::move(buf);
            page->loader_ = NULL;

            page->pulse_waiters_or_make_evictable(page_cache);
        }
    }
}

void page_t::finish_load_with_block_id(page_t *page, page_cache_t *page_cache,
                                       counted_t<standard_block_token_t> block_token,
                                       scoped_malloc_t<ser_buffer_t> buf) {
    rassert(!page->block_token_.has());
    rassert(!page->buf_.has());
    rassert(block_token.has());
    page->ser_buf_size_ = block_token->block_size().ser_value();
    page->buf_ = std::move(buf);
    page->block_token_ = std::move(block_token);
    page->loader_ = NULL;

    page->pulse_waiters_or_make_evictable(page_cache);
}

// This lives on the serializer thread.
class deferred_block_token_t {
public:
    counted_t<standard_block_token_t> token;
};

class deferred_page_loader_t : public page_loader_t {
public:
    explicit deferred_page_loader_t(page_t *page)
        : page_(page), block_token_ptr_(new deferred_block_token_t) { }

    deferred_block_token_t *block_token_ptr() {
        return block_token_ptr_.get();
    }

    scoped_ptr_t<deferred_block_token_t> extract_block_token_ptr() {
        return std::move(block_token_ptr_);
    }

    void added_waiter(page_cache_t *page_cache, cache_account_t *account) {
        coro_t::spawn_now_dangerously(std::bind(&page_t::catch_up_with_deferred_load,
                                                this,
                                                page_cache,
                                                account));
    }

    page_t *page() { return page_; }


private:
    page_t *page_;

    // We put the block token on the heap, so that it can survive past the
    // deferred_load_with_block_id coroutine (so that catch_up_with_deferred_load can
    // take ownership of it).
    scoped_ptr_t<deferred_block_token_t> block_token_ptr_;

    DISABLE_COPYING(deferred_page_loader_t);
};

void page_t::catch_up_with_deferred_load(
        deferred_page_loader_t *deferred_loader,
        page_cache_t *page_cache,
        cache_account_t *account) {
    page_t *page = deferred_loader->page();
    page_cache->evicter().catch_up_deferred_load(page);

    // This is called using spawn_now_dangerously.  The deferred_load_with_block_id
    // operation associated with `loader` is on the serializer thread, or being sent
    // to the serializer thread, or returning back from the serializer thread, right
    // now.
    rassert(page->loader_ == deferred_loader);
    page_loader_t our_loader;
    page->loader_ = &our_loader;

    // Before blocking, take ownership of the block_token_ptr.
    scoped_ptr_t<deferred_block_token_t> block_token_ptr
        = deferred_loader->extract_block_token_ptr();

    // Before blocking, tell the deferred loader to abandon the page.  It's before
    // the deferred loader has gotten back to this thread, so we can do that.
    deferred_loader->abandon_page();

    scoped_malloc_t<ser_buffer_t> buf;
    {
        serializer_t *const serializer = page_cache->serializer_;
        buf = serializer_t::allocate_buffer(page_cache->max_block_size());

        // We use the fact that on_thread_t preserves order with the on_thread_t in
        // deferred_load_with_block_id.  This means that it's already run its section
        // on the serializer thread, and has initialized block_token_ptr->token.
        on_thread_t th(serializer->home_thread());
        // Now finish what the rest of load_with_block_id would do.
        rassert(block_token_ptr->token.has());
        serializer->block_read(block_token_ptr->token,
                               buf.get(),
                               account->get());
    }

    ASSERT_FINITE_CORO_WAITING;
    if (our_loader.abandon_page()) {
        return;
    }

    page_t::finish_load_with_block_id(page, page_cache,
                                      std::move(block_token_ptr->token),
                                      std::move(buf));
}


void page_t::deferred_load_with_block_id(page_t *page, block_id_t block_id,
                                         page_cache_t *page_cache) {
    // This is called using spawn_now_dangerously.  We need to set
    // loader_ before blocking the coroutine.
    deferred_page_loader_t loader(page);
    rassert(page->loader_ == NULL);
    page->loader_ = &loader;

    // Before blocking, before any catch_up_with_deferred_load can steal it, get the
    // deferred_block_token_t pointer.
    deferred_block_token_t *on_heap_token = loader.block_token_ptr();

    auto_drainer_t::lock_t lock(page_cache->drainer_.get());

    {
        serializer_t *const serializer = page_cache->serializer_;
        on_thread_t th(serializer->home_thread());
        // The index_read should not block.  It must not block, because we're
        // depending on order preservation across the on_thread_t.
        ASSERT_FINITE_CORO_WAITING;
        on_heap_token->token = serializer->index_read(block_id);
        rassert(on_heap_token->token.has());
    }

    ASSERT_FINITE_CORO_WAITING;
    if (loader.abandon_page()) {
        return;
    }

    rassert(!page->block_token_.has());
    rassert(!page->buf_.has());
    rassert(loader.block_token_ptr() == on_heap_token);
    rassert(on_heap_token->token.has());
    page->ser_buf_size_ = on_heap_token->token->block_size().ser_value();
    page->block_token_ = std::move(on_heap_token->token);
    page->loader_ = NULL;

    rassert(page->waiters_.empty());

#ifndef NDEBUG
    evicter_t *const evicter = &page_cache->evicter();
    rassert(evicter->page_is_in_evicted_bag(page));
    rassert(evicter->evicted_category()
            == evicter->correct_eviction_category(page));
#endif
}

void page_t::load_with_block_id(page_t *page, block_id_t block_id,
                                page_cache_t *page_cache,
                                cache_account_t *account) {
    // This is called using spawn_now_dangerously.  We need to set
    // loader_ before blocking the coroutine.
    page_loader_t loader;
    rassert(page->loader_ == NULL);
    page->loader_ = &loader;

    auto_drainer_t::lock_t lock(page_cache->drainer_.get());

    scoped_malloc_t<ser_buffer_t> buf;
    counted_t<standard_block_token_t> block_token;

    {
        serializer_t *const serializer = page_cache->serializer_;
        buf = serializer_t::allocate_buffer(page_cache->max_block_size());

        on_thread_t th(serializer->home_thread());
        block_token = serializer->index_read(block_id);
        rassert(block_token.has());
        serializer->block_read(block_token,
                               buf.get(),
                               account->get());
    }

    ASSERT_FINITE_CORO_WAITING;
    if (loader.abandon_page()) {
        return;
    }

    page_t::finish_load_with_block_id(page, page_cache,
                                      std::move(block_token),
                                      std::move(buf));
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

page_t *page_t::make_copy(page_cache_t *page_cache, cache_account_t *account) {
    page_t *ret = new page_t(this, page_cache, account);
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

void page_t::add_waiter(page_acq_t *acq, cache_account_t *account) {
    eviction_bag_t *old_bag
        = acq->page_cache()->evicter().correct_eviction_category(this);
    waiters_.push_back(acq);
    acq->page_cache()->evicter().change_to_correct_eviction_bag(old_bag, this);
    if (buf_.has()) {
        acq->buf_ready_signal_.pulse();
    } else if (loader_ != NULL) {
        loader_->added_waiter(acq->page_cache(), account);
    } else if (block_token_.has()) {
        coro_t::spawn_now_dangerously(std::bind(&page_t::load_using_block_token,
                                                this,
                                                acq->page_cache(),
                                                account));
    } else {
        crash("An unloaded block is not in a loadable state.");
    }
}

// Unevicts page.
void page_t::load_using_block_token(page_t *page, page_cache_t *page_cache,
                                    cache_account_t *account) {
    // This is called using spawn_now_dangerously.  We need to set
    // loader_ before blocking the coroutine.
    page_loader_t loader;
    rassert(page->loader_ == NULL);
    page->loader_ = &loader;

    page_cache->evicter().reloading_page(page);

    auto_drainer_t::lock_t lock(page_cache->drainer_.get());

    counted_t<standard_block_token_t> block_token = page->block_token_;
    rassert(block_token.has());

    scoped_malloc_t<ser_buffer_t> buf;
    {
        serializer_t *const serializer = page_cache->serializer_;
        buf = serializer_t::allocate_buffer(page_cache->max_block_size());

        on_thread_t th(serializer->home_thread());
        serializer->block_read(block_token,
                               buf.get(),
                               account->get());
    }

    ASSERT_FINITE_CORO_WAITING;
    if (loader.abandon_page()) {
        return;
    }

    rassert(page->block_token_.get() == block_token.get());
    rassert(!page->buf_.has());
    rassert(page->ser_buf_size_ == block_token->block_size().ser_value());
    block_token.reset();
    page->buf_ = std::move(buf);
    page->loader_ = NULL;

    page->pulse_waiters_or_make_evictable(page_cache);
}

void page_t::set_page_buf_size(block_size_t block_size) {
    rassert(buf_.has(),
            "Called outside page_acq_t or without waiting for the buf_ready_signal_?");
    rassert(ser_buf_size_ != 0);
    rassert(!block_token_.has(),
            "Modified a page_t without resetting the block token.");
    ser_buf_size_ = block_size.ser_value();
}

block_size_t page_t::get_page_buf_size() {
    rassert(buf_.has());
    rassert(ser_buf_size_ != 0);
    return block_size_t::unsafe_make(ser_buf_size_);
}

uint32_t page_t::hypothetical_memory_usage() const {
    return max_ser_block_size_;
}

void *page_t::get_page_buf(page_cache_t *page_cache) {
    rassert(buf_.has());
    access_time_ = page_cache->evicter().next_access_time();
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
    acq->page_cache()->evicter().change_to_correct_eviction_bag(old_bag, this);

    // page_acq_t always has a lesser lifetime than some page_ptr_t.
    rassert(snapshot_refcount_ > 0);
}

void page_t::evict_self() {
    // A page_t can only self-evict if it has a block token (for now).
    rassert(waiters_.empty());
    rassert(block_token_.has());
    rassert(buf_.has());
    buf_.reset();
}


page_acq_t::page_acq_t() : page_(NULL), page_cache_(NULL) {
}

void page_acq_t::init(page_t *page, page_cache_t *page_cache,
                      cache_account_t *account) {
    rassert(page_ == NULL);
    rassert(page_cache_ == NULL);
    rassert(!buf_ready_signal_.is_pulsed());
    page_ = page;
    page_cache_ = page_cache;
    page_->add_waiter(this, account);
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

block_size_t page_acq_t::get_buf_size() {
    buf_ready_signal_.wait();
    return page_->get_page_buf_size();
}

void *page_acq_t::get_buf_write(block_size_t block_size) {
    buf_ready_signal_.wait();
    page_->reset_block_token();
    page_->set_page_buf_size(block_size);
    return page_->get_page_buf(page_cache_);
}

const void *page_acq_t::get_buf_read() {
    buf_ready_signal_.wait();
    return page_->get_page_buf(page_cache_);
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

page_t *page_ptr_t::get_page_for_write(page_cache_t *page_cache,
                                       cache_account_t *account) {
    rassert(page_ != NULL);
    if (page_->num_snapshot_references() > 1) {
        page_ptr_t tmp(page_->make_copy(page_cache, account), page_cache);
        *this = std::move(tmp);
    }
    return page_;
}

timestamped_page_ptr_t::timestamped_page_ptr_t()
    : timestamp_(repli_timestamp_t::invalid) { }

timestamped_page_ptr_t::timestamped_page_ptr_t(timestamped_page_ptr_t &&movee)
    : timestamp_(movee.timestamp_), page_ptr_(std::move(movee.page_ptr_)) {
    movee.timestamp_ = repli_timestamp_t::invalid;
    movee.page_ptr_.reset();
}

timestamped_page_ptr_t &timestamped_page_ptr_t::operator=(timestamped_page_ptr_t &&movee) {
    timestamped_page_ptr_t tmp(std::move(movee));
    std::swap(timestamp_, tmp.timestamp_);
    std::swap(page_ptr_, tmp.page_ptr_);
    return *this;
}

timestamped_page_ptr_t::~timestamped_page_ptr_t() { }

bool timestamped_page_ptr_t::has() const {
    return page_ptr_.has();
}

void timestamped_page_ptr_t::init(repli_timestamp_t timestamp,
                                  page_t *page,
                                  page_cache_t *page_cache) {
    rassert(timestamp_ == repli_timestamp_t::invalid);
    rassert(page == NULL || timestamp != repli_timestamp_t::invalid);
    timestamp_ = timestamp;
    page_ptr_.init(page, page_cache);
}

page_t *timestamped_page_ptr_t::get_page_for_read() const {
    return page_ptr_.get_page_for_read();
}


}  // namespace alt
