// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_SYM_HPP_
#define RDB_PROTOCOL_SYM_HPP_

#include <stdint.h>

#include "rpc/serialize_macros.hpp"
#include "utils.hpp"

namespace ql {

class sym_t {
public:
    int64_t value;

    sym_t() : value(valgrind_undefined<int64_t>(0)) { }
    explicit sym_t(int64_t _value) : value(_value) { }
};

inline bool operator<(sym_t x, sym_t y) {
    return x.value < y.value;
}

RDB_DECLARE_SERIALIZABLE(sym_t);

void debug_print(printf_buffer_t *buf, sym_t sym);

inline sym_t GENSYM_A() { return sym_t(-1); }
inline sym_t GENSYM_B() { return sym_t(-2); }
inline sym_t GENSYM_C() { return sym_t(-3); }

class symgen_t {
public:
    symgen_t() : next_gensym_val(-4) { }

    // Returns a globally unique variable (with respect to the environment).
    sym_t gensym();

    static bool var_allows_implicit(sym_t varnum);
private:
    // All gensyms have to be inside [-2^53, 2^53], because we store them in doubles
    // in protobuf func objects.

    // We limit gensyms to a small proportion of the values, leaving plenty of room for
    // backwards-compatible growth in the set of possible _different_ sym types.  Right
    // now there are two types: those that can be referenced by an implicit variable
    // (in appropriate situations), which are positive (and client-supplied), and those
    // which cannot.
    static const int64_t MIN_RAW_GENSYM = -(1LL << 24);

    // Both these are between -1 and MIN_RAW_GENSYM.
    int64_t next_gensym_val;  // always negative
    DISABLE_COPYING(symgen_t);
};

}  // namespace ql


#endif  // RDB_PROTOCOL_SYM_HPP_
