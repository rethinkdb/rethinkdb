#include "buffer_cache/alt/evicter.hpp"

#include "buffer_cache/alt/page.hpp"
#include "buffer_cache/alt/page_cache.hpp"
#include "buffer_cache/alt/cache_balancer.hpp"

namespace alt {

evicter_t::evicter_t(cache_balancer_t *balancer)
    : balancer_(balancer),
      bytes_loaded_counter_(0),
      access_time_counter_(INITIAL_ACCESS_TIME)
{
    guarantee(balancer_ != NULL);
    memory_limit_ = balancer_->base_mem_per_store();
    balancer_->add_evicter(this);
}

evicter_t::~evicter_t() {
    assert_thread();
    balancer_->remove_evicter(this);
}

void evicter_t::update_memory_limit(uint64_t new_memory_limit,
                                    uint64_t bytes_loaded_accounted_for) {
    assert_thread();
    __sync_sub_and_fetch(&bytes_loaded_counter_, bytes_loaded_accounted_for);
    memory_limit_ = new_memory_limit;
    evict_if_necessary();
}

uint64_t evicter_t::get_clamped_bytes_loaded() const {
    __sync_synchronize();
    int64_t res = bytes_loaded_counter_;
    __sync_synchronize();
    return std::max<int64_t>(res, 0);
}

void evicter_t::notify_bytes_loading(int64_t in_memory_buf_change) {
    assert_thread();
    __sync_add_and_fetch(&bytes_loaded_counter_, in_memory_buf_change);
    balancer_->notify_access();
}

void evicter_t::add_deferred_loaded(page_t *page) {
    assert_thread();
    evicted_.add(page, page->hypothetical_memory_usage());
}

void evicter_t::catch_up_deferred_load(page_t *page) {
    assert_thread();
    rassert(evicted_.has_page(page));
    evicted_.remove(page, page->hypothetical_memory_usage());
    unevictable_.add(page, page->hypothetical_memory_usage());
    evict_if_necessary();
    notify_bytes_loading(page->hypothetical_memory_usage());
}

void evicter_t::add_not_yet_loaded(page_t *page) {
    assert_thread();
    unevictable_.add(page, page->hypothetical_memory_usage());
    evict_if_necessary();
    notify_bytes_loading(page->hypothetical_memory_usage());
}

void evicter_t::reloading_page(page_t *page) {
    assert_thread();
    notify_bytes_loading(page->hypothetical_memory_usage());
}

bool evicter_t::page_is_in_unevictable_bag(page_t *page) const {
    assert_thread();
    return unevictable_.has_page(page);
}

bool evicter_t::page_is_in_evicted_bag(page_t *page) const {
    assert_thread();
    return evicted_.has_page(page);
}

void evicter_t::add_to_evictable_unbacked(page_t *page) {
    assert_thread();
    evictable_unbacked_.add(page, page->hypothetical_memory_usage());
    evict_if_necessary();
    notify_bytes_loading(page->hypothetical_memory_usage());
}

void evicter_t::add_to_evictable_disk_backed(page_t *page) {
    assert_thread();
    evictable_disk_backed_.add(page, page->hypothetical_memory_usage());
    evict_if_necessary();
    notify_bytes_loading(page->hypothetical_memory_usage());
}

void evicter_t::move_unevictable_to_evictable(page_t *page) {
    assert_thread();
    rassert(unevictable_.has_page(page));
    unevictable_.remove(page, page->hypothetical_memory_usage());
    eviction_bag_t *new_bag = correct_eviction_category(page);
    rassert(new_bag == &evictable_disk_backed_
            || new_bag == &evictable_unbacked_);
    new_bag->add(page, page->hypothetical_memory_usage());
    evict_if_necessary();
}

void evicter_t::change_to_correct_eviction_bag(eviction_bag_t *current_bag,
                                               page_t *page) {
    assert_thread();
    rassert(current_bag->has_page(page));
    current_bag->remove(page, page->hypothetical_memory_usage());
    eviction_bag_t *new_bag = correct_eviction_category(page);
    new_bag->add(page, page->hypothetical_memory_usage());
    evict_if_necessary();
}

eviction_bag_t *evicter_t::correct_eviction_category(page_t *page) {
    assert_thread();
    // RSI: is_loading is wrong for deferred loaders.
    if (page->is_loading() || page->has_waiters()) {
        return &unevictable_;
    } else if (page->is_evicted()) {
        return &evicted_;
    } else if (page->is_disk_backed()) {
        return &evictable_disk_backed_;
    } else {
        return &evictable_unbacked_;
    }
}

void evicter_t::remove_page(page_t *page) {
    assert_thread();
    eviction_bag_t *bag = correct_eviction_category(page);
    bag->remove(page, page->hypothetical_memory_usage());
    evict_if_necessary();
    notify_bytes_loading(-static_cast<int64_t>(page->hypothetical_memory_usage()));
}

uint64_t evicter_t::in_memory_size() const {
    assert_thread();
    return unevictable_.size()
        + evictable_disk_backed_.size()
        + evictable_unbacked_.size();
}

void evicter_t::evict_if_necessary() {
    assert_thread();
    // KSI: Implement eviction of unbacked evictables too.  When flushing, you
    // could use the page_t::eviction_index_ field to identify pages that are
    // currently in the process of being evicted, to avoid reflushing a page
    // currently being written for the purpose of eviction.

    page_t *page;
    while (in_memory_size() > memory_limit_
           && evictable_disk_backed_.remove_oldish(&page, access_time_counter_)) {
        evicted_.add(page, page->hypothetical_memory_usage());
        page->evict_self();
    }
}

}  // namespace alt
