// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/sym.hpp"

#include "containers/archive/archive.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

RDB_IMPL_SERIALIZABLE_1(sym_t, value);

void debug_print(printf_buffer_t *buf, sym_t sym) {
    buf->appendf("var_%" PRIi64 "", sym.value);
}

sym_t symgen_t::do_gensym(int64_t *next_value, int64_t offset) {
    r_sanity_check(0 > *next_value && *next_value >= MIN_RAW_GENSYM);
    const int64_t ret = *next_value + offset;
    --*next_value;
    return sym_t(ret);
}

sym_t symgen_t::gensym() {
    return do_gensym(&next_non_implicit_gensym_val, MIN_IMPLICIT_GENSYM);
}

sym_t symgen_t::gensym_allowing_implicit() {
    return do_gensym(&next_implicit_gensym_val, 0);
}

bool symgen_t::var_allows_implicit(sym_t varnum) {
    return varnum.value >= MIN_IMPLICIT_GENSYM;
}


}  // namespace ql
