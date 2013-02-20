#include "containers/ptr_bag.hpp"

// #define PTR_BAG_LOG 1

#ifdef PTR_BAG_LOG
#include "activity_logger.hpp"
static activity_logger_t ptr_bag_log;
#define pblog(...) debugf_log(ptr_bag_log, __VA_ARGS__)
#else
#define pblog(...)
#endif // PTR_BAG_LOG

ptr_bag_t::ptr_bag_t() : parent(0), mem_estimate_(0) {
    pblog("%p created", this);
}
ptr_bag_t::~ptr_bag_t() {
    assert_thread();
    guarantee(!parent || ptrs.size() == 0);
    for (std::set<ptr_baggable_t *>::iterator
             it = ptrs.begin(); it != ptrs.end(); ++it) {
        pblog("deleting %p from %p", *it, this);
        delete *it;
    }
}

bool ptr_bag_t::has(const ptr_baggable_t *ptr) {
    if (parent) return parent->has(ptr);
    return ptrs.count(const_cast<ptr_baggable_t *>(ptr)) > 0;
}
void ptr_bag_t::yield_to(ptr_bag_t *new_bag, const ptr_baggable_t *ptr) {
    size_t num_erased = ptrs.erase(const_cast<ptr_baggable_t *>(ptr));
    guarantee(num_erased == 1);
    new_bag->add(ptr);
}

std::string ptr_bag_t::print_debug() const {
    std::string acc = strprintf("%zu(%zu) [", ptrs.size(), mem_estimate_);
    for (std::set<ptr_baggable_t *>::const_iterator
             it = ptrs.begin(); it != ptrs.end(); ++it) {
        acc += (it == ptrs.begin() ? "" : ", ") + strprintf("%p", *it);
    }
    return acc + "]";
}

size_t ptr_bag_t::mem_estimate() const {
    return parent ? parent->mem_estimate() : (mem_estimate_ * mem_estimate_multiplier);
}

void ptr_bag_t::real_add(ptr_baggable_t *ptr, size_t mem_estimate) {
    pblog("adding %p to %p", ptr, this);
    assert_thread();
    if (parent) {
        guarantee(ptrs.size() == 0);
        guarantee(mem_estimate_ == 0);
        parent->real_add(ptr, mem_estimate);
    } else {
        guarantee(ptrs.count(ptr) == 0);
        ptrs.insert(static_cast<ptr_baggable_t *>(ptr));
        mem_estimate_ += mem_estimate;
    }
}

void ptr_bag_t::shadow(ptr_bag_t *_parent, size_t *bag_size_out) {
    parent = _parent;
    for (std::set<ptr_baggable_t *>::iterator
             it = ptrs.begin(); it != ptrs.end(); ++it) {
        parent->real_add(*it, 0);
    }
    ptrs = std::set<ptr_baggable_t *>();
    *bag_size_out = mem_estimate_;
    mem_estimate_ = 0;
}

void debug_print(append_only_printf_buffer_t *buf, const ptr_bag_t &pbag) {
    buf->appendf("%s", pbag.print_debug().c_str());
}
