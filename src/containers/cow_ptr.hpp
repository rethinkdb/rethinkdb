#ifndef CONTAINERS_COW_PTR_HPP_
#define CONTAINERS_COW_PTR_HPP_

#include "containers/intrusive_ptr.hpp"


/* `cow_ptr_t<T>` (short for [c]opy-[o]n-[w]rite pointer) acts like a container
that holds a single `T`, but it internally shares references to the `T` so that
copying a `cow_ptr_t<T>` is fast even if copying a `T` is not.

Even though it has `ptr_t` in its name, it's not like most smart pointers.
`cow_ptr_t` is never empty, and it's impossible to make `cow_ptr_t` take
ownership of an object you allocated on the heap. */


template <class T>
class cow_pointee_t;

template <class T>
class cow_ptr_t {
public:
    cow_ptr_t();   /* Fills the `cow_ptr_t` with a default-constructed `T` */
    explicit cow_ptr_t(const T &t);
    cow_ptr_t(const cow_ptr_t &ref);
    ~cow_ptr_t();
    cow_ptr_t &operator=(const cow_ptr_t &other);

    const T &operator*() const;
    const T *operator->() const;
    const T *get() const;

    void set(const T &new_value);

    class change_t {
    public:
        change_t(cow_ptr_t *parent);
        ~change_t();
        T *get();
    private:
        cow_ptr_t *parent;
    };

private:
    intrusive_ptr_t<cow_pointee_t<T> > ptr;

    /* Number of outstanding `change_t` objects for us. */
    intptr_t change_count;
};

#include "containers/cow_ptr.tcc"

#endif   /* CONTAINERS_COW_PTR_HPP_ */
