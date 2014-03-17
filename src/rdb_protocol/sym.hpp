// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef RDB_PROTOCOL_SYM_HPP_
#define RDB_PROTOCOL_SYM_HPP_

#include <stdint.h>

#include <vector>

#include "containers/archive/archive.hpp"
#include "rpc/serialize_macros.hpp"
#include "valgrind.hpp"

namespace ql {

class sym_t {
public:
    int64_t value;

    sym_t() : value(valgrind_undefined<int64_t>(0)) { }
    explicit sym_t(int64_t _value) : value(_value) { }
};

bool function_emits_implicit_variable(const std::vector<sym_t> &arg_names);

inline bool operator<(sym_t x, sym_t y) {
    return x.value < y.value;
}

RDB_DECLARE_SERIALIZABLE(sym_t);

void debug_print(printf_buffer_t *buf, sym_t sym);

}  // namespace ql


#endif  // RDB_PROTOCOL_SYM_HPP_
