#ifndef BUFFER_CACHE_EVICTER_HPP_
#define BUFFER_CACHE_EVICTER_HPP_

#include <stdint.h>

#include <functional>

#include "buffer_cache/eviction_bag.hpp"
#include "concurrency/auto_drainer.hpp"
#include "concurrency/cache_line_padded.hpp"
#include "concurrency/pubsub.hpp"
#include "threading.hpp"

class cache_balancer_t;
class alt_txn_throttler_t;

namespace alt {

class page_cache_t;

class evicter_t : public home_thread_mixin_debug_only_t {
public:
    void add_not_yet_loaded(page_t *page);
    void add_deferred_loaded(page_t *page);
    void catch_up_deferred_load(page_t *page);
    void add_to_evictable_unbacked(page_t *page);
    void add_to_evictable_disk_backed(page_t *page);
    bool page_is_in_unevictable_bag(page_t *page) const;
    bool page_is_in_evicted_bag(page_t *page) const;
    void move_unevictable_to_evictable(page_t *page);
    void change_to_correct_eviction_bag(eviction_bag_t *current_bag, page_t *page);
    eviction_bag_t *correct_eviction_category(page_t *page);
    eviction_bag_t *unevictable_category() { return &unevictable_; }
    eviction_bag_t *evicted_category() { return &evicted_; }
    void remove_page(page_t *page);
    void reloading_page(page_t *page);

    // Evicter will be unusable until initialize is called
    evicter_t();
    ~evicter_t();

    void initialize(page_cache_t *page_cache,
                    cache_balancer_t *balancer,
                    alt_txn_throttler_t *throttler);
    void update_memory_limit(uint64_t new_memory_limit,
                             uint64_t bytes_loaded_accounted_for,
                             uint64_t access_count_accounted_for,
                             bool read_ahead_ok);

    uint64_t next_access_time() {
        guarantee(initialized_);
        return ++access_time_counter_;
    }

    uint64_t memory_limit() const;
    uint64_t access_count() const;
    uint64_t get_clamped_bytes_loaded() const;

    uint64_t in_memory_size() const;

    // This is decremented past UINT64_MAX to force code to be aware of access time
    // rollovers.
    static const uint64_t INITIAL_ACCESS_TIME = UINT64_MAX - 100;

private:
    friend class usage_adjuster_t;

    // Tells the cache balancer about a page being loaded
    void notify_bytes_loading(int64_t ser_buf_change);

    // Evicts any evictable pages until under the memory limit
    void evict_if_necessary() THROWS_NOTHING;

    bool initialized_;
    page_cache_t *page_cache_;
    cache_balancer_t *balancer_;
    bool *balancer_notify_activity_boolean_;

    alt_txn_throttler_t *throttler_;

    uint64_t memory_limit_;

    // These are updated every time a page is loaded, created, or destroyed, and
    // cleared when cache memory limits are re-evaluated.  This value can go
    // negative, if you keep deleting blocks or suddenly drop a snapshot.
    int64_t bytes_loaded_counter_;
    uint64_t access_count_counter_;

    // This gets incremented every time a page is accessed.
    uint64_t access_time_counter_;

    // This is set to true while `evict_if_necessary()` is active.
    // It avoids reentrant calls to that function.
    bool evict_if_necessary_active_;

    // These track every page's eviction status.
    eviction_bag_t unevictable_;
    eviction_bag_t evictable_disk_backed_;
    eviction_bag_t evictable_unbacked_;
    eviction_bag_t evicted_;

    auto_drainer_t drainer_;

    DISABLE_COPYING(evicter_t);
};

// This adjusts the memory usage in the destructor, with the _same_ eviction bag the
// page had in the constructor -- its lifespan ends before the page would have its
// eviction bag changed.
class usage_adjuster_t {
public:
    usage_adjuster_t(page_cache_t *page_cache, page_t *page);
    ~usage_adjuster_t();

private:
    page_cache_t *const page_cache_;
    page_t *const page_;
    eviction_bag_t *const eviction_bag_;
    const uint32_t original_usage_;

    DISABLE_COPYING(usage_adjuster_t);
};

}  // namespace alt

#endif  // BUFFER_CACHE_EVICTER_HPP_
