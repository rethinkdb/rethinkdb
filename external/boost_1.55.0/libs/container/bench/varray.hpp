// Boost.Container varray
//
// Copyright (c) 2012-2013 Adam Wulkiewicz, Lodz, Poland.
// Copyright (c) 2011-2013 Andrew Hundt.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTAINER_VARRAY_HPP
#define BOOST_CONTAINER_VARRAY_HPP

#if (defined _MSC_VER)
#  pragma once
#endif

#include <boost/container/detail/config_begin.hpp>

#include "detail/varray.hpp"

namespace boost { namespace container {

/**
 * @defgroup varray_non_member varray non-member functions
 */

/**
 * @brief A variable-size array container with fixed capacity.
 *
 * varray is a sequence container like boost::container::vector with contiguous storage that can
 * change in size, along with the static allocation, low overhead, and fixed capacity of boost::array.
 *
 * A varray is a sequence that supports random access to elements, constant time insertion and
 * removal of elements at the end, and linear time insertion and removal of elements at the beginning or 
 * in the middle. The number of elements in a varray may vary dynamically up to a fixed capacity
 * because elements are stored within the object itself similarly to an array. However, objects are 
 * initialized as they are inserted into varray unlike C arrays or std::array which must construct
 * all elements on instantiation. The behavior of varray enables the use of statically allocated
 * elements in cases with complex object lifetime requirements that would otherwise not be trivially 
 * possible.
 *
 * @par Error Handling
 *  Insertion beyond the capacity and out of bounds errors result in undefined behavior.
 *  The reason for this is because unlike vectors, varray does not perform allocation.
 *
 * @tparam Value    The type of element that will be stored.
 * @tparam Capacity The maximum number of elements varray can store, fixed at compile time.
 */
template <typename Value, std::size_t Capacity>
class varray
    : public container_detail::varray<Value, Capacity>
{
    typedef container_detail::varray<Value, Capacity> base_t;

    BOOST_COPYABLE_AND_MOVABLE(varray)

public:
    //! @brief The type of elements stored in the container.
    typedef typename base_t::value_type value_type;
    //! @brief The unsigned integral type used by the container.
    typedef typename base_t::size_type size_type;
    //! @brief The pointers difference type.
    typedef typename base_t::difference_type difference_type;
    //! @brief The pointer type.
    typedef typename base_t::pointer pointer;
    //! @brief The const pointer type.
    typedef typename base_t::const_pointer const_pointer;
    //! @brief The value reference type.
    typedef typename base_t::reference reference;
    //! @brief The value const reference type.
    typedef typename base_t::const_reference const_reference;
    //! @brief The iterator type.
    typedef typename base_t::iterator iterator;
    //! @brief The const iterator type.
    typedef typename base_t::const_iterator const_iterator;
    //! @brief The reverse iterator type.
    typedef typename base_t::reverse_iterator reverse_iterator;
    //! @brief The const reverse iterator.
    typedef typename base_t::const_reverse_iterator const_reverse_iterator;

    //! @brief Constructs an empty varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    varray()
        : base_t()
    {}

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Constructs a varray containing count value initialized Values.
    //!
    //! @param count    The number of values which will be contained in the container.
    //!
    //! @par Throws
    //!   If Value's default constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    explicit varray(size_type count)
        : base_t(count)
    {}

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Constructs a varray containing count copies of value.
    //!
    //! @param count    The number of copies of a values that will be contained in the container.
    //! @param value    The value which will be used to copy construct values.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray(size_type count, value_type const& value)
        : base_t(count, value)
    {}

    //! @pre
    //!  @li <tt>distance(first, last) <= capacity()</tt>
    //!  @li Iterator must meet the \c ForwardTraversalIterator concept.
    //!
    //! @brief Constructs a varray containing copy of a range <tt>[first, last)</tt>.
    //!
    //! @param first    The iterator to the first element in range.
    //! @param last     The iterator to the one after the last element in range.
    //!
    //! @par Throws
    //!   If Value's constructor taking a dereferenced Iterator throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <typename Iterator>
    varray(Iterator first, Iterator last)
        : base_t(first, last)
    {}

    //! @brief Constructs a copy of other varray.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray(varray const& other)
        : base_t(other)
    {}

    //! @pre <tt>other.size() <= capacity()</tt>.
    //!
    //! @brief Constructs a copy of other varray.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    varray(varray<value_type, C> const& other) : base_t(other) {}

    //! @brief Copy assigns Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws.
    //!
    //! @par Complexity
    //! Linear O(N).
    varray & operator=(BOOST_COPY_ASSIGN_REF(varray) other)
    {
        base_t::operator=(static_cast<base_t const&>(other));
        return *this;
    }

    //! @pre <tt>other.size() <= capacity()</tt>
    //!
    //! @brief Copy assigns Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be copied to this one.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
// TEMPORARY WORKAROUND
#if defined(BOOST_NO_RVALUE_REFERENCES)
    varray & operator=(::boost::rv< varray<value_type, C> > const& other)
#else
    varray & operator=(varray<value_type, C> const& other)
#endif
    {
        base_t::operator=(static_cast<varray<value_type, C> const&>(other));
        return *this;
    }

    //! @brief Move constructor. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray(BOOST_RV_REF(varray) other)
        : base_t(boost::move(static_cast<base_t&>(other)))
    {}

    //! @pre <tt>other.size() <= capacity()</tt>
    //!
    //! @brief Move constructor. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    varray(BOOST_RV_REF_2_TEMPL_ARGS(varray, value_type, C) other)
        : base_t(boost::move(static_cast<container_detail::varray<value_type, C>&>(other)))
    {}

    //! @brief Move assignment. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    varray & operator=(BOOST_RV_REF(varray) other)
    {
        base_t::operator=(boost::move(static_cast<base_t&>(other)));
        return *this;
    }

    //! @pre <tt>other.size() <= capacity()</tt>
    //!
    //! @brief Move assignment. Moves Values stored in the other varray to this one.
    //!
    //! @param other    The varray which content will be moved to this one.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws.
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    varray & operator=(BOOST_RV_REF_2_TEMPL_ARGS(varray, value_type, C) other)
    {
        base_t::operator=(boost::move(static_cast<container_detail::varray<value_type, C>&>(other)));
        return *this;
    }

#ifdef BOOST_CONTAINER_DOXYGEN_INVOKED

    //! @brief Destructor. Destroys Values stored in this container.
    //!
    //! @par Throws
    //!   Nothing
    //!
    //! @par Complexity
    //!   Linear O(N).
    ~varray();

    //! @brief Swaps contents of the other varray and this one.
    //!
    //! @param other    The varray which content will be swapped with this one's content.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws,
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws,
    //!
    //! @par Complexity
    //!   Linear O(N).
    void swap(varray & other);

    //! @pre <tt>other.size() <= capacity() && size() <= other.capacity()</tt>
    //!
    //! @brief Swaps contents of the other varray and this one.
    //!
    //! @param other    The varray which content will be swapped with this one's content.
    //!
    //! @par Throws
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c true and Value's move constructor or move assignment throws,
    //!   @li If \c boost::has_nothrow_move<Value>::value is \c false and Value's copy constructor or copy assignment throws,
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <std::size_t C>
    void swap(varray<value_type, C> & other);

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Inserts or erases elements at the end such that
    //!   the size becomes count. New elements are value initialized.
    //!
    //! @param count    The number of elements which will be stored in the container.
    //!
    //! @par Throws
    //!   If Value's default constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    void resize(size_type count);

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Inserts or erases elements at the end such that
    //!   the size becomes count. New elements are copy constructed from value.
    //!
    //! @param count    The number of elements which will be stored in the container.
    //! @param value    The value used to copy construct the new element.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    void resize(size_type count, value_type const& value);

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief This call has no effect because the Capacity of this container is constant.
    //!
    //! @param count    The number of elements which the container should be able to contain.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Linear O(N).
    void reserve(size_type count);

    //! @pre <tt>size() < capacity()</tt>
    //!
    //! @brief Adds a copy of value at the end.
    //!
    //! @param value    The value used to copy construct the new element.
    //!
    //! @par Throws
    //!   If Value's copy constructor throws.
    //!
    //! @par Complexity
    //!   Constant O(1).
    void push_back(value_type const& value);

    //! @pre <tt>size() < capacity()</tt>
    //!
    //! @brief Moves value to the end.
    //!
    //! @param value    The value to move construct the new element.
    //!
    //! @par Throws
    //!   If Value's move constructor throws.
    //!
    //! @par Complexity
    //!   Constant O(1).
    void push_back(BOOST_RV_REF(value_type) value);

    //! @pre <tt>!empty()</tt>
    //!
    //! @brief Destroys last value and decreases the size.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    void pop_back();

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a copy of element at position.
    //!
    //! @param position    The position at which the new value will be inserted.
    //! @param value       The value used to copy construct the new element.
    //!
    //! @par Throws
    //!   @li If Value's copy constructor or copy assignment throws
    //!   @li If Value's move constructor or move assignment throws.
    //!
    //! @par Complexity
    //!   Constant or linear.
    iterator insert(iterator position, value_type const& value);

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a move-constructed element at position.
    //!
    //! @param position    The position at which the new value will be inserted.
    //! @param value       The value used to move construct the new element.
    //!
    //! @par Throws
    //!   If Value's move constructor or move assignment throws.
    //!
    //! @par Complexity
    //!   Constant or linear.
    iterator insert(iterator position, BOOST_RV_REF(value_type) value);

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>size() + count <= capacity()</tt>
    //!
    //! @brief Inserts a count copies of value at position.
    //!
    //! @param position    The position at which new elements will be inserted.
    //! @param count       The number of new elements which will be inserted.
    //! @param value       The value used to copy construct new elements.
    //!
    //! @par Throws
    //!   @li If Value's copy constructor or copy assignment throws.
    //!   @li If Value's move constructor or move assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    iterator insert(iterator position, size_type count, value_type const& value);

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>.
    //!  @li <tt>distance(first, last) <= capacity()</tt>
    //!  @li \c Iterator must meet the \c ForwardTraversalIterator concept.
    //!
    //! @brief Inserts a copy of a range <tt>[first, last)</tt> at position.
    //!
    //! @param position    The position at which new elements will be inserted.
    //! @param first       The iterator to the first element of a range used to construct new elements.
    //! @param last        The iterator to the one after the last element of a range used to construct new elements.
    //!
    //! @par Throws
    //!   @li If Value's constructor and assignment taking a dereferenced \c Iterator.
    //!   @li If Value's move constructor or move assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <typename Iterator>
    iterator insert(iterator position, Iterator first, Iterator last);

    //! @pre \c position must be a valid iterator of \c *this in range <tt>[begin(), end())</tt>
    //!
    //! @brief Erases Value from position.
    //!
    //! @param position    The position of the element which will be erased from the container.
    //!
    //! @par Throws
    //!   If Value's move assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    iterator erase(iterator position);

    //! @pre
    //!  @li \c first and \c last must define a valid range
    //!  @li iterators must be in range <tt>[begin(), end()]</tt>
    //!
    //! @brief Erases Values from a range <tt>[first, last)</tt>.
    //!
    //! @param first    The position of the first element of a range which will be erased from the container.
    //! @param last     The position of the one after the last element of a range which will be erased from the container.
    //!
    //! @par Throws
    //!   If Value's move assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    iterator erase(iterator first, iterator last);

    //! @pre <tt>distance(first, last) <= capacity()</tt>
    //!
    //! @brief Assigns a range <tt>[first, last)</tt> of Values to this container.
    //!
    //! @param first       The iterator to the first element of a range used to construct new content of this container.
    //! @param last        The iterator to the one after the last element of a range used to construct new content of this container.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws,
    //!
    //! @par Complexity
    //!   Linear O(N).
    template <typename Iterator>
    void assign(Iterator first, Iterator last);

    //! @pre <tt>count <= capacity()</tt>
    //!
    //! @brief Assigns a count copies of value to this container.
    //!
    //! @param count       The new number of elements which will be container in the container.
    //! @param value       The value which will be used to copy construct the new content.
    //!
    //! @par Throws
    //!   If Value's copy constructor or copy assignment throws.
    //!
    //! @par Complexity
    //!   Linear O(N).
    void assign(size_type count, value_type const& value);

    //! @pre <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a Value constructed with
    //!   \c std::forward<Args>(args)... in the end of the container.
    //!
    //! @param args     The arguments of the constructor of the new element which will be created at the end of the container.
    //!
    //! @par Throws
    //!   If in-place constructor throws or Value's move constructor throws.
    //!
    //! @par Complexity
    //!   Constant O(1).
    template<class ...Args>
    void emplace_back(Args &&...args);

    //! @pre
    //!  @li \c position must be a valid iterator of \c *this in range <tt>[begin(), end()]</tt>
    //!  @li <tt>size() < capacity()</tt>
    //!
    //! @brief Inserts a Value constructed with
    //!   \c std::forward<Args>(args)... before position
    //!
    //! @param position The position at which new elements will be inserted.
    //! @param args     The arguments of the constructor of the new element.
    //!
    //! @par Throws
    //!   If in-place constructor throws or if Value's move constructor or move assignment throws.
    //!
    //! @par Complexity
    //!   Constant or linear.
    template<class ...Args>
    iterator emplace(iterator position, Args &&...args);

    //! @brief Removes all elements from the container.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    void clear();

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   \c std::out_of_range exception by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference at(size_type i);

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns const reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return const reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   \c std::out_of_range exception by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference at(size_type i) const;

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference operator[](size_type i);

    //! @pre <tt>i < size()</tt>
    //!
    //! @brief Returns const reference to the i-th element.
    //!
    //! @param i    The element's index.
    //!
    //! @return const reference to the i-th element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference operator[](size_type i) const;

    //! @pre \c !empty()
    //!
    //! @brief Returns reference to the first element.
    //!
    //! @return reference to the first element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference front();

    //! @pre \c !empty()
    //!
    //! @brief Returns const reference to the first element.
    //!
    //! @return const reference to the first element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference front() const;

    //! @pre \c !empty()
    //!
    //! @brief Returns reference to the last element.
    //!
    //! @return reference to the last element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reference back();

    //! @pre \c !empty()
    //!
    //! @brief Returns const reference to the first element.
    //!
    //! @return const reference to the last element
    //!   from the beginning of the container.
    //!
    //! @par Throws
    //!   Nothing by default.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reference back() const;

    //! @brief Pointer such that <tt>[data(), data() + size())</tt> is a valid range.
    //!   For a non-empty vector <tt>data() == &front()</tt>.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    Value * data();

    //! @brief Const pointer such that <tt>[data(), data() + size())</tt> is a valid range.
    //!   For a non-empty vector <tt>data() == &front()</tt>.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const Value * data() const;

    //! @brief Returns iterator to the first element.
    //!
    //! @return iterator to the first element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    iterator begin();

    //! @brief Returns const iterator to the first element.
    //!
    //! @return const_iterator to the first element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator begin() const;

    //! @brief Returns const iterator to the first element.
    //!
    //! @return const_iterator to the first element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator cbegin() const;

    //! @brief Returns iterator to the one after the last element.
    //!
    //! @return iterator pointing to the one after the last element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    iterator end();

    //! @brief Returns const iterator to the one after the last element.
    //!
    //! @return const_iterator pointing to the one after the last element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator end() const;

    //! @brief Returns const iterator to the one after the last element.
    //!
    //! @return const_iterator pointing to the one after the last element contained in the vector.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_iterator cend() const;

    //! @brief Returns reverse iterator to the first element of the reversed container.
    //!
    //! @return reverse_iterator pointing to the beginning
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reverse_iterator rbegin();

    //! @brief Returns const reverse iterator to the first element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the beginning
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator rbegin() const;

    //! @brief Returns const reverse iterator to the first element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the beginning
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator crbegin() const;

    //! @brief Returns reverse iterator to the one after the last element of the reversed container.
    //!
    //! @return reverse_iterator pointing to the one after the last element
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    reverse_iterator rend();

    //! @brief Returns const reverse iterator to the one after the last element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the one after the last element
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator rend() const;

    //! @brief Returns const reverse iterator to the one after the last element of the reversed container.
    //!
    //! @return const_reverse_iterator pointing to the one after the last element
    //! of the reversed varray.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    const_reverse_iterator crend() const;

    //! @brief Returns container's capacity.
    //!
    //! @return container's capacity.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    static size_type capacity();

    //! @brief Returns container's capacity.
    //!
    //! @return container's capacity.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    static size_type max_size();

    //! @brief Returns the number of stored elements.
    //!
    //! @return Number of elements contained in the container.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    size_type size() const;

    //! @brief Queries if the container contains elements.
    //!
    //! @return true if the number of elements contained in the
    //!   container is equal to 0.
    //!
    //! @par Throws
    //!   Nothing.
    //!
    //! @par Complexity
    //!   Constant O(1).
    bool empty() const;

#endif // BOOST_CONTAINER_DOXYGEN_INVOKED

};

#ifdef BOOST_CONTAINER_DOXYGEN_INVOKED

//! @brief Checks if contents of two varrays are equal.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if containers have the same size and elements in both containers are equal.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator== (varray<V, C1> const& x, varray<V, C2> const& y);

//! @brief Checks if contents of two varrays are not equal.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if containers have different size or elements in both containers are not equal.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator!= (varray<V, C1> const& x, varray<V, C2> const& y);

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if x compares lexicographically less than y.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator< (varray<V, C1> const& x, varray<V, C2> const& y);

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if y compares lexicographically less than x.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator> (varray<V, C1> const& x, varray<V, C2> const& y);

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if y don't compare lexicographically less than x.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator<= (varray<V, C1> const& x, varray<V, C2> const& y);

//! @brief Lexicographically compares varrays.
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @return     \c true if x don't compare lexicographically less than y.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
bool operator>= (varray<V, C1> const& x, varray<V, C2> const& y);

//! @brief Swaps contents of two varrays.
//!
//! This function calls varray::swap().
//!
//! @ingroup varray_non_member
//!
//! @param x    The first varray.
//! @param y    The second varray.
//!
//! @par Complexity
//!   Linear O(N).
template<typename V, std::size_t C1, std::size_t C2>
inline void swap(varray<V, C1> & x, varray<V, C2> & y);

#endif // BOOST_CONTAINER_DOXYGEN_INVOKED

}} // namespace boost::container

#include <boost/container/detail/config_end.hpp>

#endif // BOOST_CONTAINER_VARRAY_HPP
