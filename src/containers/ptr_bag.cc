#include "containers/ptr_bag.hpp"

#ifdef PTR_BAG_LOG
#include "activity_logger.hpp"
static activity_logger_t ptr_bag_log;
#define pblog(...) debugf_log(ptr_bag_log, __VA_ARGS__)
#else
#define pblog(...)
#endif // PTR_BAG_LOG

ptr_bag_t::ptr_bag_t() : parent(0), size_est_(0) {
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
    return ptrs.count(const_cast<ptr_baggable_t *>(ptr)) > 0;
}
void ptr_bag_t::yield_to(ptr_bag_t *new_bag, const ptr_baggable_t *ptr, bool dup_ok) {
    size_t num_erased = ptrs.erase(const_cast<ptr_baggable_t *>(ptr));
    guarantee(num_erased == 1);
    // TODO(mlucy): this is fragile and should go away.
    if (dup_ok && new_bag->has(ptr)) return;
    new_bag->add(ptr);
}

std::string ptr_bag_t::print_debug() const {
    std::string acc = strprintf("%lu(%lu) [", ptrs.size(), size_est_);
    for (std::set<ptr_baggable_t *>::const_iterator
             it = ptrs.begin(); it != ptrs.end(); ++it) {
        acc += (it == ptrs.begin() ? "" : ", ") + strprintf("%p", *it);
    }
    return acc + "]";
}

size_t ptr_bag_t::size_est() const {
    return parent ? parent->size_est() : (size_est_ * size_est_mul);
}

void ptr_bag_t::real_add(ptr_baggable_t *ptr, size_t size_est) {
    pblog("adding %p to %p", ptr, this);
    assert_thread();
    if (parent) {
        guarantee(ptrs.size() == 0);
        guarantee(size_est_ == 0);
        parent->real_add(ptr, size_est);
    } else {
        guarantee(ptrs.count(ptr) == 0);
        ptrs.insert(static_cast<ptr_baggable_t *>(ptr));
        size_est_ += size_est;
    }
}

void ptr_bag_t::shadow(ptr_bag_t *_parent, size_t *bag_size_out) {
    parent = _parent;
    for (std::set<ptr_baggable_t *>::iterator
             it = ptrs.begin(); it != ptrs.end(); ++it) {
        parent->real_add(*it, 0);
    }
    ptrs = std::set<ptr_baggable_t *>();
    *bag_size_out = size_est_;
    size_est_ = 0;
}
