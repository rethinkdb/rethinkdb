#ifndef BUFFER_CACHE_ALT_EVICTION_BAG_HPP_
#define BUFFER_CACHE_ALT_EVICTION_BAG_HPP_

#include <stdint.h>

#include "containers/backindex_bag.hpp"

namespace alt {

class page_t;

class eviction_bag_t {
public:
    eviction_bag_t();
    ~eviction_bag_t();

    // For adding a page that doesn't have a size yet -- its size is not yet
    // accounted for, because its buf isn't loaded yet.
    void add_without_size(page_t *page);

    // Adds the size for a page that was added with add_without_size, now that it has
    // been loaded and we know the size.
    void add_size(uint32_t ser_buf_size);

    // Adds the page with its known size.
    void add(page_t *page, uint32_t ser_buf_size);

    // Removes the page with its size.
    void remove(page_t *page, uint32_t ser_buf_size);

    // Returns true if this bag contains the given page.
    bool has_page(page_t *page) const;

    uint64_t size() const { return size_; }

    bool remove_oldish(page_t **page_out, uint64_t access_time_offset);

private:
    backindex_bag_t<page_t *> bag_;
    // The size in memory.
    uint64_t size_;

    DISABLE_COPYING(eviction_bag_t);
};


}  // namespace alt

#endif  // BUFFER_CACHE_ALT_EVICTION_BAG_HPP_
