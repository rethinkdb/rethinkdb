// Boost.Geometry Index
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_DETAIL_PUSHABLE_ARRAY_HPP
#define BOOST_GEOMETRY_INDEX_DETAIL_PUSHABLE_ARRAY_HPP

#include <boost/array.hpp>

#include <boost/geometry/index/detail/assert.hpp>

namespace boost { namespace geometry { namespace index { namespace detail {

template <typename Element, size_t Capacity>
class pushable_array
{
    typedef typename boost::array<Element, Capacity> array_type;

public:
    typedef typename array_type::value_type value_type;
    typedef typename array_type::size_type size_type;
    typedef typename array_type::iterator iterator;
    typedef typename array_type::const_iterator const_iterator;
    typedef typename array_type::reverse_iterator reverse_iterator;
    typedef typename array_type::const_reverse_iterator const_reverse_iterator;
    typedef typename array_type::reference reference;
    typedef typename array_type::const_reference const_reference;

    inline pushable_array()
        : m_size(0)
    {}

    inline explicit pushable_array(size_type s)
        : m_size(s)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(s <= Capacity, "size too big");
    }

    inline void resize(size_type s)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(s <= Capacity, "size too big");
        m_size = s;
    }

    inline void reserve(size_type /*s*/)
    {
        //BOOST_GEOMETRY_INDEX_ASSERT(s <= Capacity, "size too big");
        // do nothing
    }

    inline Element & operator[](size_type i)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(i < m_size, "index of the element outside the container");
        return m_array[i];
    }

    inline Element const& operator[](size_type i) const
    {
        BOOST_GEOMETRY_INDEX_ASSERT(i < m_size, "index of the element outside the container");
        return m_array[i];
    }

    inline Element const& front() const
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < m_size, "there are no elements in the container");
        return m_array.front();
    }

    inline Element & front()
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < m_size, "there are no elements in the container");
        return m_array.front();
    }

    inline Element const& back() const
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < m_size, "there are no elements in the container");
        return *(begin() + (m_size - 1));
    }

    inline Element & back()
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < m_size, "there are no elements in the container");
        return *(begin() + (m_size - 1));
    }

    inline iterator begin()
    {
        return m_array.begin();
    }

    inline iterator end()
    {
        return m_array.begin() + m_size;
    }

    inline const_iterator begin() const
    {
        return m_array.begin();
    }

    inline const_iterator end() const
    {
        return m_array.begin() + m_size;
    }

    inline reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }

    inline reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }

    inline const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }

    inline const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }

    inline void clear()
    {
        m_size = 0;
    }

    inline void push_back(Element const& v)
    {
        BOOST_GEOMETRY_INDEX_ASSERT(m_size < Capacity, "can't further increase the size of the container");
        m_array[m_size] = v;
        ++m_size;
    }

    inline void pop_back()
    {
        BOOST_GEOMETRY_INDEX_ASSERT(0 < m_size, "there are no elements in the container");
        --m_size;
    }

    inline bool empty() const
    {
        return m_size == 0;
    }
    
    inline size_t size() const
    {
        return m_size;
    }

    inline size_t capacity() const
    {
        return Capacity;
    }
    
private:
    boost::array<Element, Capacity> m_array;
    size_type m_size;
};

}}}} // namespace boost::geometry::index::detail

#endif // BOOST_GEOMETRY_INDEX_DETAIL_PUSHABLE_ARRAY_HPP
