
#ifndef __RETEST_HPP__
#define __RETEST_HPP__

#include <iostream>
#include <string.h>
#include <stdlib.h>

// Generic assertion
template<typename T>
void assert_impl(bool cond, const char* op, T x, T y, const char *file, unsigned int line) {
    using namespace std;
    if(!cond) {
        const char *real_file = file + 1;
        cout << endl << "FAILED: " << x << " " << op << " " << y
             << " (" << real_file << ":" << line << ")"
             << endl;
        exit(-1);
    }
}

// User assertion
#define assert(C)  assert_impl(C, __FILE__, __LINE__)
void assert_impl(bool cond, const char *file, unsigned int line) {
    using namespace std;
    if(!cond) {
        cout << endl;
        exit(-1);
    }
}


// Equality
#define assert_eq(X, Y)  assert_eq_impl(X, Y, __FILE__, __LINE__)

template<typename T>
void assert_eq_impl(T x, T y, const char *file, unsigned int line) {
    assert_impl(x == y, "==", x, y, file, line);
}
template<>
void assert_eq_impl(const char* x, const char* y, const char *file, unsigned int line) {
    assert_impl(strcmp(x, y) == 0, "==", x, y, file, line);
}

// Inequality
#define assert_ne(X, Y)  assert_ne_impl(X, Y, __FILE__, __LINE__)

template<typename T>
void assert_ne_impl(T x, T y, const char *file, unsigned int line) {
    assert_impl(x != y, "!=", x, y, file, line);
}
template<>
void assert_ne_impl(const char* x, const char* y, const char *file, unsigned int line) {
    assert_impl(strcmp(x, y) != 0, "!=", x, y, file, line);
}

// Less than
#define assert_lt(X, Y)  assert_lt_impl(X, Y, __FILE__, __LINE__)

template<typename T>
void assert_lt_impl(T x, T y, const char *file, unsigned int line) {
    assert_impl(x < y, "<", x, y, file, line);
}
template<>
void assert_lt_impl(const char* x, const char* y, const char *file, unsigned int line) {
    assert_impl(strcmp(x, y) < 0, "<", x, y, file, line);
}

// Less than or equal
#define assert_lte(X, Y)  assert_lte_impl(X, Y, __FILE__, __LINE__)

template<typename T>
void assert_lte_impl(T x, T y, const char *file, unsigned int line) {
    assert_impl(x <= y, "<=", x, y, file, line);
}
template<>
void assert_lte_impl(const char* x, const char* y, const char *file, unsigned int line) {
    assert_impl(strcmp(x, y) <= 0, "<=", x, y, file, line);
}

// Greater than
#define assert_gt(X, Y)  assert_gt_impl(X, Y, __FILE__, __LINE__)

template<typename T>
void assert_gt_impl(T x, T y, const char *file, unsigned int line) {
    assert_impl(x > y, ">", x, y, file, line);
}
template<>
void assert_gt_impl(const char* x, const char* y, const char *file, unsigned int line) {
    assert_impl(strcmp(x, y) > 0, ">", x, y, file, line);
}

// Greater than or equal
#define assert_gte(X, Y)  assert_eq_impl(X, Y, __FILE__, __LINE__)

template<typename T>
void assert_gte_impl(T x, T y, const char *file, unsigned int line) {
    assert_impl(x >= y, ">=", x, y, file, line);
}
template<>
void assert_gte_impl(const char* x, const char* y, const char *file, unsigned int line) {
    assert_impl(strcmp(x, y) >= 0, ">=", x, y, file, line);
}

#endif // __RETEST_HPP__

