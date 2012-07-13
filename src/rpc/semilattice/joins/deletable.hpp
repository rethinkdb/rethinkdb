#ifndef RPC_SEMILATTICE_JOINS_DELETABLE_HPP_
#define RPC_SEMILATTICE_JOINS_DELETABLE_HPP_

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
    typedef T value_t;

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
        res.deleted = true;
        return res;
    }

    void mark_deleted() {
        deleted = true;
        t = T();
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

#endif /* RPC_SEMILATTICE_JOINS_DELETABLE_HPP_ */
