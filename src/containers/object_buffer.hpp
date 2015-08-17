// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CONTAINERS_OBJECT_BUFFER_HPP_
#define CONTAINERS_OBJECT_BUFFER_HPP_

#include <new>
#include <utility>

#include "errors.hpp"

// Caveat: do not use this template with an object that has a blocking destructor, if
//  you are going to allocate multiple times using a single object_buffer_t.  This object
//  should catch it if you try to do anything particularly stupid, though.
template <class T>
class object_buffer_t {
public:
    class destruction_sentinel_t {
    public:
        explicit destruction_sentinel_t(object_buffer_t<T> *_parent) : parent(_parent) { }

        ~destruction_sentinel_t() {
            if (parent->has()) {
                parent->reset();
            }
        }
    private:
        object_buffer_t<T> *parent;

        DISABLE_COPYING(destruction_sentinel_t);
    };

    object_buffer_t() : state(EMPTY) { }
    ~object_buffer_t() {
        // The buffer cannot be destroyed while an object is in the middle of
        //  constructing or destructing
        if (state == INSTANTIATED) {
            reset();
        } else {
            rassert(state == EMPTY);
        }
    }

    template <class... Args>
    T *create(Args &&... args) {
        rassert(state == EMPTY);
        state = CONSTRUCTING;
        try {
            new (&object_data[0]) T(std::forward<Args>(args)...);
        } catch (...) {
            state = EMPTY;
            throw;
        }
        state = INSTANTIATED;
        return get();
    }

    T *get() {
        rassert(state == INSTANTIATED);
        return reinterpret_cast<T *>(&object_data[0]);
    }

    T *operator->() {
        return get();
    }

    const T *get() const {
        rassert(state == INSTANTIATED);
        return reinterpret_cast<const T *>(&object_data[0]);
    }

    const T *operator->() const {
        return get();
    }

    void reset() {
        guarantee(state == INSTANTIATED || state == EMPTY);
        if (state == INSTANTIATED) {
            T *obj_ptr = get();
            state = DESTRUCTING;
            obj_ptr->~T();
            state = EMPTY;
        }
    }

    bool has() const {
        return (state == INSTANTIATED);
    }

private:
    // Force alignment of the data to the alignment of the templatized type,
    // this avoids some optimization errors, see github issue #3300 for an example.
    char object_data[sizeof(T)] __attribute__((aligned(alignof(T))));

    enum buffer_state_t {
        EMPTY,
        CONSTRUCTING,
        INSTANTIATED,
        DESTRUCTING
    } state;

    DISABLE_COPYING(object_buffer_t);
};

#endif  // CONTAINERS_OBJECT_BUFFER_HPP_
