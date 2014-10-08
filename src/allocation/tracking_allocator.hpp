#ifndef TRACKING_ALLOCATOR_HPP_
#define TRACKING_ALLOCATOR_HPP_

#include <cstddef>
#include <memory>

class tracking_allocator_factory;

// Base class for our allocators.
//
// We need this, because the allocator used forms part of the type for types like
// `std::map` and `std::string`.  As a result, if we were to use `tracking_allocator`
// and `unbounded_allocator` directly, the type of `datum_t` would need to be
// parameterized by the allocator used, which would suck.
template <class T, class Allocator=std::allocator<T>>
class rethinkdb_allocator {
public:
    typedef T value_type;

    rethinkdb_allocator(Allocator _original) : original(_original) {}
    
    virtual T* allocate(size_t n) = 0;
    virtual void deallocate(T* region, size_t n) = 0;

    virtual bool operator == (const rethinkdb_allocator<T, Allocator> &other) = 0;
    bool operator != (const rethinkdb_allocator<T, Allocator> &other) { return !(*this == other); }
protected:
    enum class allocator_types {
        UNBOUNDED,
        TRACKING,
    };
    virtual allocator_types type() = 0;

    Allocator original;
};

// Minimal definitions required to use `std::allocator_traits` and also keep track of
// the amount of memory allocated.
template <class T, class Allocator=std::allocator<T>>
class tracking_allocator : public rethinkdb_allocator<T, Allocator> {
public:
    tracking_allocator(Allocator _original, tracking_allocator_factory *_parent) : rethinkdb_allocator<T, Allocator>(_original), parent(_parent) {}
    tracking_allocator(const tracking_allocator<T, Allocator> &allocator) : rethinkdb_allocator<T, Allocator>(allocator), parent(allocator._parent) {}
    tracking_allocator(tracking_allocator<T, Allocator> &&allocator) : rethinkdb_allocator<T, Allocator>(allocator), parent(allocator._parent) {}
    
    virtual T* allocate(size_t n);
    virtual void deallocate(T* region, size_t n);

    bool operator == (const tracking_allocator<T, Allocator> &other) { return parent == other.parent; }
    bool operator != (const tracking_allocator<T, Allocator> &other) { return parent != other.parent; }
    virtual bool operator == (const rethinkdb_allocator<T, Allocator> &other)
    {
        if (other.type() == rethinkdb_allocator<T, Allocator>::allocator_types::TRACKING)
            return parent == static_cast<tracking_allocator<T, Allocator>>(other).parent;
        else
            return false;
    }
private:
    tracking_allocator_factory *parent;
};

// Thin shell around the standard allocator.  We need this so the type of `datum_t` doesn't change.
template <class T, class Allocator=std::allocator<T>>
class unbounded_allocator : public rethinkdb_allocator<T, Allocator> {
public:
    unbounded_allocator() {}
    unbounded_allocator(const unbounded_allocator<T> &allocator) : rethinkdb_allocator<T, Allocator>(allocator) {}
    unbounded_allocator(unbounded_allocator<T> &&allocator) : rethinkdb_allocator<T, Allocator>(allocator) {}
    
    virtual T* allocate(size_t n) { return this->original.allocate(n); }
    virtual void deallocate(T* region, size_t n) { this->original.deallocate(region, n); }

    bool operator == (const tracking_allocator<T, Allocator> &) { return true; }
    bool operator != (const tracking_allocator<T, Allocator> &) { return false; }
    virtual bool operator == (const rethinkdb_allocator<T, Allocator> &other)
    {
        if (other.type() == rethinkdb_allocator<T, Allocator>::allocator_types::UNBOUNDED)
            return true;
        else
            return false;
    }
};

class tracking_allocator_factory {
public:
    tracking_allocator_factory(size_t total_bytes) : available(total_bytes) {}
    template <class T, class Allocator>
    tracking_allocator<T, Allocator> allocator_for(Allocator original) {
        return tracking_allocator<T, Allocator>(original, this);
    }

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
private:
    size_t available;
};

#endif /* TRACKING_ALLOCATOR_HPP_ */
