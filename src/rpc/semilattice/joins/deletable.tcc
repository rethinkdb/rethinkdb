#ifndef RPC_SEMILATTICE_JOINS_DELETABLE_WRAPPER_TCC_
#define RPC_SEMILATTICE_JOINS_DELETABLE_WRAPPER_TCC_

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
T &deletable_getter(deletable_t<T> &target) {
    return target.get_mutable();
}

#endif
