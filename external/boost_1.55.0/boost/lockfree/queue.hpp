//  lock-free queue from
//  Michael, M. M. and Scott, M. L.,
//  "simple, fast and practical non-blocking and blocking concurrent queue algorithms"
//
//  Copyright (C) 2008-2013 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_LOCKFREE_FIFO_HPP_INCLUDED
#define BOOST_LOCKFREE_FIFO_HPP_INCLUDED

#include <boost/assert.hpp>
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
#include <boost/noncopyable.hpp>
#endif
#include <boost/static_assert.hpp>
#include <boost/type_traits/has_trivial_assign.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>

#include <boost/lockfree/detail/atomic.hpp>
#include <boost/lockfree/detail/copy_payload.hpp>
#include <boost/lockfree/detail/freelist.hpp>
#include <boost/lockfree/detail/parameter.hpp>
#include <boost/lockfree/detail/tagged_ptr.hpp>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4324) // structure was padded due to __declspec(align())
#endif


namespace boost    {
namespace lockfree {
namespace detail   {

typedef parameter::parameters<boost::parameter::optional<tag::allocator>,
                              boost::parameter::optional<tag::capacity>
                             > queue_signature;

} /* namespace detail */


/** The queue class provides a multi-writer/multi-reader queue, pushing and popping is lock-free,
 *  construction/destruction has to be synchronized. It uses a freelist for memory management,
 *  freed nodes are pushed to the freelist and not returned to the OS before the queue is destroyed.
 *
 *  \b Policies:
 *  - \ref boost::lockfree::fixed_sized, defaults to \c boost::lockfree::fixed_sized<false> \n
 *    Can be used to completely disable dynamic memory allocations during push in order to ensure lockfree behavior. \n
 *    If the data structure is configured as fixed-sized, the internal nodes are stored inside an array and they are addressed
 *    by array indexing. This limits the possible size of the queue to the number of elements that can be addressed by the index
 *    type (usually 2**16-2), but on platforms that lack double-width compare-and-exchange instructions, this is the best way
 *    to achieve lock-freedom.
 *
 *  - \ref boost::lockfree::capacity, optional \n
 *    If this template argument is passed to the options, the size of the queue is set at compile-time.\n
 *    It this option implies \c fixed_sized<true>
 *
 *  - \ref boost::lockfree::allocator, defaults to \c boost::lockfree::allocator<std::allocator<void>> \n
 *    Specifies the allocator that is used for the internal freelist
 *
 *  \b Requirements:
 *   - T must have a copy constructor
 *   - T must have a trivial assignment operator
 *   - T must have a trivial destructor
 *
 * */
#ifndef BOOST_DOXYGEN_INVOKED
template <typename T,
          class A0 = boost::parameter::void_,
          class A1 = boost::parameter::void_,
          class A2 = boost::parameter::void_>
#else
template <typename T, ...Options>
#endif
class queue
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
    : boost::noncopyable
#endif
{
private:
#ifndef BOOST_DOXYGEN_INVOKED

#ifdef BOOST_HAS_TRIVIAL_DESTRUCTOR
    BOOST_STATIC_ASSERT((boost::has_trivial_destructor<T>::value));
#endif

#ifdef BOOST_HAS_TRIVIAL_ASSIGN
    BOOST_STATIC_ASSERT((boost::has_trivial_assign<T>::value));
#endif

    typedef typename detail::queue_signature::bind<A0, A1, A2>::type bound_args;

    static const bool has_capacity = detail::extract_capacity<bound_args>::has_capacity;
    static const size_t capacity = detail::extract_capacity<bound_args>::capacity + 1; // the queue uses one dummy node
    static const bool fixed_sized = detail::extract_fixed_sized<bound_args>::value;
    static const bool node_based = !(has_capacity || fixed_sized);
    static const bool compile_time_sized = has_capacity;

    struct BOOST_LOCKFREE_CACHELINE_ALIGNMENT node
    {
        typedef typename detail::select_tagged_handle<node, node_based>::tagged_handle_type tagged_node_handle;
        typedef typename detail::select_tagged_handle<node, node_based>::handle_type handle_type;

        node(T const & v, handle_type null_handle):
            data(v)//, next(tagged_node_handle(0, 0))
        {
            /* increment tag to avoid ABA problem */
            tagged_node_handle old_next = next.load(memory_order_relaxed);
            tagged_node_handle new_next (null_handle, old_next.get_next_tag());
            next.store(new_next, memory_order_release);
        }

        node (handle_type null_handle):
            next(tagged_node_handle(null_handle, 0))
        {}

        node(void)
        {}

        atomic<tagged_node_handle> next;
        T data;
    };

    typedef typename detail::extract_allocator<bound_args, node>::type node_allocator;
    typedef typename detail::select_freelist<node, node_allocator, compile_time_sized, fixed_sized, capacity>::type pool_t;
    typedef typename pool_t::tagged_node_handle tagged_node_handle;
    typedef typename detail::select_tagged_handle<node, node_based>::handle_type handle_type;

    void initialize(void)
    {
        node * n = pool.template construct<true, false>(pool.null_handle());
        tagged_node_handle dummy_node(pool.get_handle(n), 0);
        head_.store(dummy_node, memory_order_relaxed);
        tail_.store(dummy_node, memory_order_release);
    }

    struct implementation_defined
    {
        typedef node_allocator allocator;
        typedef std::size_t size_type;
    };

#endif

#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
    queue(queue const &) = delete;
    queue(queue &&)      = delete;
    const queue& operator=( const queue& ) = delete;
#endif

public:
    typedef T value_type;
    typedef typename implementation_defined::allocator allocator;
    typedef typename implementation_defined::size_type size_type;

    /**
     * \return true, if implementation is lock-free.
     *
     * \warning It only checks, if the queue head and tail nodes and the freelist can be modified in a lock-free manner.
     *       On most platforms, the whole implementation is lock-free, if this is true. Using c++0x-style atomics, there is
     *       no possibility to provide a completely accurate implementation, because one would need to test every internal
     *       node, which is impossible if further nodes will be allocated from the operating system.
     * */
    bool is_lock_free (void) const
    {
        return head_.is_lock_free() && tail_.is_lock_free() && pool.is_lock_free();
    }

    //! Construct queue
    // @{
    queue(void):
        head_(tagged_node_handle(0, 0)),
        tail_(tagged_node_handle(0, 0)),
        pool(node_allocator(), capacity)
    {
        BOOST_ASSERT(has_capacity);
        initialize();
    }

    template <typename U>
    explicit queue(typename node_allocator::template rebind<U>::other const & alloc):
        head_(tagged_node_handle(0, 0)),
        tail_(tagged_node_handle(0, 0)),
        pool(alloc, capacity)
    {
        BOOST_STATIC_ASSERT(has_capacity);
        initialize();
    }

    explicit queue(allocator const & alloc):
        head_(tagged_node_handle(0, 0)),
        tail_(tagged_node_handle(0, 0)),
        pool(alloc, capacity)
    {
        BOOST_ASSERT(has_capacity);
        initialize();
    }
    // @}

    //! Construct queue, allocate n nodes for the freelist.
    // @{
    explicit queue(size_type n):
        head_(tagged_node_handle(0, 0)),
        tail_(tagged_node_handle(0, 0)),
        pool(node_allocator(), n + 1)
    {
        BOOST_ASSERT(!has_capacity);
        initialize();
    }

    template <typename U>
    queue(size_type n, typename node_allocator::template rebind<U>::other const & alloc):
        head_(tagged_node_handle(0, 0)),
        tail_(tagged_node_handle(0, 0)),
        pool(alloc, n + 1)
    {
        BOOST_STATIC_ASSERT(!has_capacity);
        initialize();
    }
    // @}

    /** \copydoc boost::lockfree::stack::reserve
     * */
    void reserve(size_type n)
    {
        pool.template reserve<true>(n);
    }

    /** \copydoc boost::lockfree::stack::reserve_unsafe
     * */
    void reserve_unsafe(size_type n)
    {
        pool.template reserve<false>(n);
    }

    /** Destroys queue, free all nodes from freelist.
     * */
    ~queue(void)
    {
        T dummy;
        while(unsynchronized_pop(dummy))
        {}

        pool.template destruct<false>(head_.load(memory_order_relaxed));
    }

    /** Check if the queue is empty
     *
     * \return true, if the queue is empty, false otherwise
     * \note The result is only accurate, if no other thread modifies the queue. Therefore it is rarely practical to use this
     *       value in program logic.
     * */
    bool empty(void)
    {
        return pool.get_handle(head_.load()) == pool.get_handle(tail_.load());
    }

    /** Pushes object t to the queue.
     *
     * \post object will be pushed to the queue, if internal node can be allocated
     * \returns true, if the push operation is successful.
     *
     * \note Thread-safe. If internal memory pool is exhausted and the memory pool is not fixed-sized, a new node will be allocated
     *                    from the OS. This may not be lock-free.
     * */
    bool push(T const & t)
    {
        return do_push<false>(t);
    }

    /** Pushes object t to the queue.
     *
     * \post object will be pushed to the queue, if internal node can be allocated
     * \returns true, if the push operation is successful.
     *
     * \note Thread-safe and non-blocking. If internal memory pool is exhausted, operation will fail
     * \throws if memory allocator throws
     * */
    bool bounded_push(T const & t)
    {
        return do_push<true>(t);
    }


private:
#ifndef BOOST_DOXYGEN_INVOKED
    template <bool Bounded>
    bool do_push(T const & t)
    {
        using detail::likely;

        node * n = pool.template construct<true, Bounded>(t, pool.null_handle());
        handle_type node_handle = pool.get_handle(n);

        if (n == NULL)
            return false;

        for (;;) {
            tagged_node_handle tail = tail_.load(memory_order_acquire);
            node * tail_node = pool.get_pointer(tail);
            tagged_node_handle next = tail_node->next.load(memory_order_acquire);
            node * next_ptr = pool.get_pointer(next);

            tagged_node_handle tail2 = tail_.load(memory_order_acquire);
            if (likely(tail == tail2)) {
                if (next_ptr == 0) {
                    tagged_node_handle new_tail_next(node_handle, next.get_next_tag());
                    if ( tail_node->next.compare_exchange_weak(next, new_tail_next) ) {
                        tagged_node_handle new_tail(node_handle, tail.get_next_tag());
                        tail_.compare_exchange_strong(tail, new_tail);
                        return true;
                    }
                }
                else {
                    tagged_node_handle new_tail(pool.get_handle(next_ptr), tail.get_next_tag());
                    tail_.compare_exchange_strong(tail, new_tail);
                }
            }
        }
    }
#endif

public:

    /** Pushes object t to the queue.
     *
     * \post object will be pushed to the queue, if internal node can be allocated
     * \returns true, if the push operation is successful.
     *
     * \note Not Thread-safe. If internal memory pool is exhausted and the memory pool is not fixed-sized, a new node will be allocated
     *       from the OS. This may not be lock-free.
     * \throws if memory allocator throws
     * */
    bool unsynchronized_push(T const & t)
    {
        node * n = pool.template construct<false, false>(t, pool.null_handle());

        if (n == NULL)
            return false;

        for (;;) {
            tagged_node_handle tail = tail_.load(memory_order_relaxed);
            tagged_node_handle next = tail->next.load(memory_order_relaxed);
            node * next_ptr = next.get_ptr();

            if (next_ptr == 0) {
                tail->next.store(tagged_node_handle(n, next.get_next_tag()), memory_order_relaxed);
                tail_.store(tagged_node_handle(n, tail.get_next_tag()), memory_order_relaxed);
                return true;
            }
            else
                tail_.store(tagged_node_handle(next_ptr, tail.get_next_tag()), memory_order_relaxed);
        }
    }

    /** Pops object from queue.
     *
     * \post if pop operation is successful, object will be copied to ret.
     * \returns true, if the pop operation is successful, false if queue was empty.
     *
     * \note Thread-safe and non-blocking
     * */
    bool pop (T & ret)
    {
        return pop<T>(ret);
    }

    /** Pops object from queue.
     *
     * \pre type U must be constructible by T and copyable, or T must be convertible to U
     * \post if pop operation is successful, object will be copied to ret.
     * \returns true, if the pop operation is successful, false if queue was empty.
     *
     * \note Thread-safe and non-blocking
     * */
    template <typename U>
    bool pop (U & ret)
    {
        using detail::likely;
        for (;;) {
            tagged_node_handle head = head_.load(memory_order_acquire);
            node * head_ptr = pool.get_pointer(head);

            tagged_node_handle tail = tail_.load(memory_order_acquire);
            tagged_node_handle next = head_ptr->next.load(memory_order_acquire);
            node * next_ptr = pool.get_pointer(next);

            tagged_node_handle head2 = head_.load(memory_order_acquire);
            if (likely(head == head2)) {
                if (pool.get_handle(head) == pool.get_handle(tail)) {
                    if (next_ptr == 0)
                        return false;

                    tagged_node_handle new_tail(pool.get_handle(next), tail.get_next_tag());
                    tail_.compare_exchange_strong(tail, new_tail);

                } else {
                    if (next_ptr == 0)
                        /* this check is not part of the original algorithm as published by michael and scott
                         *
                         * however we reuse the tagged_ptr part for the freelist and clear the next part during node
                         * allocation. we can observe a null-pointer here.
                         * */
                        continue;
                    detail::copy_payload(next_ptr->data, ret);

                    tagged_node_handle new_head(pool.get_handle(next), head.get_next_tag());
                    if (head_.compare_exchange_weak(head, new_head)) {
                        pool.template destruct<true>(head);
                        return true;
                    }
                }
            }
        }
    }

    /** Pops object from queue.
     *
     * \post if pop operation is successful, object will be copied to ret.
     * \returns true, if the pop operation is successful, false if queue was empty.
     *
     * \note Not thread-safe, but non-blocking
     *
     * */
    bool unsynchronized_pop (T & ret)
    {
        return unsynchronized_pop<T>(ret);
    }

    /** Pops object from queue.
     *
     * \pre type U must be constructible by T and copyable, or T must be convertible to U
     * \post if pop operation is successful, object will be copied to ret.
     * \returns true, if the pop operation is successful, false if queue was empty.
     *
     * \note Not thread-safe, but non-blocking
     *
     * */
    template <typename U>
    bool unsynchronized_pop (U & ret)
    {
        for (;;) {
            tagged_node_handle head = head_.load(memory_order_relaxed);
            node * head_ptr = pool.get_pointer(head);
            tagged_node_handle tail = tail_.load(memory_order_relaxed);
            tagged_node_handle next = head_ptr->next.load(memory_order_relaxed);
            node * next_ptr = pool.get_pointer(next);

            if (pool.get_handle(head) == pool.get_handle(tail)) {
                if (next_ptr == 0)
                    return false;

                tagged_node_handle new_tail(pool.get_handle(next), tail.get_next_tag());
                tail_.store(new_tail);
            } else {
                if (next_ptr == 0)
                    /* this check is not part of the original algorithm as published by michael and scott
                     *
                     * however we reuse the tagged_ptr part for the freelist and clear the next part during node
                     * allocation. we can observe a null-pointer here.
                     * */
                    continue;
                detail::copy_payload(next_ptr->data, ret);
                tagged_node_handle new_head(pool.get_handle(next), head.get_next_tag());
                head_.store(new_head);
                pool.template destruct<false>(head);
                return true;
            }
        }
    }

    /** consumes one element via a functor
     *
     *  pops one element from the queue and applies the functor on this object
     *
     * \returns true, if one element was consumed
     *
     * \note Thread-safe and non-blocking, if functor is thread-safe and non-blocking
     * */
    template <typename Functor>
    bool consume_one(Functor & f)
    {
        T element;
        bool success = pop(element);
        if (success)
            f(element);

        return success;
    }

    /// \copydoc boost::lockfree::queue::consume_one(Functor & rhs)
    template <typename Functor>
    bool consume_one(Functor const & f)
    {
        T element;
        bool success = pop(element);
        if (success)
            f(element);

        return success;
    }

    /** consumes all elements via a functor
     *
     * sequentially pops all elements from the queue and applies the functor on each object
     *
     * \returns number of elements that are consumed
     *
     * \note Thread-safe and non-blocking, if functor is thread-safe and non-blocking
     * */
    template <typename Functor>
    size_t consume_all(Functor & f)
    {
        size_t element_count = 0;
        while (consume_one(f))
            element_count += 1;

        return element_count;
    }

    /// \copydoc boost::lockfree::queue::consume_all(Functor & rhs)
    template <typename Functor>
    size_t consume_all(Functor const & f)
    {
        size_t element_count = 0;
        while (consume_one(f))
            element_count += 1;

        return element_count;
    }

private:
#ifndef BOOST_DOXYGEN_INVOKED
    atomic<tagged_node_handle> head_;
    static const int padding_size = BOOST_LOCKFREE_CACHELINE_BYTES - sizeof(tagged_node_handle);
    char padding1[padding_size];
    atomic<tagged_node_handle> tail_;
    char padding2[padding_size];

    pool_t pool;
#endif
};

} /* namespace lockfree */
} /* namespace boost */

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif /* BOOST_LOCKFREE_FIFO_HPP_INCLUDED */
