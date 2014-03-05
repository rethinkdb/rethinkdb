#ifndef BUFFER_CACHE_ALT_EVICTER_HPP_
#define BUFFER_CACHE_ALT_EVICTER_HPP_

#include <stdint.h>

#include "buffer_cache/alt/eviction_bag.hpp"
#include "threading.hpp"

class alt_cache_balancer_t;

namespace alt {

class evicter_t : public home_thread_mixin_debug_only_t {
public:
    void add_not_yet_loaded(page_t *page);
    void add_now_loaded_size(uint32_t ser_buf_size);
    void add_to_evictable_unbacked(page_t *page);
    void add_to_evictable_disk_backed(page_t *page);
    bool page_is_in_unevictable_bag(page_t *page) const;
    void move_unevictable_to_evictable(page_t *page);
    void change_to_correct_eviction_bag(eviction_bag_t *current_bag, page_t *page);
    eviction_bag_t *correct_eviction_category(page_t *page);
    void remove_page(page_t *page);

    explicit evicter_t(alt_cache_balancer_t *balancer,
                       uint64_t memory_limit);
    ~evicter_t();

    bool interested_in_read_ahead_block(uint32_t ser_block_size) const;

    void update_memory_limit(uint64_t new_memory_limit);

    uint64_t next_access_time() {
        return ++access_time_counter_;
    }

    uint64_t get_cache_misses() const {
        return cache_miss_counter_;
    }

    static const uint64_t INITIAL_ACCESS_TIME = UINT64_MAX - 100;

private:
    void evict_if_necessary();
    uint64_t in_memory_size() const;

    void inform_tracker() const;

    alt_cache_balancer_t *const balancer_;
    uint64_t memory_limit_;

    // This gets incremented every time a page is loaded,
    // and cleared when cache memory limits are re-evaluated
    uint64_t cache_miss_counter_;

    // This gets incremented every time a page is accessed.
    uint64_t access_time_counter_;

    // These track whether every page's eviction status.
    eviction_bag_t unevictable_;
    eviction_bag_t evictable_disk_backed_;
    eviction_bag_t evictable_unbacked_;
    eviction_bag_t evicted_;

    DISABLE_COPYING(evicter_t);
};

}  // namespace alt

#endif  // BUFFER_CACHE_ALT_EVICTER_HPP_
