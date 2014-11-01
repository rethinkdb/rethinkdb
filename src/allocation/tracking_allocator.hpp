#ifndef ALLOCATION_TRACKING_ALLOCATOR_HPP_
#define ALLOCATION_TRACKING_ALLOCATOR_HPP_

#include <cstddef>
#include <memory>
#include <functional>

#include "threading.hpp"

class tracking_allocator_factory_t;

enum class allocator_types_t {
    UNBOUNDED,
    TRACKING,
};

// Base class for our allocators.
//
// We need this, because the allocator used forms part of the type for types like
// `std::map` and `std::string`.  As a result, if we were to use `tracking_allocator`
// and `unbounded_allocator` directly, the type of `datum_t` would need to be
// parameterized by the allocator used, which would suck.
template <class T>
class rethinkdb_allocator_t {
public:
    typedef T value_type;
    typedef std::allocator_traits<rethinkdb_allocator_t<T> > traits;

    virtual ~rethinkdb_allocator_t() {}

    virtual T* allocate(size_t n) = 0;
    virtual T* allocate(size_t n, const void *hint) = 0;
    virtual void deallocate(T* region, size_t n) = 0;

    void dispose_of(T *ptr) {
        traits::destroy(*this, ptr);
        traits::deallocate(*this, ptr, 1);
    }

    virtual bool operator==(const rethinkdb_allocator_t<T> &other) const = 0;
    bool operator!=(const rethinkdb_allocator_t<T> &other) const {
        return !(*this == other);
    }

    virtual size_t max_size() const = 0;

    virtual allocator_types_t type() const = 0;

    // function interface
    void operator() (T *ptr) { deallocate(ptr, 1); }
};

// Minimal definitions required to use `std::allocator_traits` and also keep track of
// the amount of memory allocated.
template <class T>
class tracking_allocator_t : public rethinkdb_allocator_t<T> {
public:
    explicit tracking_allocator_t(std::shared_ptr<tracking_allocator_factory_t> _parent)
        : parent(_parent) {}
    explicit tracking_allocator_t(std::weak_ptr<tracking_allocator_factory_t> _parent)
        : parent(_parent) {}
    explicit tracking_allocator_t(const tracking_allocator_t<T> &allocator)
        : parent(allocator._parent) {}
    explicit tracking_allocator_t(tracking_allocator_t<T> &&allocator)
        : parent(allocator._parent) {}
    virtual ~tracking_allocator_t() {}

    virtual T* allocate(size_t n);
    virtual T* allocate(size_t n, const void *hint);
    virtual void deallocate(T* region, size_t n);

    bool operator==(const tracking_allocator_t<T> &other) const {
        return parent.lock() == other.parent.lock();
    }
    bool operator!=(const tracking_allocator_t<T> &other) const {
        return parent.lock() != other.parent.lock();
    }
    virtual bool operator==(const rethinkdb_allocator_t<T> &other) const {
        if (other.type() == allocator_types_t::TRACKING) {
            return parent.lock() ==
                static_cast<const tracking_allocator_t<T> &>(other)
                .parent.lock();
        } else {
            return false;
        }
    }

    virtual size_t max_size() const;
    virtual allocator_types_t type() const {
        return allocator_types_t::TRACKING;
    }

    std::weak_ptr<tracking_allocator_factory_t> parent;
};

// Thin shell around the standard allocator.  We need this so the type
// of `datum_t` doesn't change.
template <class T>
class unbounded_allocator_t : public rethinkdb_allocator_t<T> {
public:
    unbounded_allocator_t() {}
    explicit unbounded_allocator_t(const unbounded_allocator_t<T> &) {}
    explicit unbounded_allocator_t(unbounded_allocator_t<T> &&) {}
    virtual ~unbounded_allocator_t() {}

    virtual T* allocate(size_t n) {
        return std::allocator<T>().allocate(n);
    }
    virtual T* allocate(size_t n, const void *hint) {
        return std::allocator<T>().allocate(n, hint);
    }
    virtual void deallocate(T* region, size_t n) {
        std::allocator<T>().deallocate(region, n);
    }

    bool operator==(const unbounded_allocator_t<T> &) const { return true; }
    bool operator!=(const unbounded_allocator_t<T> &) const { return false; }
    virtual bool operator==(const rethinkdb_allocator_t<T> &other) const
    {
        if (other.type() == allocator_types_t::UNBOUNDED)
            return true;
        else
            return false;
    }
    virtual size_t max_size() const { return std::numeric_limits<size_t>::max(); }
    virtual allocator_types_t type() const {
        return allocator_types_t::UNBOUNDED;
    }
};

class tracking_allocator_factory_t : public home_thread_mixin_debug_only_t {
public:
    explicit tracking_allocator_factory_t(size_t total_bytes) : available(total_bytes) {}

    bool debit(size_t bytes) {
        assert_thread();
        if (available >= bytes) {
            available -= bytes;
            return true;
        } else {
            return false;
        }
    }
    void credit(size_t bytes) {
        assert_thread();
        available += bytes;
    }
    size_t left() const {
        assert_thread();
        return available;
    }
private:
    size_t available;

    DISABLE_COPYING(tracking_allocator_factory_t);
};

#endif /* ALLOCATION_TRACKING_ALLOCATOR_HPP_ */
