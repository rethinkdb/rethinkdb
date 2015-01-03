// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_CLONE_PTR_HPP_
#define CONTAINERS_CLONE_PTR_HPP_

#include "containers/archive/archive.hpp"
#include "containers/scoped.hpp"
#include "rpc/serialize_macros.hpp"

/* `clone_ptr_t` is a smart pointer that calls the `clone()` method on its
underlying object whenever the `clone_ptr_t`'s copy constructor is called. It's
primarily useful when you have a type that effectively acts like a piece of
data (i.e. it can be meaningfully copied) but it also has virtual methods.
Remember to declare `clone()` as a virtual method! */

template<class T>
class clone_ptr_t {
public:
    clone_ptr_t();

    /* Takes ownership of the argument. */
    explicit clone_ptr_t(T *p);

    // (We have noexcept specifiers on move operations in particular so that STL
    // containers don't have to copy.)
    clone_ptr_t(clone_ptr_t &&movee) noexcept : object(std::move(movee.object)) { }

    template<class U>
    clone_ptr_t(clone_ptr_t<U> &&movee) noexcept : object(std::move(movee.object)) { }

    clone_ptr_t(const clone_ptr_t &x);
    template<class U>
    clone_ptr_t(const clone_ptr_t<U> &x);  // NOLINT(runtime/explicit)

    clone_ptr_t &operator=(const clone_ptr_t &x);
    clone_ptr_t &operator=(const clone_ptr_t &&x) noexcept;

    template<class U>
    clone_ptr_t &operator=(const clone_ptr_t<U> &x);
    template<class U>
    clone_ptr_t &operator=(const clone_ptr_t<U> &&x) noexcept;



    T &operator*() const;
    T *operator->() const;
    T *get() const;

    bool has() const {
        return object.has();
    }

private:
    template<class U> friend class clone_ptr_t;

    template <cluster_version_t W>
    void rdb_serialize(write_message_t *wm) const {
        // clone pointers own their pointees exclusively, so we don't
        // have to worry about replicating any boost pointer
        // serialization bullshit.
        bool exists = object;
        serialize<W>(wm, exists);
        if (exists) {
            serialize<W>(wm, *object);
        }
    }

    template <cluster_version_t W>
    archive_result_t rdb_deserialize(read_stream_t *s) {
        rassert(!object.has());
        object.reset();
        T *tmp;
        archive_result_t res = deserialize<W>(s, &tmp);
        object.init(tmp);
        return res;
    }

    scoped_ptr_t<T> object;
};

RDB_SERIALIZE_TEMPLATED_OUTSIDE(clone_ptr_t<T>);

#include "containers/clone_ptr.tcc"

#endif /* CONTAINERS_CLONE_PTR_HPP_ */
