#ifndef __RPC_CORE_SEMILATTICE_SEMILATTICE_HPP__
#define __RPC_CORE_SEMILATTICE_SEMILATTICE_HPP__

/*
Objects implementing constructive_semilattice_t provide elements of a semilattice,
in the sense that a constructive join operation is defined between elements.
The sl_join() operation has to meet certain criteria, which are not enforced by
the type. Specifically, it must be associative, commutative and idempotent.

The name constructive_semilattice_t is chosen to distinguish this class from
a formally defined semilattice, which would be a set together with a partial
ordering over its elements and a special bottom (or top) element. In that definition,
the join operation itself does not necessarily have to be known (join(a, b)
could merely be defined as the smallest element that's larger than both a and b),
therefore such a semilattice would not necessarily be constructive.

The template parameter T is there for type safety, such that implementations
of this class can make sure that x in sl_join(x) to be of the correct type.
Usually, T should be of the type that implements constructive_semilattice_t.
If you want to handle type safety differently (or dynamically), feel free to use
T=void or something like that.
*/

template<typename T> class constructive_semilattice_t {
public:
    virtual ~constructive_semilattice_t();

    // Modifies this object by joining with x.
    // Must be associative, commutatice and idempotent
    virtual void sl_join(const T &x) = 0;
};

#endif // __RPC_CORE_SEMILATTICE_SEMILATTICE_HPP__
