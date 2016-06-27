#include "buffer_cache/eviction_bag.hpp"

#include <inttypes.h>

#include "buffer_cache/page.hpp"
#include "random.hpp"
#include "utils.hpp"

namespace alt {

eviction_bag_t::eviction_bag_t()
    : bag_(), size_(0) { }

eviction_bag_t::~eviction_bag_t() {
    guarantee(bag_.size() == 0);
    guarantee(size_ == 0, "size was %" PRIu64, size_);
}

void eviction_bag_t::change_size(int64_t adjustment) {
    rassert(adjustment >= 0 || size_ >= static_cast<uint64_t>(-adjustment));
    size_ += adjustment;
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

bool eviction_bag_t::remove_oldish(page_t **page_out, uint64_t access_time_offset,
                                   page_cache_t *page_cache) {
    if (bag_.size() == 0) {
        return false;
    } else {
        const size_t num_randoms = 5;
        page_t *oldest = bag_.access_random(randsize(bag_.size()));
        for (size_t i = 1; i < num_randoms; ++i) {
            page_t *page = bag_.access_random(randsize(bag_.size()));
            // We compare relative to the access time offset, so that in the unlikely
            // event of a 64-bit overflow, performance degradation is "smooth".
            if (access_time_offset - page->access_time() >
                access_time_offset - oldest->access_time()) {
                oldest = page;
            }
        }

        remove(oldest, oldest->hypothetical_memory_usage(page_cache));
        *page_out = oldest;
        return true;
    }
}


}  // namespace alt
