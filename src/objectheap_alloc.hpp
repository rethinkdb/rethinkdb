
#ifndef __OBJECTHEAP_ALLOC_HPP__
#define __OBJECTHEAP_ALLOC_HPP__

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
#define GEN_WRAPPER           \
public:                       \
    template<typename U>      \
    U* malloc() {             \
        U *ptr = NULL;        \
        malloc(&ptr);         \
        return ptr;           \
    }
/* End of the implementation macro */

// Generic allocator (10 allocators)
template <class super_alloc_t,
          typename T0 = null_0_t, typename T1 = null_1_t,
          typename T2 = null_2_t, typename T3 = null_3_t,
          typename T4 = null_4_t, typename T5 = null_5_t,
          typename T6 = null_6_t, typename T7 = null_7_t,
          typename T8 = null_8_t, typename T9 = null_9_t>
struct objectheap_alloc_t
{
    // TODO: implement
};

// Specialization with 0 allocators
template <class super_alloc_t>
struct objectheap_alloc_t<super_alloc_t,
                          null_0_t, null_1_t,
                          null_2_t, null_3_t,
                          null_4_t, null_5_t,
                          null_6_t, null_7_t,
                          null_8_t, null_9_t>
{};

// Specialization with 1 allocator
template <class super_alloc_t, typename T0>
struct objectheap_alloc_t<super_alloc_t,
                          T0, null_1_t,
                          null_2_t, null_3_t,
                          null_4_t, null_5_t,
                          null_6_t, null_7_t,
                          null_8_t, null_9_t>
{
public:
    objectheap_alloc_t() :
        _p0(sizeof(T0))
        {}

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
};

// Specialization with 2 allocators
template <class super_alloc_t, typename T0, typename T1>
struct objectheap_alloc_t<super_alloc_t,
                          T0, T1,
                          null_2_t, null_3_t,
                          null_4_t, null_5_t,
                          null_6_t, null_7_t,
                          null_8_t, null_9_t>
{
public:
    objectheap_alloc_t() :
        _p0(sizeof(T0)),
        _p1(sizeof(T1))
        {}

    GEN_WRAPPER
    GEN_ALLOC_IMPL(T0, _p0)
    GEN_ALLOC_IMPL(T1, _p1)
};

// Adapter from objectheap_alloc_t to a standard allocator (for test code uniformity)
template<typename super_alloc_t, typename T>
struct objectheap_adapter_t : public super_alloc_t {
    void* malloc(size_t size) {
        return (void*)super_alloc_t::template malloc<T>();
    }

    void free(void *ptr) {
        super_alloc_t::free((T*)ptr);
    }
};

#endif // __OBJECTHEAP_ALLOC_HPP__

