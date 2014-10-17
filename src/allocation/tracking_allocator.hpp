#ifndef TRACKING_ALLOCATOR_HPP_
#define TRACKING_ALLOCATOR_HPP_

#include <cstddef>
#include <memory>
#include <functional>

class tracking_allocator_factory_t;

// Base class for our allocators.
//
// We need this, because the allocator used forms part of the type for types like
// `std::map` and `std::string`.  As a result, if we were to use `tracking_allocator`
// and `unbounded_allocator` directly, the type of `datum_t` would need to be
// parameterized by the allocator used, which would suck.
template <class T, class Allocator=std::allocator<T> >
class rethinkdb_allocator_t {
public:
    typedef T value_type;
    typedef std::allocator_traits<rethinkdb_allocator_t<T, Allocator> > traits;

    explicit rethinkdb_allocator_t(Allocator _original) : original(_original) {}
    virtual ~rethinkdb_allocator_t() {}

    virtual T* allocate(size_t n) = 0;
    virtual T* allocate(size_t n, const void *hint) = 0;
    virtual void deallocate(T* region, size_t n) = 0;

    void dispose_of(T *ptr) {
        traits::destroy(*this, ptr);
        traits::deallocate(*this, ptr, 1);
    }

    virtual bool operator ==(const rethinkdb_allocator_t<T, Allocator> &other) const = 0;
    bool operator !=(const rethinkdb_allocator_t<T, Allocator> &other) const {
        return !(*this == other);
    }

    virtual size_t max_size() const = 0;

    enum class allocator_types_t {
        UNBOUNDED,
        TRACKING,
    };
    virtual allocator_types_t type() const = 0;

    // function interface
    void operator() (T *ptr) { deallocate(ptr, 1); }
protected:
    Allocator original;
};

// Minimal definitions required to use `std::allocator_traits` and also keep track of
// the amount of memory allocated.
template <class T, class Allocator=std::allocator<T> >
class tracking_allocator_t : public rethinkdb_allocator_t<T, Allocator> {
public:
    tracking_allocator_t(Allocator _original,
                       std::shared_ptr<tracking_allocator_factory_t> _parent)
        : rethinkdb_allocator_t<T, Allocator>(_original), parent(_parent) {}
    tracking_allocator_t(Allocator _original,
                       std::weak_ptr<tracking_allocator_factory_t> _parent)
        : rethinkdb_allocator_t<T, Allocator>(_original), parent(_parent) {}
    explicit tracking_allocator_t(const tracking_allocator_t<T, Allocator> &allocator)
        : rethinkdb_allocator_t<T, Allocator>(allocator), parent(allocator._parent) {}
    explicit tracking_allocator_t(tracking_allocator_t<T, Allocator> &&allocator)
        : rethinkdb_allocator_t<T, Allocator>(allocator), parent(allocator._parent) {}
    virtual ~tracking_allocator_t() {}

    virtual T* allocate(size_t n);
    virtual T* allocate(size_t n, const void *hint);
    virtual void deallocate(T* region, size_t n);

    bool operator ==(const tracking_allocator_t<T, Allocator> &other) const {
        return parent.lock() == other.parent.lock();
    }
    bool operator !=(const tracking_allocator_t<T, Allocator> &other) const {
        return parent.lock() != other.parent.lock();
    }
    virtual bool operator ==(const rethinkdb_allocator_t<T, Allocator> &other) const
    {
        if (other.type() == rethinkdb_allocator_t<T, Allocator>::allocator_types_t::TRACKING)
            return parent.lock() ==
                static_cast<tracking_allocator_t<T, Allocator> &>
                (const_cast<rethinkdb_allocator_t<T, Allocator> &>(other)).parent.lock();
        else
            return false;
    }

    virtual size_t max_size() const;
    virtual typename rethinkdb_allocator_t<T, Allocator>::allocator_types_t type() const {
        return rethinkdb_allocator_t<T, Allocator>::allocator_types_t::TRACKING;
    }

    std::weak_ptr<tracking_allocator_factory_t> parent;
};

// Thin shell around the standard allocator.  We need this so the type
// of `datum_t` doesn't change.
template <class T, class Allocator=std::allocator<T> >
class unbounded_allocator_t : public rethinkdb_allocator_t<T, Allocator> {
public:
    unbounded_allocator_t(Allocator _original)
        : rethinkdb_allocator_t<T, Allocator>(_original) {}
    unbounded_allocator_t(const unbounded_allocator_t<T> &allocator)
        : rethinkdb_allocator_t<T, Allocator>(allocator.original) {}
    unbounded_allocator_t(unbounded_allocator_t<T> &&allocator)
        : rethinkdb_allocator_t<T, Allocator>(allocator.original) {}
    virtual ~unbounded_allocator_t() {}

    virtual T* allocate(size_t n) {
        return this->original.allocate(n);
    }
    virtual T* allocate(size_t n, const void *hint) {
        return this->original.allocate(n, hint);
    }
    virtual void deallocate(T* region, size_t n) {
        this->original.deallocate(region, n);
    }

    bool operator ==(const tracking_allocator_t<T, Allocator> &) const { return true; }
    bool operator !=(const tracking_allocator_t<T, Allocator> &) const { return false; }
    virtual bool operator ==(const rethinkdb_allocator_t<T, Allocator> &other) const
    {
        if (other.type()
            == rethinkdb_allocator_t<T, Allocator>::allocator_types::UNBOUNDED)
            return true;
        else
            return false;
    }
    virtual size_t max_size() const { return std::numeric_limits<size_t>::max(); }
    virtual typename rethinkdb_allocator_t<T, Allocator>::allocator_types_t type() const {
        return rethinkdb_allocator_t<T, Allocator>::allocator_types_t::UNBOUNDED;
    }
};

class tracking_allocator_factory_t {
public:
    tracking_allocator_factory_t(size_t total_bytes) : available(total_bytes) {}

    bool debit(size_t bytes) {
        if (available >= bytes) {
            available -= bytes;
            return true;
        } else {
            return false;
        }
    }
    void credit(size_t bytes) {
        available += bytes;
    }
    size_t left() const {
        return available;
    }
private:
    size_t available;
};

template <class T, class Allocator, class... Args>
T* make(std::allocator_arg_t, Allocator &alloc, Args&&... args) {
    T* result = std::allocator_traits<Allocator>::allocate(alloc, 1);
    std::allocator_traits<Allocator>::construct(alloc, result, std::forward<Args>(args)...);
    return result;
}

#endif /* TRACKING_ALLOCATOR_HPP_ */
