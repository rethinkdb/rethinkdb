#ifndef RPC_SEMILATTICE_JOINS_DELETABLE_TCC_
#define RPC_SEMILATTICE_JOINS_DELETABLE_TCC_

#include "rpc/semilattice/joins/deletable.hpp"

//semilattice concept for deletable_t
template <class T>
bool operator==(const deletable_t<T> &a, const deletable_t<T> &b) {
    return (a.deleted && b.deleted) || ((!a.deleted && !b.deleted) && (a.t == b.t));
}

template <class T>
void semilattice_join(deletable_t<T> *a, const deletable_t<T> &b) {
    if (b.is_deleted()) {
        *a = b;
    } else if (a->is_deleted()) {
        return;
    } else {
        semilattice_join(&a->t, b.t);
    }
}

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const deletable_t<T> &x) {
    buf->appendf("deletable{deleted=%s, t=", x.deleted ? "true" : "false");
    debug_print(buf, x.t);
    buf->appendf("}");
}


#endif  // RPC_SEMILATTICE_JOINS_DELETABLE_TCC_
