#ifndef CONTAINERS_PTR_BAG_HPP_
#define CONTAINERS_PTR_BAG_HPP_

#include <set>

#include "errors.hpp"

class ptr_baggable_t {
public:
    virtual ~ptr_baggable_t() { }
};

class ptr_bag_t : public ptr_baggable_t {
public:
    ptr_bag_t() { }
    ~ptr_bag_t() {
        for (std::set<ptr_baggable_t *>::iterator
                 it = ptrs.begin(); it != ptrs.end(); ++it) {
            delete *it;
        }
    }

    // We want to be able to add const pointers to the bag too.
    template<class T>
    const T *add(const T *ptr) {
        return add(const_cast<T *>(ptr));
    }

    template<class T>
    T *add(T *ptr) {
        ptrs.insert(static_cast<ptr_baggable_t *>(ptr));
        return ptr;
    }
private:
    // This *MUST* be a set, because it's legal to bag the same pointer twice.
    std::set<ptr_baggable_t *> ptrs;
    DISABLE_COPYING(ptr_bag_t);
};

#endif // CONTAINERS_PTR_BAG_HPP_
