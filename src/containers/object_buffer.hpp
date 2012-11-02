// Copyright 2010-2012 RethinkDB, all rights reserved.
#ifndef CONTAINERS_OBJECT_BUFFER_HPP_
#define CONTAINERS_OBJECT_BUFFER_HPP_

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

#define OBJECT_BUFFER_CREATE_INTERNAL(...) do {     \
        rassert(state == EMPTY);                    \
        state = CONSTRUCTING;                       \
        new (&object_data[0]) T(__VA_ARGS__);       \
        state = INSTANTIATED;                       \
        return get();                               \
    } while (0)

    // 9 arguments ought to be enough for anybody
    T * create()
    { OBJECT_BUFFER_CREATE_INTERNAL(); }

    template <class arg1_t>
    T * create(const arg1_t &arg1)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1); }

    template <class arg1_t, class arg2_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2); }

    template <class arg1_t, class arg2_t, class arg3_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2, const arg3_t &arg3)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2, arg3); }

    template <class arg1_t, class arg2_t, class arg3_t, class arg4_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2, const arg3_t &arg3, const arg4_t &arg4)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2, arg3, arg4); }

    template <class arg1_t, class arg2_t, class arg3_t, class arg4_t, class arg5_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2, const arg3_t &arg3, const arg4_t &arg4, const arg5_t &arg5)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2, arg3, arg4, arg5); }

    template <class arg1_t, class arg2_t, class arg3_t, class arg4_t, class arg5_t, class arg6_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2, const arg3_t &arg3, const arg4_t &arg4, const arg5_t &arg5, const arg6_t &arg6)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2, arg3, arg4, arg5, arg6); }

    template <class arg1_t, class arg2_t, class arg3_t, class arg4_t, class arg5_t, class arg6_t, class arg7_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2, const arg3_t &arg3, const arg4_t &arg4, const arg5_t &arg5, const arg6_t &arg6, const arg7_t &arg7)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2, arg3, arg4, arg5, arg6, arg7); }

    template <class arg1_t, class arg2_t, class arg3_t, class arg4_t, class arg5_t, class arg6_t, class arg7_t, class arg8_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2, const arg3_t &arg3, const arg4_t &arg4, const arg5_t &arg5, const arg6_t &arg6, const arg7_t &arg7, const arg8_t &arg8)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); }

    template <class arg1_t, class arg2_t, class arg3_t, class arg4_t, class arg5_t, class arg6_t, class arg7_t, class arg8_t, class arg9_t>
    T * create(const arg1_t &arg1, const arg2_t &arg2, const arg3_t &arg3, const arg4_t &arg4, const arg5_t &arg5, const arg6_t &arg6, const arg7_t &arg7, const arg8_t &arg8, const arg9_t &arg9)
    { OBJECT_BUFFER_CREATE_INTERNAL(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9); }

    T * get() {
        rassert(state == INSTANTIATED);
        return reinterpret_cast<T *>(&object_data[0]);
    }

    T * operator->() {
        return get();
    }

    const T * get() const {
        rassert(state == INSTANTIATED);
        return reinterpret_cast<const T *>(&object_data[0]);
    }

    void reset() {
        T *obj_ptr = get();
        state = DESTRUCTING;
        obj_ptr->~T();
        state = EMPTY;
    }

    bool has() const {
        return (state == INSTANTIATED);
    }

private:
    // We're going more for a high probability of good alignment than
    // proof of good alignment.
    uint8_t object_data[sizeof(T)];

    enum buffer_state_t {
        EMPTY,
        CONSTRUCTING,
        INSTANTIATED,
        DESTRUCTING
    } state;

    DISABLE_COPYING(object_buffer_t);
};

#endif  // CONTAINERS_OBJECT_BUFFER_HPP_
