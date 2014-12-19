#ifndef BUFFER_CACHE_EVICTION_BAG_HPP_
#define BUFFER_CACHE_EVICTION_BAG_HPP_

#include <stdint.h>

#include "containers/backindex_bag.hpp"

namespace alt {

class page_t;
class page_cache_t;

class eviction_bag_t {
public:
    eviction_bag_t();
    ~eviction_bag_t();

    // Adjusts the size, given how much the size has changed of one of the pages in
    // the bag.
    void change_size(int64_t adjustment);

    // Adds the page with its known size.
    void add(page_t *page, uint32_t ser_buf_size);

    // Removes the page with its size.
    void remove(page_t *page, uint32_t ser_buf_size);

    // Returns true if this bag contains the given page.
    bool has_page(page_t *page) const;

    uint64_t size() const { return size_; }

    bool remove_oldish(page_t **page_out, uint64_t access_time_offset,
                       page_cache_t *page_cache);

private:
    backindex_bag_t<page_t *> bag_;
    // The size in memory.
    uint64_t size_;

    DISABLE_COPYING(eviction_bag_t);
};


}  // namespace alt

#endif  // BUFFER_CACHE_EVICTION_BAG_HPP_
