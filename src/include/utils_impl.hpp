
#ifndef __UTILS_IMPL_HPP__
#define __UTILS_IMPL_HPP__

template <typename T>
T* _gnew() {
    T *ptr = NULL;
    int res = posix_memalign((void**)&ptr, 64, sizeof(T));
    if(res != 0)
        return NULL;
    else
        return ptr;
}

template <typename T>
T* gnew() {
    T *ptr = _gnew<T>();
    ptr = new (ptr) T();
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

#endif // __UTILS_IMPL_HPP__

