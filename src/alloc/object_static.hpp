
#ifndef __OBJECT_STATIC_ALLOC_HPP__
#define __OBJECT_STATIC_ALLOC_HPP__

#include <new>

// Implementation helpers
enum null_0_t {};
enum null_1_t {};
enum null_2_t {};
enum null_3_t {};
enum null_4_t {};
enum null_5_t {};
enum null_6_t {};
enum null_7_t {};
enum null_8_t {};
enum null_9_t {};

/* The implementation macro */
#define GEN_ALLOC_IMPL(T, P)  \
public:                       \
    void free(T *ptr) {       \
        ptr->~T();            \
        P.free(ptr);          \
    }                         \
                              \
private:                      \
    void malloc(T **ptr) {    \
        *ptr = (T*)P.         \
          malloc(sizeof(T));  \
    }                         \
                              \
    super_alloc_t P;
/* End of the implementation macro */

/* The implementation macro */
#define GEN_WRAPPER                   \
public:                               \
    template <typename U>             \
    U* malloc() {                     \
        U *ptr = NULL;                \
        malloc(&ptr);                 \
        return new ((void*)ptr) U();  \
    }                                 \
                                      \
    template <typename U, typename A0T>             \
    U* malloc(A0T a0) {                             \
        U *ptr = NULL;                              \
        malloc(&ptr);                               \
        return new ((void*)ptr) U(a0);              \
    }                                               \
                                                    \
    template <typename U, typename A0T,             \
              typename A1T>                         \
    U* malloc(A0T a0, A1T a1) {                     \
        U *ptr = NULL;                              \
        malloc(&ptr);                               \
        return new ((void*)ptr) U(a0, a1);          \
    }                                               \
                                                    \
    template <typename U, typename A0T,             \
              typename A1T, typename A2T>           \
    U* malloc(A0T a0, A1T a1, A2T a2) {             \
        U *ptr = NULL;                              \
        malloc(&ptr);                               \
        return new ((void*)ptr) U(a0, a1, a2);      \
    }                                               \
                                                    \
    template <typename U, typename A0T,             \
              typename A1T, typename A2T,           \
              typename A3T>                         \
    U* malloc(A0T a0, A1T a1, A2T a2, A3T a3) {     \
        U *ptr = NULL;                              \
        malloc(&ptr);                               \
        return new ((void*)ptr) U(a0, a1, a2, a3);  \
    }                                               \
                                                    \
    template <typename U, typename A0T,             \
              typename A1T, typename A2T,           \
              typename A3T, typename A4T>           \
    U* malloc(A0T a0, A1T a1, A2T a2, A3T a3,       \
              A4T a4) {                             \
        U *ptr = NULL;                              \
        malloc(&ptr);                               \
        return new ((void*)ptr) U(a0, a1, a2, a3,   \
                                  a4);              \
    }

/* End of the implementation macro */

// Generic allocator (10 allocators)
template <class super_alloc_t,
          typename T0 = null_0_t, typename T1 = null_1_t,
          typename T2 = null_2_t, typename T3 = null_3_t,
          typename T4 = null_4_t, typename T5 = null_5_t,
          typename T6 = null_6_t, typename T7 = null_7_t,
          typename T8 = null_8_t, typename T9 = null_9_t>
struct object_static_alloc_t
{
    // TODO: implement
};

// Specialization with 0 allocators
template <class super_alloc_t>
struct object_static_alloc_t<super_alloc_t,
                             null_0_t, null_1_t,
                             null_2_t, null_3_t,
                             null_4_t, null_5_t,
                             null_6_t, null_7_t,
                             null_8_t, null_9_t>
{
};

// Specialization with 1 allocator
template <class super_alloc_t, typename T0>
struct object_static_alloc_t<super_alloc_t,
                             T0, null_1_t,
                             null_2_t, null_3_t,
                             null_4_t, null_5_t,
                             null_6_t, null_7_t,
                             null_8_t, null_9_t>
{
public:
    object_static_alloc_t() :
        _p0(sizeof(T0))
        {}

    void gc() {
        _p0.gc();
    }

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
};

// Specialization with 2 allocators
template <class super_alloc_t, typename T0, typename T1>
struct object_static_alloc_t<super_alloc_t,
                             T0, T1,
                             null_2_t, null_3_t,
                             null_4_t, null_5_t,
                             null_6_t, null_7_t,
                             null_8_t, null_9_t>
{
public:
    object_static_alloc_t() :
        _p0(sizeof(T0)),
        _p1(sizeof(T1))
        {}

    void gc() {
        _p0.gc();
        _p1.gc();
    }

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
    GEN_ALLOC_IMPL(T1, _p1)
};

// Specialization with 3 allocators
template <class super_alloc_t, typename T0, typename T1, typename T2>
struct object_static_alloc_t<super_alloc_t,
                             T0, T1,
                             T2, null_3_t,
                             null_4_t, null_5_t,
                             null_6_t, null_7_t,
                             null_8_t, null_9_t>
{
public:
    object_static_alloc_t() :
        _p0(sizeof(T0)),
        _p1(sizeof(T1)),
        _p2(sizeof(T2))
        {}

    void gc() {
        _p0.gc();
        _p1.gc();
        _p2.gc();
    }

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
    GEN_ALLOC_IMPL(T1, _p1)
    GEN_ALLOC_IMPL(T2, _p2)
};

// Specialization with 4 allocators
template <class super_alloc_t, typename T0, typename T1, typename T2, typename T3>
struct object_static_alloc_t<super_alloc_t,
                             T0, T1,
                             T2, T3,
                             null_4_t, null_5_t,
                             null_6_t, null_7_t,
                             null_8_t, null_9_t>
{
public:
    object_static_alloc_t() :
        _p0(sizeof(T0)),
        _p1(sizeof(T1)),
        _p2(sizeof(T2)),
        _p3(sizeof(T3))
        {}

    void gc() {
        _p0.gc();
        _p1.gc();
        _p2.gc();
        _p3.gc();
    }

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
    GEN_ALLOC_IMPL(T1, _p1)
    GEN_ALLOC_IMPL(T2, _p2)
    GEN_ALLOC_IMPL(T3, _p3)
};

// Specialization with 5 allocators
template <class super_alloc_t, typename T0, typename T1, typename T2, typename T3, typename T4>
struct object_static_alloc_t<super_alloc_t,
                             T0, T1,
                             T2, T3,
                             T4, null_5_t,
                             null_6_t, null_7_t,
                             null_8_t, null_9_t>
{
public:
    object_static_alloc_t() :
        _p0(sizeof(T0)),
        _p1(sizeof(T1)),
        _p2(sizeof(T2)),
        _p3(sizeof(T3)),
        _p4(sizeof(T4))
        {}

    void gc() {
        _p0.gc();
        _p1.gc();
        _p2.gc();
        _p3.gc();
        _p4.gc();
    }

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
    GEN_ALLOC_IMPL(T1, _p1)
    GEN_ALLOC_IMPL(T2, _p2)
    GEN_ALLOC_IMPL(T3, _p3)
    GEN_ALLOC_IMPL(T4, _p4)
};

// Specialization with 6 allocators
template <class super_alloc_t,
          typename T0, typename T1, typename T2, typename T3, 
          typename T4, typename T5>
struct object_static_alloc_t<super_alloc_t,
                             T0, T1,
                             T2, T3,
                             T4, T5,
                             null_6_t, null_7_t,
                             null_8_t, null_9_t>
{
public:
    object_static_alloc_t() :
        _p0(sizeof(T0)),
        _p1(sizeof(T1)),
        _p2(sizeof(T2)),
        _p3(sizeof(T3)),
        _p4(sizeof(T4)),
        _p5(sizeof(T5))
        {}

    void gc() {
        _p0.gc();
        _p1.gc();
        _p2.gc();
        _p3.gc();
        _p4.gc();
        _p5.gc();
    }

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
    GEN_ALLOC_IMPL(T1, _p1)
    GEN_ALLOC_IMPL(T2, _p2)
    GEN_ALLOC_IMPL(T3, _p3)
    GEN_ALLOC_IMPL(T4, _p4)
    GEN_ALLOC_IMPL(T5, _p5)
};

// Specialization with 7 allocators
template <class super_alloc_t,
          typename T0, typename T1, typename T2, typename T3, 
          typename T4, typename T5, typename T6>
struct object_static_alloc_t<super_alloc_t,
                             T0, T1,
                             T2, T3,
                             T4, T5,
                             T6, null_7_t,
                             null_8_t, null_9_t>
{
public:
    object_static_alloc_t() :
        _p0(sizeof(T0)),
        _p1(sizeof(T1)),
        _p2(sizeof(T2)),
        _p3(sizeof(T3)),
        _p4(sizeof(T4)),
        _p5(sizeof(T5)),
        _p6(sizeof(T6))
        {}

    void gc() {
        _p0.gc();
        _p1.gc();
        _p2.gc();
        _p3.gc();
        _p4.gc();
        _p5.gc();
        _p6.gc();
    }

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
    GEN_ALLOC_IMPL(T1, _p1)
    GEN_ALLOC_IMPL(T2, _p2)
    GEN_ALLOC_IMPL(T3, _p3)
    GEN_ALLOC_IMPL(T4, _p4)
    GEN_ALLOC_IMPL(T5, _p5)
    GEN_ALLOC_IMPL(T6, _p6)
};

// Adapter from object_static_alloc_t to a standard allocator (for test code uniformity)
template<typename super_alloc_t, typename T>
struct object_static_adapter_t : public super_alloc_t {
    void* malloc(size_t size) {
        return (void*)super_alloc_t::template malloc<T>();
    }

    void free(void *ptr) {
        super_alloc_t::free((T*)ptr);
    }
};

#endif // __OBJECT_STATIC_ALLOC_HPP__

