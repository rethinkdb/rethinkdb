
#ifndef __UTILS_TCC__
#define __UTILS_TCC__

#include <cstring>
#include <string>
#include <limits>

void *_gmalloc(size_t size);
void _gfree(void *ptr);

template <typename T>
T* _gnew() {
    return (T*)_gmalloc(sizeof(T));
}

template <typename T>
T* gnew() {
    T *ptr = _gnew<T>();
    return new (ptr) T();
}

template <typename T, typename A1>
T* gnew(A1 a1) {
    T *ptr = _gnew<T>();
    return new (ptr) T(a1);
}

template <typename T, typename A1, typename A2>
T* gnew(A1 a1, A2 a2) {
    T *ptr = _gnew<T>();
    return new (ptr) T(a1, a2);
}

template <typename T, typename A1, typename A2, typename A3>
T* gnew(A1 a1, A2 a2, A3 a3) {
    T *ptr = _gnew<T>();
    return new (ptr) T(a1, a2, a3);
}

template <typename T, typename A1, typename A2, typename A3, typename A4>
T* gnew(A1 a1, A2 a2, A3 a3, A4 a4) {
    T *ptr = _gnew<T>();
    return new (ptr) T(a1, a2, a3, a4);
}

template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5>
T* gnew(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    T *ptr = _gnew<T>();
    return new (ptr) T(a1, a2, a3, a4, a5);
}

template <typename T, typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
T* gnew(A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6) {
    T *ptr = _gnew<T>();
    return new (ptr) T(a1, a2, a3, a4, a5, a6);
}

// A custom STL allocator for calling gnew
template<typename T>
class gnew_alloc {
public: 
    // typedefs
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

public:
    // convert an allocator<T> to allocator<U>
    template<typename U>
    struct rebind {
        typedef gnew_alloc<U> other;
    };

public: 
    inline gnew_alloc() {}

    inline gnew_alloc(const gnew_alloc&) {}
    template<typename U>
    
    inline gnew_alloc(const gnew_alloc<U>&) {}
    
    inline ~gnew_alloc() {}

    // address
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) const { return &r; }

    // memory allocation
    inline pointer allocate(size_type cnt, const void* = 0)
    {
        return reinterpret_cast<pointer>(_gmalloc(sizeof(T) * cnt)); 
    }
    inline void deallocate(pointer p, size_type) { 
        _gfree(p); 
    }

    // size
    inline size_type max_size() const { 
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    // construction/destruction
    inline void construct(pointer p, const T& t) { new(p) T(t); }
    inline void destroy(pointer p) { p->~T(); }
};


template<typename _Tp>
inline bool operator==(const gnew_alloc<_Tp>&, const gnew_alloc<_Tp>&)
{ return true; }
  
template<typename _Tp>
inline bool operator!=(const gnew_alloc<_Tp>&, const gnew_alloc<_Tp>&)
{ return false; }



#endif // __UTILS_TCC__

