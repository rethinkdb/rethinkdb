#include "buffer_cache/alt/evicter.hpp"

#include "buffer_cache/alt/page.hpp"

namespace alt {

evicter_t::evicter_t(memory_tracker_t *tracker, uint64_t memory_limit)
    : tracker_(tracker), memory_limit_(memory_limit),
      access_time_counter_(INITIAL_ACCESS_TIME) { }

evicter_t::~evicter_t() {
    assert_thread();
}


void evicter_t::add_not_yet_loaded(page_t *page) {
    assert_thread();
    unevictable_.add_without_size(page);
    inform_tracker();
}

void evicter_t::add_now_loaded_size(uint32_t in_memory_buf_size) {
    assert_thread();
    unevictable_.add_size(in_memory_buf_size);
    inform_tracker();
    evict_if_necessary();
}

bool evicter_t::page_is_in_unevictable_bag(page_t *page) const {
    assert_thread();
    return unevictable_.has_page(page);
}

void evicter_t::add_to_evictable_unbacked(page_t *page) {
    assert_thread();
    evictable_unbacked_.add(page, page->hypothetical_memory_usage());
    inform_tracker();
    evict_if_necessary();
}

void evicter_t::add_to_evictable_disk_backed(page_t *page) {
    assert_thread();
    evictable_disk_backed_.add(page, page->hypothetical_memory_usage());
    inform_tracker();
    evict_if_necessary();
}

void evicter_t::move_unevictable_to_evictable(page_t *page) {
    assert_thread();
    rassert(unevictable_.has_page(page));
    unevictable_.remove(page, page->hypothetical_memory_usage());
    eviction_bag_t *new_bag = correct_eviction_category(page);
    rassert(new_bag == &evictable_disk_backed_
            || new_bag == &evictable_unbacked_);
    new_bag->add(page, page->hypothetical_memory_usage());
    inform_tracker();
    evict_if_necessary();
}

void evicter_t::change_to_correct_eviction_bag(eviction_bag_t *current_bag,
                                               page_t *page) {
    assert_thread();
    rassert(current_bag->has_page(page));
    current_bag->remove(page, page->hypothetical_memory_usage());
    eviction_bag_t *new_bag = correct_eviction_category(page);
    new_bag->add(page, page->hypothetical_memory_usage());
    inform_tracker();
    evict_if_necessary();
}

eviction_bag_t *evicter_t::correct_eviction_category(page_t *page) {
    assert_thread();
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
    inform_tracker();
    evict_if_necessary();
}

uint64_t evicter_t::in_memory_size() const {
    assert_thread();
    return unevictable_.size()
        + evictable_disk_backed_.size()
        + evictable_unbacked_.size();
}

bool evicter_t::interested_in_read_ahead_block(uint32_t in_memory_block_size) const {
    return in_memory_size() + in_memory_block_size < memory_limit_;
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

void evicter_t::inform_tracker() const {
    tracker_->inform_memory_change(in_memory_size(),
                                   memory_limit_);
}


}  // namespace alt
