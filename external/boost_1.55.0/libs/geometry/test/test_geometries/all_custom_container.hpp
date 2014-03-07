// Boost.Geometry (aka GGL, Generic Geometry Library) 
// Unit Test

// Copyright (c) 2011-2012 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_CONTAINER_HPP
#define GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_CONTAINER_HPP

#include <cstddef>
#include <deque>


template <typename Item>
class all_custom_container
{
private :
    std::deque<Item> m_hidden_deque;

public :
    typedef typename std::deque<Item>::iterator custom_iterator_type;
    typedef typename std::deque<Item>::const_iterator custom_const_iterator_type;

    inline std::size_t custom_size() const { return m_hidden_deque.size(); }

    inline custom_const_iterator_type custom_begin() const { return m_hidden_deque.begin(); }
    inline custom_const_iterator_type custom_end() const { return m_hidden_deque.end(); }
    inline custom_iterator_type custom_begin() { return m_hidden_deque.begin(); }
    inline custom_iterator_type custom_end() { return m_hidden_deque.end(); }

    inline void custom_clear() { m_hidden_deque.clear(); }
    inline void custom_push_back(Item const& p) { m_hidden_deque.push_back(p); }
    inline void custom_resize(std::size_t new_size) { m_hidden_deque.resize(new_size); }
};


// 1. Adapt to Boost.Geometry (for e.g. inner rings)
namespace boost { namespace geometry
{

namespace traits
{
    template <typename Item>
    struct clear<all_custom_container<Item> >
    {
        static inline void apply(all_custom_container<Item>& container)
        {
            container.custom_clear();
        }
    };

    template <typename Item>
    struct push_back<all_custom_container<Item> >
    {
        static inline void apply(all_custom_container<Item>& container, Item const& item)
        {
            container.custom_push_back(item);
        }
    };

    template <typename Item>
    struct resize<all_custom_container<Item> >
    {
        static inline void apply(all_custom_container<Item>& container, std::size_t new_size)
        {
            container.custom_resize(new_size);
        }
    };

} // namespace traits

}} // namespace boost::geometry


// 2a. Adapt to Boost.Range, meta-functions
namespace boost
{
    template<typename Item>
    struct range_mutable_iterator<all_custom_container<Item> >
    {
        typedef typename all_custom_container<Item>::custom_iterator_type type;
    };

    template<typename Item>
    struct range_const_iterator<all_custom_container<Item> >
    {
        typedef typename all_custom_container<Item>::custom_const_iterator_type  type;
    };

} // namespace boost


// 2b. Adapt to Boost.Range, part 2, ADP

template<typename Item>
inline typename all_custom_container<Item>::custom_iterator_type
    range_begin(all_custom_container<Item>& container)
{
    return container.custom_begin();
}

template<typename Item>
inline typename all_custom_container<Item>::custom_const_iterator_type
    range_begin(all_custom_container<Item> const& container)
{
    return container.custom_begin();
}

template<typename Item>
inline typename all_custom_container<Item>::custom_iterator_type
    range_end(all_custom_container<Item>& container)
{
    return container.custom_end();
}

template<typename Item>
inline typename all_custom_container<Item>::custom_const_iterator_type
    range_end(all_custom_container<Item> const& container)
{
    return container.custom_end();
}

// (Optional)
template<typename Item>
inline std::size_t range_calculate_size(all_custom_container<Item> const& container)
{
    return container.custom_size();
}




#endif // GEOMETRY_TEST_TEST_GEOMETRIES_ALL_CUSTOM_CONTAINER_HPP
