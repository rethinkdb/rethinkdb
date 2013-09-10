// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "rdb_protocol/sym.hpp"

#include "containers/archive/archive.hpp"
#include "rdb_protocol/error.hpp"

namespace ql {

RDB_IMPL_SERIALIZABLE_1(sym_t, value);

void debug_print(printf_buffer_t *buf, sym_t sym) {
    buf->appendf("var_%" PRIi64 "", sym.value);
}

sym_t symgen_t::gensym(bool allow_implicit) {
    if (allow_implicit) {
        r_sanity_check(0 > next_implicit_gensym_val
                       && next_implicit_gensym_val >= MIN_RAW_GENSYM_VALUE);
        const int64_t ret = next_implicit_gensym_val;
        --next_implicit_gensym_val;
        return sym_t(ret);
    } else {
        r_sanity_check(0 > next_non_implicit_gensym_val
                       && next_non_implicit_gensym_val >= MIN_RAW_GENSYM_VALUE);
        const int64_t ret = next_non_implicit_gensym_val + MIN_IMPLICIT_GENSYM;
        --next_non_implicit_gensym_val;
        return sym_t(ret);
    }
}

bool symgen_t::var_allows_implicit(sym_t varnum) {
    return varnum.value >= MIN_IMPLICIT_GENSYM;
}


}  // namespace ql
