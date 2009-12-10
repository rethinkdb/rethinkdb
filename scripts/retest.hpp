
#ifndef __RETEST_HPP__
#define __RETEST_HPP__

#include <iostream>
#include <string.h>
#include <stdlib.h>

// Generic assertion
template<typename T>
void assert(bool cond, const char* op, T x, T y) {
    using namespace std;
    if(!cond) {
        cout << endl << "FAILED: " << x << " " << op << " " << y << endl;
        exit(-1);
    }
}

// User assertion
void assert(bool cond) {
    using namespace std;
    if(!cond) {
        cout << endl;
        exit(-1);
    }
}


// Equality
template<typename T>
void assert_eq(T x, T y) {
    assert(x == y, "==", x, y);
}
template<>
void assert_eq(const char* x, const char* y) {
    assert(strcmp(x, y) == 0, "==", x, y);
}

// Inequality
template<typename T>
void assert_ne(T x, T y) {
    assert(x != y, "!=", x, y);
}
template<>
void assert_ne(const char* x, const char* y) {
    assert(strcmp(x, y) != 0, "!=", x, y);
}

// Less than
template<typename T>
void assert_lt(T x, T y) {
    assert(x < y, "<", x, y);
}
template<>
void assert_lt(const char* x, const char* y) {
    assert(strcmp(x, y) < 0, "<", x, y);
}

// Less than or equal
template<typename T>
void assert_lte(T x, T y) {
    assert(x <= y, "<=", x, y);
}
template<>
void assert_lte(const char* x, const char* y) {
    assert(strcmp(x, y) <= 0, "<=", x, y);
}

// Greater than
template<typename T>
void assert_gt(T x, T y) {
    assert(x > y, ">", x, y);
}
template<>
void assert_gt(const char* x, const char* y) {
    assert(strcmp(x, y) > 0, ">", x, y);
}

// Greater than or equal
template<typename T>
void assert_gte(T x, T y) {
    assert(x >= y, ">=", x, y);
}
template<>
void assert_gte(const char* x, const char* y) {
    assert(strcmp(x, y) >= 0, ">=", x, y);
}

#endif // __RETEST_HPP__

