#ifndef ALLOCATION_UNUSUAL_ALLOCATOR_HPP
#define ALLOCATION_UNUSUAL_ALLOCATOR_HPP

#include "allocation/tracking_allocator.hpp"

// For weird types like shared_buf_t that have char[]s in them.  No
// suballocators possible.
template <typename T>
class unusual_size_allocator_t {
public:
    unusual_size_allocator_t(std::shared_ptr<tracking_allocator_factory_t> _parent,
                             size_t _extra)
        : parent(_parent), extra(_extra) {}
    unusual_size_allocator_t(std::weak_ptr<tracking_allocator_factory_t> _parent,
                             size_t _extra)
        : parent(_parent), extra(_extra) {}
    explicit unusual_size_allocator_t(const unusual_size_allocator_t<T> &allocator)
        : parent(allocator.parent), extra(allocator.extra) {}
    explicit unusual_size_allocator_t(unusual_size_allocator_t<T> &&allocator)
        : parent(std::move(allocator.parent)), extra(std::move(allocator.extra)) {}
    ~unusual_size_allocator_t() {}

    typedef T value_type;
    typedef std::allocator_traits<unusual_size_allocator_t<T> > traits;

    T* allocate(size_t n);
    void deallocate(T* region, size_t n);

    void dispose_of(T *ptr) {
        traits::destroy(*this, ptr);
        traits::deallocate(*this, ptr, 1);
    }

    bool operator==(const unusual_size_allocator_t<T> &other) const {
        return other.parent.lock() == parent.lock() && other.extra == extra;
    }
    bool operator!=(const unusual_size_allocator_t<T> &other) const {
        return !(*this == other);
    }

    size_t max_size() const;

private:
    std::weak_ptr<tracking_allocator_factory_t> parent;
    size_t extra;

    size_t full_size(size_t copies) const {
        return (sizeof(T) + extra) * copies;
    }
};

#endif // ALLOCATION_UNUSUAL_ALLOCATOR_HPP
