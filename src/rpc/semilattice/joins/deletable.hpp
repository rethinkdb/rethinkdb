#ifndef RPC_SEMILATTICE_JOINS_DELETABLE_HPP_
#define RPC_SEMILATTICE_JOINS_DELETABLE_HPP_

#include "rpc/serialize_macros.hpp"

class append_only_printf_buffer_t;


//a deletable wrapper allows a piece of metadata to be deleted (this makes up
//for the fact that we don't have inverses in semilattices)
template <class T>
class deletable_t {
private:
    template <class U>
    friend bool operator==(const deletable_t<U> &, const deletable_t<U> &);

    template <class U>
    friend void semilattice_join(deletable_t<U> *, const deletable_t<U> &);

    template <class U>
    friend void debug_print(append_only_printf_buffer_t *buf, const deletable_t<U> &x);

    bool deleted;

public:
    typedef T value_t;
    typedef T value_type;

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

template <class T>
void debug_print(append_only_printf_buffer_t *buf, const deletable_t<T> &x);

#include "rpc/semilattice/joins/deletable.tcc"

#endif /* RPC_SEMILATTICE_JOINS_DELETABLE_HPP_ */
