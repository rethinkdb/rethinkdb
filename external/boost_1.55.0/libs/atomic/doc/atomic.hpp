/** \file boost/atomic.hpp */

//  Copyright (c) 2009 Helge Bahmann
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

/* this is just a pseudo-header file fed to doxygen
to more easily generate the class documentation; will
be replaced by proper documentation down the road */

namespace boost {

/**
    \brief Memory ordering constraints

    This defines the relative order of one atomic operation
    and other memory operations (loads, stores, other atomic operations)
    executed by the same thread.

    The order of operations specified by the programmer in the
    source code ("program order") does not necessarily match
    the order in which they are actually executed on the target system:
    Both compiler as well as processor may reorder operations
    quite arbitrarily. <B>Specifying the wrong ordering
    constraint will therefore generally result in an incorrect program.</B>
*/
enum memory_order {
    /**
        \brief No constraint
        Atomic operation and other memory operations may be reordered freely.
    */
    memory_order_relaxed,
    /**
        \brief Data dependence constraint
        Atomic operation must strictly precede any memory operation that
        computationally depends on the outcome of the atomic operation.
    */
    memory_order_consume,
    /**
        \brief Acquire memory
        Atomic operation must strictly precede all memory operations that
        follow in program order.
    */
    memory_order_acquire,
    /**
        \brief Release memory
        Atomic operation must strictly follow all memory operations that precede
        in program order.
    */
    memory_order_release,
    /**
        \brief Acquire and release memory
        Combines the effects of \ref memory_order_acquire and \ref memory_order_release
    */
    memory_order_acq_rel,
    /**
        \brief Sequentially consistent
        Produces the same result \ref memory_order_acq_rel, but additionally
        enforces globally sequential consistent execution
    */
    memory_order_seq_cst
};

/**
    \brief Atomic datatype

    An atomic variable. Provides methods to modify this variable atomically.
    Valid template parameters are:

    - integral data types (char, short, int, ...)
    - pointer data types
    - any other data type that has a non-throwing default
      constructor and that can be copied via <TT>memcpy</TT>

    Unless specified otherwise, any memory ordering constraint can be used
    with any of the atomic operations.
*/
template<typename Type>
class atomic {
public:
    /**
        \brief Create uninitialized atomic variable
        Creates an atomic variable. Its initial value is undefined.
    */
    atomic();
    /**
        \brief Create an initialize atomic variable
        \param value Initial value
        Creates and initializes an atomic variable.
    */
    atomic(Type value);

    /**
        \brief Read the current value of the atomic variable
        \param order Memory ordering constraint, see \ref memory_order
        \return Current value of the variable

        Valid memory ordering constraints are:
        - @c memory_order_relaxed
        - @c memory_order_consume
        - @c memory_order_acquire
        - @c memory_order_seq_cst
    */
    Type load(memory_order order=memory_order_seq_cst) const;

    /**
        \brief Write new value to atomic variable
        \param value New value
        \param order Memory ordering constraint, see \ref memory_order

        Valid memory ordering constraints are:
        - @c memory_order_relaxed
        - @c memory_order_release
        - @c memory_order_seq_cst
    */
    void store(Type value, memory_order order=memory_order_seq_cst);

    /**
        \brief Atomically compare and exchange variable
        \param expected Expected old value
        \param desired Desired new value
        \param order Memory ordering constraint, see \ref memory_order
        \return @c true if value was changed

        Atomically performs the following operation

        \code
        if (variable==expected) {
            variable=desired;
            return true;
        } else {
            expected=variable;
            return false;
        }
        \endcode

        This operation may fail "spuriously", i.e. the state of the variable
        is unchanged even though the expected value was found (this is the
        case on architectures using "load-linked"/"store conditional" to
        implement the operation).

        The established memory order will be @c order if the operation
        is successful. If the operation is unsuccessful, the
        memory order will be

        - @c memory_order_relaxed if @c order is @c memory_order_acquire ,
          @c memory_order_relaxed or @c memory_order_consume
        - @c memory_order_release if @c order is @c memory_order_acq_release
          or @c memory_order_release
        - @c memory_order_seq_cst if @c order is @c memory_order_seq_cst
    */
    bool compare_exchange_weak(
        Type &expected,
        Type desired,
        memory_order order=memory_order_seq_cst);

    /**
        \brief Atomically compare and exchange variable
        \param expected Expected old value
        \param desired Desired new value
        \param success_order Memory ordering constraint if operation
        is successful
        \param failure_order Memory ordering constraint if operation is unsuccessful
        \return @c true if value was changed

        Atomically performs the following operation

        \code
        if (variable==expected) {
            variable=desired;
            return true;
        } else {
            expected=variable;
            return false;
        }
        \endcode

        This operation may fail "spuriously", i.e. the state of the variable
        is unchanged even though the expected value was found (this is the
        case on architectures using "load-linked"/"store conditional" to
        implement the operation).

        The constraint imposed by @c success_order may not be
        weaker than the constraint imposed by @c failure_order.
    */
    bool compare_exchange_weak(
        Type &expected,
        Type desired,
        memory_order success_order,
        memory_order failure_order);
    /**
        \brief Atomically compare and exchange variable
        \param expected Expected old value
        \param desired Desired new value
        \param order Memory ordering constraint, see \ref memory_order
        \return @c true if value was changed

        Atomically performs the following operation

        \code
        if (variable==expected) {
            variable=desired;
            return true;
        } else {
            expected=variable;
            return false;
        }
        \endcode

        In contrast to \ref compare_exchange_weak, this operation will never
        fail spuriously. Since compare-and-swap must generally be retried
        in a loop, implementors are advised to prefer \ref compare_exchange_weak
        where feasible.

        The established memory order will be @c order if the operation
        is successful. If the operation is unsuccessful, the
        memory order will be

        - @c memory_order_relaxed if @c order is @c memory_order_acquire ,
          @c memory_order_relaxed or @c memory_order_consume
        - @c memory_order_release if @c order is @c memory_order_acq_release
          or @c memory_order_release
        - @c memory_order_seq_cst if @c order is @c memory_order_seq_cst
    */
    bool compare_exchange_strong(
        Type &expected,
        Type desired,
        memory_order order=memory_order_seq_cst);

    /**
        \brief Atomically compare and exchange variable
        \param expected Expected old value
        \param desired Desired new value
        \param success_order Memory ordering constraint if operation
        is successful
        \param failure_order Memory ordering constraint if operation is unsuccessful
        \return @c true if value was changed

        Atomically performs the following operation

        \code
        if (variable==expected) {
            variable=desired;
            return true;
        } else {
            expected=variable;
            return false;
        }
        \endcode

        In contrast to \ref compare_exchange_weak, this operation will never
        fail spuriously. Since compare-and-swap must generally be retried
        in a loop, implementors are advised to prefer \ref compare_exchange_weak
        where feasible.

        The constraint imposed by @c success_order may not be
        weaker than the constraint imposed by @c failure_order.
    */
    bool compare_exchange_strong(
        Type &expected,
        Type desired,
        memory_order success_order,
        memory_order failure_order);
    /**
        \brief Atomically exchange variable
        \param value New value
        \param order Memory ordering constraint, see \ref memory_order
        \return Old value of the variable

        Atomically exchanges the value of the variable with the new
        value and returns its old value.
    */
    Type exchange(Type value, memory_order order=memory_order_seq_cst);

    /**
        \brief Atomically add and return old value
        \param operand Operand
        \param order Memory ordering constraint, see \ref memory_order
        \return Old value of the variable

        Atomically adds operand to the variable and returns its
        old value.
    */
    Type fetch_add(Type operand, memory_order order=memory_order_seq_cst);
    /**
        \brief Atomically subtract and return old value
        \param operand Operand
        \param order Memory ordering constraint, see \ref memory_order
        \return Old value of the variable

        Atomically subtracts operand from the variable and returns its
        old value.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        @c operand is of type @c ptrdiff_t and the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type fetch_sub(Type operand, memory_order order=memory_order_seq_cst);

    /**
        \brief Atomically perform bitwise "AND" and return old value
        \param operand Operand
        \param order Memory ordering constraint, see \ref memory_order
        \return Old value of the variable

        Atomically performs bitwise "AND" with the variable and returns its
        old value.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        @c operand is of type @c ptrdiff_t and the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type fetch_and(Type operand, memory_order order=memory_order_seq_cst);

    /**
        \brief Atomically perform bitwise "OR" and return old value
        \param operand Operand
        \param order Memory ordering constraint, see \ref memory_order
        \return Old value of the variable

        Atomically performs bitwise "OR" with the variable and returns its
        old value.

        This method is available only if \c Type is an integral type.
    */
    Type fetch_or(Type operand, memory_order order=memory_order_seq_cst);

    /**
        \brief Atomically perform bitwise "XOR" and return old value
        \param operand Operand
        \param order Memory ordering constraint, see \ref memory_order
        \return Old value of the variable

        Atomically performs bitwise "XOR" with the variable and returns its
        old value.

        This method is available only if \c Type is an integral type.
    */
    Type fetch_xor(Type operand, memory_order order=memory_order_seq_cst);

    /**
        \brief Implicit load
        \return Current value of the variable

        The same as <tt>load(memory_order_seq_cst)</tt>. Avoid using
        the implicit conversion operator, use \ref load with
        an explicit memory ordering constraint.
    */
    operator Type(void) const;
    /**
        \brief Implicit store
        \param value New value
        \return Copy of @c value

        The same as <tt>store(value, memory_order_seq_cst)</tt>. Avoid using
        the implicit conversion operator, use \ref store with
        an explicit memory ordering constraint.
    */
    Type operator=(Type v);

    /**
        \brief Atomically perform bitwise "AND" and return new value
        \param operand Operand
        \return New value of the variable

        The same as <tt>fetch_and(operand, memory_order_seq_cst)&operand</tt>.
        Avoid using the implicit bitwise "AND" operator, use \ref fetch_and
        with an explicit memory ordering constraint.
    */
    Type operator&=(Type operand);

    /**
        \brief Atomically perform bitwise "OR" and return new value
        \param operand Operand
        \return New value of the variable

        The same as <tt>fetch_or(operand, memory_order_seq_cst)|operand</tt>.
        Avoid using the implicit bitwise "OR" operator, use \ref fetch_or
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type.
    */
    Type operator|=(Type operand);

    /**
        \brief Atomically perform bitwise "XOR" and return new value
        \param operand Operand
        \return New value of the variable

        The same as <tt>fetch_xor(operand, memory_order_seq_cst)^operand</tt>.
        Avoid using the implicit bitwise "XOR" operator, use \ref fetch_xor
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type.
    */
    Type operator^=(Type operand);

    /**
        \brief Atomically add and return new value
        \param operand Operand
        \return New value of the variable

        The same as <tt>fetch_add(operand, memory_order_seq_cst)+operand</tt>.
        Avoid using the implicit add operator, use \ref fetch_add
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        @c operand is of type @c ptrdiff_t and the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type operator+=(Type operand);

    /**
        \brief Atomically subtract and return new value
        \param operand Operand
        \return New value of the variable

        The same as <tt>fetch_sub(operand, memory_order_seq_cst)-operand</tt>.
        Avoid using the implicit subtract operator, use \ref fetch_sub
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        @c operand is of type @c ptrdiff_t and the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type operator-=(Type operand);

    /**
        \brief Atomically increment and return new value
        \return New value of the variable

        The same as <tt>fetch_add(1, memory_order_seq_cst)+1</tt>.
        Avoid using the implicit increment operator, use \ref fetch_add
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type operator++(void);
    /**
        \brief Atomically increment and return old value
        \return Old value of the variable

        The same as <tt>fetch_add(1, memory_order_seq_cst)</tt>.
        Avoid using the implicit increment operator, use \ref fetch_add
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type operator++(int);
    /**
        \brief Atomically subtract and return new value
        \return New value of the variable

        The same as <tt>fetch_sub(1, memory_order_seq_cst)-1</tt>.
        Avoid using the implicit increment operator, use \ref fetch_sub
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type operator--(void);
    /**
        \brief Atomically subtract and return old value
        \return Old value of the variable

        The same as <tt>fetch_sub(1, memory_order_seq_cst)</tt>.
        Avoid using the implicit increment operator, use \ref fetch_sub
        with an explicit memory ordering constraint.

        This method is available only if \c Type is an integral type
        or a non-void pointer type. If it is a pointer type,
        the operation
        is performed following the rules for pointer arithmetic
        in C++.
    */
    Type operator--(int);

private:
    /** \brief Deleted copy constructor */
    atomic(const atomic &);
    /** \brief Deleted copy assignment */
    void operator=(const atomic &);
};

/**
    \brief Insert explicit fence
    \param order Memory ordering constraint

    Inserts an explicit fence. The exact semantic depends on the
    type of fence inserted:

    - \c memory_order_relaxed: No operation
    - \c memory_order_release: Performs a "release" operation
    - \c memory_order_acquire or \c memory_order_consume: Performs an
      "acquire" operation
    - \c memory_order_acq_rel: Performs both an "acquire" and a "release"
      operation
    - \c memory_order_seq_cst: Performs both an "acquire" and a "release"
      operation and in addition there exists a global total order of
      all \c memory_order_seq_cst operations

*/
void atomic_thread_fence(memory_order order);

}
