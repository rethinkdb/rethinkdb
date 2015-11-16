#include "buffer_cache/page.hpp"

#include "arch/runtime/coroutines.hpp"
#include "buffer_cache/page_cache.hpp"
#include "serializer/serializer.hpp"

namespace alt {

class page_loader_t {
public:
    page_loader_t()
        : abandon_page_(false) { }
    virtual ~page_loader_t() { }

    virtual void added_waiter(page_cache_t *, cache_account_t *) = 0;

    virtual bool is_really_loading() const = 0;

    MUST_USE bool abandon_page() const { return abandon_page_; }

    void mark_abandon_page() {
        rassert(!abandon_page_);
        abandon_page_ = true;
    }

private:
    bool abandon_page_;
    DISABLE_COPYING(page_loader_t);
};

class instant_page_loader_t final : public page_loader_t {
public:
    instant_page_loader_t() { }
    ~instant_page_loader_t() { }

    void added_waiter(page_cache_t *, cache_account_t *) final {
        // Do nothing.
    }

    bool is_really_loading() const final {
        return true;
    }

private:
    DISABLE_COPYING(instant_page_loader_t);
};

// We pick a weird that forces the logic and performance to not spaz out if the
// access time counter overflows.  Performance degradation is "smooth" if
// access_time_counter_ loops around past INITIAL_ACCESS_TIME -- which shouldn't be a
// problem for now, as long as we increment it one value at a time.
static const uint64_t READ_AHEAD_ACCESS_TIME = evicter_t::INITIAL_ACCESS_TIME - 1;


page_t::page_t(block_id_t block_id, page_cache_t *page_cache)
    : block_id_(block_id),
      loader_(NULL),
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
    : block_id_(block_id),
      loader_(NULL),
      access_time_(page_cache->evicter().next_access_time()),
      snapshot_refcount_(0) {
    page_cache->evicter().add_not_yet_loaded(this);

    coro_t::spawn_now_dangerously(std::bind(&page_t::load_with_block_id,
                                            this,
                                            block_id,
                                            page_cache,
                                            account));
}

page_t::page_t(block_id_t block_id, buf_ptr_t buf,
               page_cache_t *page_cache)
    : block_id_(block_id),
      loader_(NULL),
      buf_(std::move(buf)),
      access_time_(page_cache->evicter().next_access_time()),
      snapshot_refcount_(0) {
    rassert(buf_.has());
    page_cache->evicter().add_to_evictable_unbacked(this);
}

page_t::page_t(block_id_t block_id,
               buf_ptr_t buf,
               const counted_t<standard_block_token_t> &block_token,
               page_cache_t *page_cache)
    : block_id_(block_id),
      loader_(NULL),
      buf_(std::move(buf)),
      block_token_(block_token),
      access_time_(READ_AHEAD_ACCESS_TIME),
      snapshot_refcount_(0) {
    rassert(buf_.has());
    page_cache->evicter().add_to_evictable_disk_backed(this);
}

page_t::page_t(page_t *copyee, page_cache_t *page_cache, cache_account_t *account)
    : block_id_(copyee->block_id_),
      loader_(NULL),
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

bool page_t::loader_is_loading(page_loader_t *loader) {
    rassert(loader != NULL);
    return loader->is_really_loading();
}

void page_t::load_from_copyee(page_t *page, page_t *copyee,
                              page_cache_t *page_cache,
                              cache_account_t *account) {
    // This is called using spawn_now_dangerously.  We need to atomically set
    // loader_ and do some other things.
    instant_page_loader_t loader;
    rassert(page->loader_ == NULL);
    page->loader_ = &loader;

    auto_drainer_t::lock_t lock = page_cache->drainer_lock();
    page_ptr_t copyee_ptr(copyee);

    // Okay, it's safe to block.
    {
        page_acq_t acq;
        acq.init(copyee, page_cache, account);
        acq.buf_ready_signal()->wait();

        ASSERT_FINITE_CORO_WAITING;
        if (!loader.abandon_page()) {
            // KSI: If somehow there are no snapshotters of copyee now (besides
            // ourself), maybe we could avoid copying this memory.  We need to
            // carefully track snapshotters anyway, once we're comfortable with that,
            // we could do it.

            {
                usage_adjuster_t adjuster(page_cache, page);
                page->buf_ = buf_ptr_t::alloc_copy(copyee->buf_);
                page->loader_ = NULL;
            }

            page->pulse_waiters_or_make_evictable(page_cache);
        }
    }

    copyee_ptr.reset_page_ptr(page_cache);
    // Don't bother calling consider_evicting_current_page -- we just acquired the
    // page for the sake of a live current_page_t, so it wouldn't do anything if it
    // did work correctly.  (It's also possible that this function didn't block, that
    // we're still inside the page_t constructor, and its page_ptr_t has not been
    // initialized.  It would turn out that we're safe, because there must be some
    // page_acq_t calling get_buf_write, not to mention a current_page_acq_t.  We
    // choose to be performance-fragile rather than correctness-fragile.
}

void page_t::finish_load_with_block_id(page_t *page, page_cache_t *page_cache,
                                       counted_t<standard_block_token_t> block_token,
                                       buf_ptr_t buf) {
    rassert(!page->block_token_.has());
    rassert(!page->buf_.has());
    rassert(block_token.has());
    {
        usage_adjuster_t adjuster(page_cache, page);
        page->buf_ = std::move(buf);
        page->block_token_ = std::move(block_token);
        page->loader_ = NULL;
    }

    page->pulse_waiters_or_make_evictable(page_cache);
}

// This lives on the serializer thread.
class deferred_block_token_t {
public:
    counted_t<standard_block_token_t> token;
};

class deferred_page_loader_t final : public page_loader_t {
public:
    explicit deferred_page_loader_t(page_t *page)
        : page_(page), block_token_ptr_(new deferred_block_token_t) { }

    deferred_block_token_t *block_token_ptr() {
        return block_token_ptr_.get();
    }

    scoped_ptr_t<deferred_block_token_t> extract_block_token_ptr() {
        return std::move(block_token_ptr_);
    }

    void added_waiter(page_cache_t *page_cache, cache_account_t *account) final {
        coro_t::spawn_now_dangerously(std::bind(&page_t::catch_up_with_deferred_load,
                                                this,
                                                page_cache,
                                                account));
    }

    bool is_really_loading() const final {
        return false;
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

    // This is called using spawn_now_dangerously.  The deferred_load_with_block_id
    // operation associated with `loader` is on the serializer thread, or being sent
    // to the serializer thread, or returning back from the serializer thread, right
    // now.
    rassert(page->loader_ == deferred_loader);
    instant_page_loader_t our_loader;
    page->loader_ = &our_loader;

    // Before blocking, take ownership of the block_token_ptr.
    scoped_ptr_t<deferred_block_token_t> block_token_ptr
        = deferred_loader->extract_block_token_ptr();

    // Before blocking, tell the deferred loader to abandon the page.  It's before
    // the deferred loader has gotten back to this thread, so we can do that.
    deferred_loader->mark_abandon_page();

    // Before blocking, tell the evicter to put us in the right category.
    page_cache->evicter().catch_up_deferred_load(page);

    buf_ptr_t buf;
    {
        serializer_t *const serializer = page_cache->serializer();

        // We use the fact that on_thread_t preserves order with the on_thread_t in
        // deferred_load_with_block_id.  This means that it's already run its section
        // on the serializer thread, and has initialized block_token_ptr->token.
        on_thread_t th(serializer->home_thread());
        // Now finish what the rest of load_with_block_id would do.
        rassert(block_token_ptr->token.has());
        buf = serializer->block_read(block_token_ptr->token,
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

    auto_drainer_t::lock_t lock = page_cache->drainer_lock();

    {
        serializer_t *const serializer = page_cache->serializer();
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
    {
        usage_adjuster_t adjuster(page_cache, page);
        page->block_token_ = std::move(on_heap_token->token);
        page->loader_ = NULL;
    }

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
    instant_page_loader_t loader;
    rassert(page->loader_ == NULL);
    page->loader_ = &loader;

    auto_drainer_t::lock_t lock = page_cache->drainer_lock();

    buf_ptr_t buf;
    counted_t<standard_block_token_t> block_token;

    {
        serializer_t *const serializer = page_cache->serializer();
        on_thread_t th(serializer->home_thread());
        block_token = serializer->index_read(block_id);
        rassert(block_token.has());
        buf = serializer->block_read(block_token,
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
    waiters_.push_front(acq);
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
    instant_page_loader_t loader;
    rassert(page->loader_ == NULL);
    page->loader_ = &loader;

    page_cache->evicter().reloading_page(page);

    auto_drainer_t::lock_t lock = page_cache->drainer_lock();

    counted_t<standard_block_token_t> block_token = page->block_token_;
    rassert(block_token.has());

    buf_ptr_t buf;
    {
        serializer_t *const serializer = page_cache->serializer();

        on_thread_t th(serializer->home_thread());
        buf = serializer->block_read(block_token,
                                     account->get());
    }

    ASSERT_FINITE_CORO_WAITING;
    if (loader.abandon_page()) {
        return;
    }

    rassert(page->block_token_.get() == block_token.get());
    rassert(!page->buf_.has());
    block_token.reset();
    {
        usage_adjuster_t adjuster(page_cache, page);
        page->buf_ = std::move(buf);
        page->loader_ = NULL;
    }

    page->pulse_waiters_or_make_evictable(page_cache);
}

void page_t::set_page_buf_size(block_size_t block_size, page_cache_t *page_cache) {
    rassert(buf_.has(),
            "Called outside page_acq_t or without waiting for the buf_ready_signal_?");
    rassert(!block_token_.has(),
            "Modified a page_t without resetting the block token.");
    {
        usage_adjuster_t adjuster(page_cache, this);
        buf_.resize_fill_zero(block_size);
    }
}

block_size_t page_t::get_page_buf_size() {
    rassert(buf_.has());
    return buf_.block_size();
}

uint32_t page_t::hypothetical_memory_usage(page_cache_t *page_cache) const {
    // Note that the `base_size` we're assigning is overly conservative.
    // Not every `page_t` corresponds to one `current_page_t`. and not every
    // `page_t` has a block token. However in typical workloads, most of them will.
    //
    // It is tempting to account for the `standard_block_token_t` only if
    // `block_token_.has()` is `true`. However there are some places in the code
    // that assume that `hypothetical_memory_usage` doesn't change after setting
    // the block token.
    size_t base_size = sizeof(current_page_t) + sizeof(page_t) +
        sizeof(standard_block_token_t);
    if (buf_.has()) {
        return base_size + buf_.aligned_block_size();
    } else if (block_token_.has()) {
        return base_size +
            buf_ptr_t::compute_aligned_block_size(block_token_->block_size());
    } else {
        // If the block isn't loaded and we don't know, we respond conservatively,
        // to stay on the proper side of the memory limit.
        return base_size + page_cache->max_block_size().ser_value();
    }
}

void *page_t::get_page_buf(page_cache_t *page_cache) {
    rassert(buf_.has());
    access_time_ = page_cache->evicter().next_access_time();
    return buf_.cache_data();
}

void page_t::reset_block_token(DEBUG_VAR page_cache_t *page_cache) {
    // The page is supposed to have its buffer acquired in reset_block_token -- it's
    // the thing modifying the page.  We thus assume that the page is unevictable and
    // resetting block_token_ doesn't change that.
    rassert(!waiters_.empty());
    rassert(buf_.has());
    if (block_token_.has()) {
        rassert(buf_.block_size().value() == block_token_->block_size().value());
#ifndef NDEBUG
        const uint32_t usage_before = hypothetical_memory_usage(page_cache);
#endif
        block_token_.reset();
        // Hypothetical memory usage shouldn't have changed -- because the buf is
        // already loaded in memory.
        rassert(usage_before == hypothetical_memory_usage(page_cache));
    }
}


void page_t::remove_waiter(page_acq_t *acq) {
    eviction_bag_t *old_bag
        = acq->page_cache()->evicter().correct_eviction_category(this);
    waiters_.remove(acq);
    acq->page_cache()->evicter().change_to_correct_eviction_bag(old_bag, this);

    // page_acq_t always has a lesser lifetime than some page_ptr_t.
    rassert(snapshot_refcount_ > 0);
}

void page_t::evict_self(DEBUG_VAR page_cache_t *page_cache) {
    // A page_t can only self-evict if it has a block token (for now).
    rassert(waiters_.empty());
    rassert(block_token_.has());
    rassert(buf_.has());
    rassert(block_token_->block_size().value() == buf_.block_size().value());
#ifndef NDEBUG
    const uint32_t usage_before = hypothetical_memory_usage(page_cache);
#endif
    buf_.reset();
    // Hypothetical memory usage shouldn't have changed -- the block token has the
    // same block size.
    rassert(usage_before == hypothetical_memory_usage(page_cache));
}

ser_buffer_t *page_t::get_loaded_ser_buffer() {
    rassert(buf_.has());
    return buf_.ser_buffer();
}

// Used for after we've flushed the page.
void page_t::init_block_token(counted_t<standard_block_token_t> token,
                              DEBUG_VAR page_cache_t *page_cache) {
    rassert(buf_.has());
    rassert(buf_.block_size().value() == token->block_size().value());
    rassert(!block_token_.has());
#ifndef NDEBUG
    const uint32_t usage_before = hypothetical_memory_usage(page_cache);
#endif
    block_token_ = std::move(token);
    // Hypothetical memory usage shouldn't have changed -- the block token has the
    // same block size.
    rassert(usage_before == hypothetical_memory_usage(page_cache));
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

        // There's no need to call consider_evicting_current_page, because page_acq_t
        // always has a lesser lifetime than some current_page_acq_t or some other
        // page_ptr_t.  It would be risky to call it -- page_acq_t is used in
        // load_from_copyee.
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
    page_->reset_block_token(page_cache_);
    page_->set_page_buf_size(block_size, page_cache_);
    return page_->get_page_buf(page_cache_);
}

const void *page_acq_t::get_buf_read() {
    buf_ready_signal_.wait();
    return page_->get_page_buf(page_cache_);
}

page_ptr_t::page_ptr_t() : page_(NULL) {
}

page_ptr_t::~page_ptr_t() {
    rassert(page_ == NULL);
}

page_ptr_t::page_ptr_t(page_ptr_t &&movee)
    : page_(movee.page_) {
    movee.page_ = NULL;
}

page_ptr_t &page_ptr_t::operator=(page_ptr_t &&movee) {
    // We can't do true assignment, destructing an old page-having value, because
    // reset() has to manually be called.  (This assertion is redundant with the one
    // that'll enforce this fact in tmp's destructor.)
    rassert(page_ == NULL);

    page_ptr_t tmp(std::move(movee));
    swap_with(&tmp);
    return *this;
}

void page_ptr_t::init(page_t *page) {
    rassert(page_ == NULL);
    page_ = page;
    if (page_ != NULL) {
        page_->add_snapshotter();
    }
}

void page_ptr_t::reset_page_ptr(page_cache_t *page_cache) {
    if (page_ != NULL) {
        page_t *ptr = page_;
        page_ = NULL;
        ptr->remove_snapshotter(page_cache);
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
        page_ptr_t tmp(page_->make_copy(page_cache, account));
        swap_with(&tmp);
        tmp.reset_page_ptr(page_cache);
        // We don't call consider_evicting_current_page here -- it wouldn't do
        // anything anyway because `this` is the page_ptr_t for the
        // current_page_acq_t, and we know it's not empty.  Also we must have a
        // page_acq_t right now.  We err on the side of being performance-fragile
        // instead of correctness-fragile.
    }
    return page_;
}

void page_ptr_t::swap_with(page_ptr_t *other) {
    std::swap(page_, other->page_);
}

timestamped_page_ptr_t::timestamped_page_ptr_t()
    : timestamp_(repli_timestamp_t::invalid) { }

timestamped_page_ptr_t::timestamped_page_ptr_t(timestamped_page_ptr_t &&movee)
    : timestamp_(movee.timestamp_), page_ptr_(std::move(movee.page_ptr_)) {
    movee.timestamp_ = repli_timestamp_t::invalid;
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
                                  page_t *page) {
    rassert(timestamp_ == repli_timestamp_t::invalid);
    rassert(page == NULL ||
            timestamp != repli_timestamp_t::invalid ||
            is_aux_block_id(page->block_id()));
    timestamp_ = timestamp;
    page_ptr_.init(page);
}

page_t *timestamped_page_ptr_t::get_page_for_read() const {
    return page_ptr_.get_page_for_read();
}

void timestamped_page_ptr_t::reset_page_ptr(page_cache_t *page_cache) {
    ASSERT_NO_CORO_WAITING;
    page_ptr_.reset_page_ptr(page_cache);
}

}  // namespace alt
