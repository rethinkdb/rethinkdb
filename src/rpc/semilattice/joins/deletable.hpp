#ifndef RPC_SEMILATTICE_JOINS_DELETABLE_HPP_
#define RPC_SEMILATTICE_JOINS_DELETABLE_HPP_

#include "containers/archive/boost_types.hpp"
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
public:
    deletable_t() : t(T()) { }
    explicit deletable_t(const T &_t) : t(_t) { }

    bool is_deleted() const { return !t; }
    void mark_deleted() { t = boost::optional<T>(); }
    /* return an object which when joined in will cause the object to be deleted */
    deletable_t get_deletion() const {
        deletable_t<T> res;
        res.mark_deleted();
        return res;
    }

    /* Usage: [get] is the normal case, [get_ref] is for efficiency, and
       [get_mutable] is for when you need to modify something. */
    const T &get_ref() const { guarantee(t); return  *t; }
          T  get() const     { guarantee(t); return  *t; }
          T *get_mutable()   { guarantee(t); return &*t; }

    typedef T value_t;
    boost::optional<T> t;
    RDB_MAKE_ME_SERIALIZABLE_1(t)
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
