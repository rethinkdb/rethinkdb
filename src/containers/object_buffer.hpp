#ifndef CONTAINERS_OBJECT_BUFFER_HPP_
#define CONTAINERS_OBJECT_BUFFER_HPP_

#include "errors.hpp"

template <class T>
class object_buffer_t {
public:
    // TODO: this object makes no guarantees that its parent still exists when it is destroyed
    //  is that ok?
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
    };

    object_buffer_t() : instantiated(false) { }
    ~object_buffer_t() {
        if (instantiated) { reset(); }
    }

#define OBJECT_BUFFER_CREATE_INTERNAL(...) do {                         \
        rassert(!instantiated);                                         \
        instantiated = true;                                            \
        return new (&object_data[0]) T(__VA_ARGS__);                    \
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
        rassert(instantiated);
        return reinterpret_cast<T *>(&object_data[0]);
    }

    const T * get() const {
        rassert(instantiated);
        return reinterpret_cast<const T *>(&object_data[0]);
    }

    void reset() {
        rassert(instantiated);
        get()->~T();
        instantiated = false;
    }

    bool has() const {
        return instantiated;
    }

private:
    bool instantiated;
    uint8_t object_data[sizeof(T)];
};

#endif  // CONTAINERS_OBJECT_BUFFER_HPP_
