#ifndef BUFFER_CACHE_ALT_EVICTER_HPP_
#define BUFFER_CACHE_ALT_EVICTER_HPP_

#include <stdint.h>

#include <functional>

#include "buffer_cache/alt/eviction_bag.hpp"
#include "concurrency/cache_line_padded.hpp"
#include "concurrency/pubsub.hpp"
#include "threading.hpp"

class cache_balancer_t;

namespace alt {

class page_cache_t;

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
    eviction_bag_t *unevictable_category() { return &unevictable_; }
    void remove_page(page_t *page);

    explicit evicter_t(page_cache_t *page_cache,
                       cache_balancer_t *balancer);
    ~evicter_t();

    void update_memory_limit(uint64_t new_memory_limit);

    uint64_t next_access_time() {
        return ++access_time_counter_;
    }

    uint64_t get_memory_limit() const {
        return memory_limit_;
    }

    int64_t get_bytes_loaded() const {
        __sync_synchronize();
        int64_t res = bytes_loaded_counter_;
        __sync_synchronize();
        return res;
    }

    uint64_t in_memory_size() const;

    static const uint64_t INITIAL_ACCESS_TIME = UINT64_MAX - 100;

private:
    // Tells the cache balancer about a page being loaded
    void notify_access();

    // Evicts any evictable pages until under the memory limit
    void evict_if_necessary();

    page_cache_t *const page_cache_;
    cache_balancer_t *const balancer_;
    uint64_t memory_limit_;

    // This is updated every time a page is loaded or created,
    // and cleared when cache memory limits are re-evaluated.
    // May be read from other threads.
    int64_t bytes_loaded_counter_;

    // This gets incremented every time a page is accessed.
    uint64_t access_time_counter_;

    // These track every page's eviction status.
    eviction_bag_t unevictable_;
    eviction_bag_t evictable_disk_backed_;
    eviction_bag_t evictable_unbacked_;
    eviction_bag_t evicted_;

    DISABLE_COPYING(evicter_t);
};

}  // namespace alt

#endif  // BUFFER_CACHE_ALT_EVICTER_HPP_
