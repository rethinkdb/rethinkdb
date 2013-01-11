#ifndef CONTAINERS_PTR_BAG_HPP_
#define CONTAINERS_PTR_BAG_HPP_

#include <set>

//#define PTR_BAG_LOG 1
#ifdef PTR_BAG_LOG
#include "activity_logger.hpp"
static activity_logger_t ptr_bag_log;
#define pblog(...) debugf_log(ptr_bag_log, __VA_ARGS__)
#else
#define pblog(...)
#endif // PTR_BAG_LOG

#include "utils.hpp"

// Classes that can be put into a pointer bag should inherit from this.
class ptr_baggable_t {
public:
    virtual ~ptr_baggable_t() { }
};

// A pointer bag holds a bunch of pointers and deletes them when it's freed.
class ptr_bag_t : public ptr_baggable_t, private home_thread_mixin_t {
public:
    ptr_bag_t() : parent(0) {
        pblog("%p created", this);
    }
    ~ptr_bag_t() {
        assert_thread();
        guarantee(!parent || ptrs.size() == 0);
        for (std::set<ptr_baggable_t *>::iterator
                 it = ptrs.begin(); it != ptrs.end(); ++it) {
            pblog("deleting %p from %p", *it, this);
            delete *it;
        }
    }

    // We want to be able to add const pointers to the bag too.
    template<class T>
    const T *add(const T *ptr) { return add(const_cast<T *>(ptr)); }

    // Add a pointer to the bag; it will be deleted when the bag is destroyed.
    template<class T>
    T *add(T *ptr) {
        real_add(static_cast<ptr_baggable_t *>(ptr));
        return ptr;
    }

    // We want to make sure that if people add `p` to ptr_bag `A`, then add `A`
    // to ptr_bag `B`, then add `p` to `B`, that `p` doesn't get double-freed.
    ptr_bag_t *add(ptr_bag_t *sub_bag) {
        sub_bag->shadow(this);
        real_add(static_cast<ptr_baggable_t *>(sub_bag));
        return sub_bag;
    }

    bool has(const ptr_baggable_t *ptr) {
        return ptrs.count(const_cast<ptr_baggable_t *>(ptr)) > 0;
    }

    void yield_to(ptr_bag_t *new_bag, const ptr_baggable_t *ptr, bool dup_ok = false) {
        size_t num_erased = ptrs.erase(const_cast<ptr_baggable_t *>(ptr));
        guarantee(num_erased == 1);
        if (dup_ok && new_bag->has(ptr)) return;
        new_bag->add(ptr);
    }

    std::string print_debug() const {
        std::string acc = strprintf("%lu [", ptrs.size());
        for (std::set<ptr_baggable_t *>::const_iterator
                 it = ptrs.begin(); it != ptrs.end(); ++it) {
            acc += (it == ptrs.begin() ? "" : ", ") + strprintf("%p", *it);
        }
        return acc + "]";
    }
private:
    void real_add(ptr_baggable_t *ptr) {
        pblog("adding %p to %p", ptr, this);
        assert_thread();
        if (parent) {
            parent->real_add(ptr);
        } else {
            guarantee(ptrs.count(ptr) == 0);
            ptrs.insert(static_cast<ptr_baggable_t *>(ptr));
        }
    }

    // When a ptr_bag shadows another ptr_bag, any pointers inserted into it in
    // the past or future are inserted into the parent ptr_bag.
    void shadow(ptr_bag_t *_parent) {
        parent = _parent;
        for (std::set<ptr_baggable_t *>::iterator
                 it = ptrs.begin(); it != ptrs.end(); ++it) {
            parent->real_add(*it);
        }
        ptrs = std::set<ptr_baggable_t *>();
        guarantee(ptrs.size() == 0);
    }

    ptr_bag_t *parent; // See `shadow`.
    std::set<ptr_baggable_t *> ptrs;
    DISABLE_COPYING(ptr_bag_t);
};

#endif // CONTAINERS_PTR_BAG_HPP_
