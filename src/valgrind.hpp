#ifndef VALGRIND_HPP_
#define VALGRIND_HPP_

#ifdef VALGRIND
#include <valgrind/memcheck.h>
#endif

template <class T>
T valgrind_undefined(T value) {
#ifdef VALGRIND
    UNUSED auto x = VALGRIND_MAKE_MEM_UNDEFINED(&value, sizeof(value));
#endif
    return value;
}

#endif  // VALGRIND_HPP_
