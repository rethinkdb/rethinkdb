#include "rdb_protocol/sym.hpp"

#include "containers/archive/archive.hpp"

namespace ql {

RDB_IMPL_SERIALIZABLE_1(sym_t, value);

void debug_print(printf_buffer_t *buf, sym_t sym) {
    buf->appendf("var_%" PRIi64 "", sym.value);
}

}  // namespace ql
