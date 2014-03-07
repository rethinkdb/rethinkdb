// Helper classes and functions for the circular buffer.

// Copyright (c) 2003-2008 Jan Gaspar

// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_CIRCULAR_BUFFER_DETAILS_HPP)
#define BOOST_CIRCULAR_BUFFER_DETAILS_HPP

#if defined(_MSC_VER) && _MSC_VER >= 1200
    #pragma once
#endif

#include <boost/iterator.hpp>
#include <boost/throw_exception.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <iterator>

// Silence MS /W4 warnings like C4913:
// "user defined binary operator ',' exists but no overload could convert all operands, default built-in binary operator ',' used"
// This might happen when previously including some boost headers that overload the coma operator.
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4913)
#endif

namespace boost {

namespace cb_details {

template <class Traits> struct nonconst_traits;

template<class ForwardIterator, class Diff, class T, class Alloc>
void uninitialized_fill_n_with_alloc(
    ForwardIterator first, Diff n, const T& item, Alloc& alloc);

template<class ValueType, class InputIterator, class ForwardIterator>
ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator dest);

template<class ValueType, class InputIterator, class ForwardIterator>
ForwardIterator uninitialized_move_if_noexcept(InputIterator first, InputIterator last, ForwardIterator dest);

/*!
    \struct const_traits
    \brief Defines the data types for a const iterator.
*/
template <class Traits>
struct const_traits {
    // Basic types
    typedef typename Traits::value_type value_type;
    typedef typename Traits::const_pointer pointer;
    typedef typename Traits::const_reference reference;
    typedef typename Traits::size_type size_type;
    typedef typename Traits::difference_type difference_type;

    // Non-const traits
    typedef nonconst_traits<Traits> nonconst_self;
};

/*!
    \struct nonconst_traits
    \brief Defines the data types for a non-const iterator.
*/
template <class Traits>
struct nonconst_traits {
    // Basic types
    typedef typename Traits::value_type value_type;
    typedef typename Traits::pointer pointer;
    typedef typename Traits::reference reference;
    typedef typename Traits::size_type size_type;
    typedef typename Traits::difference_type difference_type;

    // Non-const traits
    typedef nonconst_traits<Traits> nonconst_self;
};

/*!
    \struct iterator_wrapper
    \brief Helper iterator dereference wrapper.
*/
template <class Iterator>
struct iterator_wrapper {
    mutable Iterator m_it;
    explicit iterator_wrapper(Iterator it) : m_it(it) {}
    Iterator operator () () const { return m_it++; }
private:
    iterator_wrapper<Iterator>& operator = (const iterator_wrapper<Iterator>&); // do not generate
};

/*!
    \struct item_wrapper
    \brief Helper item dereference wrapper.
*/
template <class Pointer, class Value>
struct item_wrapper {
    Value m_item;
    explicit item_wrapper(Value item) : m_item(item) {}
    Pointer operator () () const { return &m_item; }
private:
    item_wrapper<Pointer, Value>& operator = (const item_wrapper<Pointer, Value>&); // do not generate
};

/*!
    \struct assign_n
    \brief Helper functor for assigning n items.
*/
template <class Value, class Alloc>
struct assign_n {
    typedef typename Alloc::size_type size_type;
    size_type m_n;
    Value m_item;
    Alloc& m_alloc;
    assign_n(size_type n, Value item, Alloc& alloc) : m_n(n), m_item(item), m_alloc(alloc) {}
    template <class Pointer>
    void operator () (Pointer p) const {
        uninitialized_fill_n_with_alloc(p, m_n, m_item, m_alloc);
    }
private:
    assign_n<Value, Alloc>& operator = (const assign_n<Value, Alloc>&); // do not generate
};

/*!
    \struct assign_range
    \brief Helper functor for assigning range of items.
*/
template <class ValueType, class Iterator>
struct assign_range {
    Iterator m_first;
    Iterator m_last;

    assign_range(const Iterator& first, const Iterator& last) BOOST_NOEXCEPT
    : m_first(first), m_last(last) {}

    template <class Pointer>
    void operator () (Pointer p) const {
        boost::cb_details::uninitialized_copy<ValueType>(m_first, m_last, p);
    }
};

template <class ValueType, class Iterator>
inline assign_range<ValueType, Iterator> make_assign_range(const Iterator& first, const Iterator& last) {
    return assign_range<ValueType, Iterator>(first, last);
}

/*!
    \class capacity_control
    \brief Capacity controller of the space optimized circular buffer.
*/
template <class Size>
class capacity_control {

    //! The capacity of the space-optimized circular buffer.
    Size m_capacity;

    //! The lowest guaranteed or minimum capacity of the adapted space-optimized circular buffer.
    Size m_min_capacity;

public:

    //! Constructor.
    capacity_control(Size buffer_capacity, Size min_buffer_capacity = 0)
    : m_capacity(buffer_capacity), m_min_capacity(min_buffer_capacity)
    { // Check for capacity lower than min_capacity.
        BOOST_CB_ASSERT(buffer_capacity >= min_buffer_capacity);
    }

    // Default copy constructor.

    // Default assign operator.

    //! Get the capacity of the space optimized circular buffer.
    Size capacity() const { return m_capacity; }

    //! Get the minimal capacity of the space optimized circular buffer.
    Size min_capacity() const { return m_min_capacity; }

    //! Size operator - returns the capacity of the space optimized circular buffer.
    operator Size() const { return m_capacity; }
};

/*!
    \struct iterator
    \brief Random access iterator for the circular buffer.
    \param Buff The type of the underlying circular buffer.
    \param Traits Basic iterator types.
    \note This iterator is not circular. It was designed
          for iterating from begin() to end() of the circular buffer.
*/
template <class Buff, class Traits>
struct iterator :
    public boost::iterator<
    std::random_access_iterator_tag,
    typename Traits::value_type,
    typename Traits::difference_type,
    typename Traits::pointer,
    typename Traits::reference>
#if BOOST_CB_ENABLE_DEBUG
    , public debug_iterator_base
#endif // #if BOOST_CB_ENABLE_DEBUG
{
// Helper types

    //! Base iterator.
    typedef boost::iterator<
        std::random_access_iterator_tag,
        typename Traits::value_type,
        typename Traits::difference_type,
        typename Traits::pointer,
        typename Traits::reference> base_iterator;

    //! Non-const iterator.
    typedef iterator<Buff, typename Traits::nonconst_self> nonconst_self;

// Basic types

    //! The type of the elements stored in the circular buffer.
    typedef typename base_iterator::value_type value_type;

    //! Pointer to the element.
    typedef typename base_iterator::pointer pointer;

    //! Reference to the element.
    typedef typename base_iterator::reference reference;

    //! Size type.
    typedef typename Traits::size_type size_type;

    //! Difference type.
    typedef typename base_iterator::difference_type difference_type;

// Member variables

    //! The circular buffer where the iterator points to.
    const Buff* m_buff;

    //! An internal iterator.
    pointer m_it;

// Construction & assignment

    // Default copy constructor.

    //! Default constructor.
    iterator() : m_buff(0), m_it(0) {}

#if BOOST_CB_ENABLE_DEBUG

    //! Copy constructor (used for converting from a non-const to a const iterator).
    iterator(const nonconst_self& it) : debug_iterator_base(it), m_buff(it.m_buff), m_it(it.m_it) {}

    //! Internal constructor.
    /*!
        \note This constructor is not intended to be used directly by the user.
    */
    iterator(const Buff* cb, const pointer p) : debug_iterator_base(cb), m_buff(cb), m_it(p) {}

#else

    iterator(const nonconst_self& it) : m_buff(it.m_buff), m_it(it.m_it) {}

    iterator(const Buff* cb, const pointer p) : m_buff(cb), m_it(p) {}

#endif // #if BOOST_CB_ENABLE_DEBUG

    //! Assign operator.
    iterator& operator = (const iterator& it) {
        if (this == &it)
            return *this;
#if BOOST_CB_ENABLE_DEBUG
        debug_iterator_base::operator =(it);
#endif // #if BOOST_CB_ENABLE_DEBUG
        m_buff = it.m_buff;
        m_it = it.m_it;
        return *this;
    }

// Random access iterator methods

    //! Dereferencing operator.
    reference operator * () const {
        BOOST_CB_ASSERT(is_valid(m_buff)); // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(m_it != 0);        // check for iterator pointing to end()
        return *m_it;
    }

    //! Dereferencing operator.
    pointer operator -> () const { return &(operator*()); }

    //! Difference operator.
    template <class Traits0>
    difference_type operator - (const iterator<Buff, Traits0>& it) const {
        BOOST_CB_ASSERT(is_valid(m_buff));    // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(it.is_valid(m_buff)); // check for uninitialized or invalidated iterator
        return linearize_pointer(*this) - linearize_pointer(it);
    }

    //! Increment operator (prefix).
    iterator& operator ++ () {
        BOOST_CB_ASSERT(is_valid(m_buff)); // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(m_it != 0);        // check for iterator pointing to end()
        m_buff->increment(m_it);
        if (m_it == m_buff->m_last)
            m_it = 0;
        return *this;
    }

    //! Increment operator (postfix).
    iterator operator ++ (int) {
        iterator<Buff, Traits> tmp = *this;
        ++*this;
        return tmp;
    }

    //! Decrement operator (prefix).
    iterator& operator -- () {
        BOOST_CB_ASSERT(is_valid(m_buff));        // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(m_it != m_buff->m_first); // check for iterator pointing to begin()
        if (m_it == 0)
            m_it = m_buff->m_last;
        m_buff->decrement(m_it);
        return *this;
    }

    //! Decrement operator (postfix).
    iterator operator -- (int) {
        iterator<Buff, Traits> tmp = *this;
        --*this;
        return tmp;
    }

    //! Iterator addition.
    iterator& operator += (difference_type n) {
        BOOST_CB_ASSERT(is_valid(m_buff)); // check for uninitialized or invalidated iterator
        if (n > 0) {
            BOOST_CB_ASSERT(m_buff->end() - *this >= n); // check for too large n
            m_it = m_buff->add(m_it, n);
            if (m_it == m_buff->m_last)
                m_it = 0;
        } else if (n < 0) {
            *this -= -n;
        }
        return *this;
    }

    //! Iterator addition.
    iterator operator + (difference_type n) const { return iterator<Buff, Traits>(*this) += n; }

    //! Iterator subtraction.
    iterator& operator -= (difference_type n) {
        BOOST_CB_ASSERT(is_valid(m_buff)); // check for uninitialized or invalidated iterator
        if (n > 0) {
            BOOST_CB_ASSERT(*this - m_buff->begin() >= n); // check for too large n
            m_it = m_buff->sub(m_it == 0 ? m_buff->m_last : m_it, n);
        } else if (n < 0) {
            *this += -n;
        }
        return *this;
    }

    //! Iterator subtraction.
    iterator operator - (difference_type n) const { return iterator<Buff, Traits>(*this) -= n; }

    //! Element access operator.
    reference operator [] (difference_type n) const { return *(*this + n); }

// Equality & comparison

    //! Equality.
    template <class Traits0>
    bool operator == (const iterator<Buff, Traits0>& it) const {
        BOOST_CB_ASSERT(is_valid(m_buff));    // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(it.is_valid(m_buff)); // check for uninitialized or invalidated iterator
        return m_it == it.m_it;
    }

    //! Inequality.
    template <class Traits0>
    bool operator != (const iterator<Buff, Traits0>& it) const {
        BOOST_CB_ASSERT(is_valid(m_buff));    // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(it.is_valid(m_buff)); // check for uninitialized or invalidated iterator
        return m_it != it.m_it;
    }

    //! Less.
    template <class Traits0>
    bool operator < (const iterator<Buff, Traits0>& it) const {
        BOOST_CB_ASSERT(is_valid(m_buff));    // check for uninitialized or invalidated iterator
        BOOST_CB_ASSERT(it.is_valid(m_buff)); // check for uninitialized or invalidated iterator
        return linearize_pointer(*this) < linearize_pointer(it);
    }

    //! Greater.
    template <class Traits0>
    bool operator > (const iterator<Buff, Traits0>& it) const { return it < *this; }

    //! Less or equal.
    template <class Traits0>
    bool operator <= (const iterator<Buff, Traits0>& it) const { return !(it < *this); }

    //! Greater or equal.
    template <class Traits0>
    bool operator >= (const iterator<Buff, Traits0>& it) const { return !(*this < it); }

// Helpers

    //! Get a pointer which would point to the same element as the iterator in case the circular buffer is linearized.
    template <class Traits0>
    typename Traits0::pointer linearize_pointer(const iterator<Buff, Traits0>& it) const {
        return it.m_it == 0 ? m_buff->m_buff + m_buff->size() :
            (it.m_it < m_buff->m_first ? it.m_it + (m_buff->m_end - m_buff->m_first)
                : m_buff->m_buff + (it.m_it - m_buff->m_first));
    }
};

//! Iterator addition.
template <class Buff, class Traits>
inline iterator<Buff, Traits>
operator + (typename Traits::difference_type n, const iterator<Buff, Traits>& it) {
    return it + n;
}

#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_MSVC_STD_ITERATOR)

//! Iterator category.
template <class Buff, class Traits>
inline std::random_access_iterator_tag iterator_category(const iterator<Buff, Traits>&) {
    return std::random_access_iterator_tag();
}

//! The type of the elements stored in the circular buffer.
template <class Buff, class Traits>
inline typename Traits::value_type* value_type(const iterator<Buff, Traits>&) { return 0; }

//! Distance type.
template <class Buff, class Traits>
inline typename Traits::difference_type* distance_type(const iterator<Buff, Traits>&) { return 0; }

#endif // #if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION) && !defined(BOOST_MSVC_STD_ITERATOR)

/*!
    \fn ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator dest)
    \brief Equivalent of <code>std::uninitialized_copy</code> but with explicit specification of value type.
*/
template<class ValueType, class InputIterator, class ForwardIterator>
inline ForwardIterator uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator dest) {
    typedef ValueType value_type;

    // We do not use allocator.construct and allocator.destroy
    // because C++03 requires to take parameter by const reference but
    // Boost.move requires nonconst reference
    ForwardIterator next = dest;
    BOOST_TRY {
        for (; first != last; ++first, ++dest)
            ::new (dest) value_type(*first);
    } BOOST_CATCH(...) {
        for (; next != dest; ++next)
            next->~value_type();
        BOOST_RETHROW
    }
    BOOST_CATCH_END
    return dest;
}

template<class ValueType, class InputIterator, class ForwardIterator>
ForwardIterator uninitialized_move_if_noexcept_impl(InputIterator first, InputIterator last, ForwardIterator dest,
    true_type) {
    for (; first != last; ++first, ++dest)
        ::new (dest) ValueType(boost::move(*first));
    return dest;
}

template<class ValueType, class InputIterator, class ForwardIterator>
ForwardIterator uninitialized_move_if_noexcept_impl(InputIterator first, InputIterator last, ForwardIterator dest,
    false_type) {
    return uninitialized_copy<ValueType>(first, last, dest);
}

/*!
    \fn ForwardIterator uninitialized_move_if_noexcept(InputIterator first, InputIterator last, ForwardIterator dest)
    \brief Equivalent of <code>std::uninitialized_copy</code> but with explicit specification of value type and moves elements if they have noexcept move constructors.
*/
template<class ValueType, class InputIterator, class ForwardIterator>
ForwardIterator uninitialized_move_if_noexcept(InputIterator first, InputIterator last, ForwardIterator dest) {
    typedef typename boost::is_nothrow_move_constructible<ValueType>::type tag_t;
    return uninitialized_move_if_noexcept_impl<ValueType>(first, last, dest, tag_t());
}

/*!
    \fn void uninitialized_fill_n_with_alloc(ForwardIterator first, Diff n, const T& item, Alloc& alloc)
    \brief Equivalent of <code>std::uninitialized_fill_n</code> with allocator.
*/
template<class ForwardIterator, class Diff, class T, class Alloc>
inline void uninitialized_fill_n_with_alloc(ForwardIterator first, Diff n, const T& item, Alloc& alloc) {
    ForwardIterator next = first;
    BOOST_TRY {
        for (; n > 0; ++first, --n)
            alloc.construct(first, item);
    } BOOST_CATCH(...) {
        for (; next != first; ++next)
            alloc.destroy(next);
        BOOST_RETHROW
    }
    BOOST_CATCH_END
}

} // namespace cb_details

} // namespace boost

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#endif // #if !defined(BOOST_CIRCULAR_BUFFER_DETAILS_HPP)
