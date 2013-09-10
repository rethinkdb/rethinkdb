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

class symgen_t {
public:
    symgen_t() : next_implicit_gensym_val(-1), next_non_implicit_gensym_val(-1) { }

    // Returns a globally unique variable.
    sym_t gensym();
    sym_t gensym_allowing_implicit();

    static bool var_allows_implicit(sym_t varnum);
private:
    sym_t do_gensym(int64_t *counter, int64_t offset);

    // All gensyms have to be inside [-2^53, 2^53], because we store them in doubles
    // in protobuf func objects.

    // The minimum gensym value that is usable as an implicit variable (if it's the
    // only one in its function's arg list).  This includes _positive_ symbol values!
    static const int64_t MIN_IMPLICIT_GENSYM = -(1LL << 28);

    // We limit gensyms to a small proportion of the values, leaving plenty of room for
    // backwards-compatible growth in the set of possible gensym types.
    static const int64_t MIN_RAW_GENSYM = -(1LL << 24);

    // Both these are between -1 and MIN_RAW_GENSYM_VALUE.
    int64_t next_implicit_gensym_val; // always negative
    int64_t next_non_implicit_gensym_val;
    DISABLE_COPYING(symgen_t);
};

}  // namespace ql


#endif  // RDB_PROTOCOL_SYM_HPP_
