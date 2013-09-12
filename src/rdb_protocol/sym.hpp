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

    // Returns true if the varnum was non-negative, unlike dummy variables used by
    // internally generated reql functions.
    static bool var_allows_implicit(sym_t varnum);
};

inline bool operator<(sym_t x, sym_t y) {
    return x.value < y.value;
}

RDB_DECLARE_SERIALIZABLE(sym_t);

void debug_print(printf_buffer_t *buf, sym_t sym);

}  // namespace ql


#endif  // RDB_PROTOCOL_SYM_HPP_
