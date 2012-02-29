#ifndef __RPC_SEMILATTICE_JOINS_DELETABLE_WRAPPER_HPP__
#define __RPC_SEMILATTICE_JOINS_DELETABLE_WRAPPER_HPP__

#include "rpc/serialize_macros.hpp"

//a deletable wrapper allows a piece of metadata to be deleted (this makes up
//for the fact that we don't have inverses in semilattices)
template <class T>
class deletable_t {
private:
template <class TT>
friend bool operator==(const deletable_t<TT> &, const deletable_t<TT> &);

template <class TT>
friend void semilattice_join(deletable_t<TT> *, const deletable_t<TT> &);

    bool deleted;

public:
    bool is_deleted() const {
        return deleted;
    }

    T t;

    deletable_t() 
        : deleted(false), t()
    { }

    explicit deletable_t(T _t)
        : deleted(false), t(_t)
    { }

    RDB_MAKE_ME_SERIALIZABLE_2(t, deleted)

    /* return an object which when joined in will cause the object to be
     * deleted */
    deletable_t get_deletion() {
        deletable_t<T> res;
        res.deletion_state = true;
        return res;
    }

    T get() const {
        return t;
    }

    T &get_mutable() {
        return t;
    }
};

//semilattice concept for deletable_t
template <class T>
bool operator==(const deletable_t<T> &, const deletable_t<T> &);

template <class T>
void semilattice_join(deletable_t<T> *, const deletable_t<T> &);

#include "rpc/semilattice/joins/deletable.tcc"

#endif
