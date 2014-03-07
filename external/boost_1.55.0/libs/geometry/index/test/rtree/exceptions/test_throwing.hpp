// Boost.Geometry Index
//
// Throwing objects implementation
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_TEST_THROWING_HPP
#define BOOST_GEOMETRY_INDEX_TEST_THROWING_HPP

// value

struct throwing_value_copy_exception : public std::exception
{
    const char * what() const throw() { return "value copy failed."; }
};

struct throwing_value
{
    explicit throwing_value(int v = 0)
        : value(v)
    {}

    bool operator==(throwing_value const& v) const
    {
        return value == v.value;
    }

    throwing_value(throwing_value const& v)
    {
        throw_if_required();

        value = v.value;
    }

    throwing_value & operator=(throwing_value const& v)
    {
        throw_if_required();

        value = v.value;
        return *this;
    }

    void throw_if_required()
    {
        // throw if counter meets max count
        if ( get_max_calls_ref() <= get_calls_counter_ref() )
            throw throwing_value_copy_exception();
        else
            ++get_calls_counter_ref();
    }

    static void reset_calls_counter() { get_calls_counter_ref() = 0; }
    static void set_max_calls(size_t mc) { get_max_calls_ref() = mc; }

    static size_t & get_calls_counter_ref() { static size_t cc = 0; return cc; }
    static size_t & get_max_calls_ref() { static size_t mc = (std::numeric_limits<size_t>::max)(); return mc; }

    int value;
};

namespace generate {
template <typename T, typename C>
struct value< std::pair<bg::model::point<T, 2, C>, throwing_value> >
{
    typedef bg::model::point<T, 2, C> P;
    typedef std::pair<P, throwing_value> R;
    static R apply(int x, int y)
    {
        return std::make_pair(P(x, y), throwing_value(x + y * 100));
    }
};
} // namespace generate

#include <boost/geometry/index/detail/varray.hpp>

struct throwing_varray_exception : public std::exception
{
    const char * what() const throw() { return "static vector exception."; }
};

struct throwing_varray_settings
{
    static void throw_if_required()
    {
        // throw if counter meets max count
        if ( get_max_calls_ref() <= get_calls_counter_ref() )
            throw throwing_varray_exception();
        else
            ++get_calls_counter_ref();
    }

    static void reset_calls_counter() { get_calls_counter_ref() = 0; }
    static void set_max_calls(size_t mc) { get_max_calls_ref() = mc; }

    static size_t & get_calls_counter_ref() { static size_t cc = 0; return cc; }
    static size_t & get_max_calls_ref() { static size_t mc = (std::numeric_limits<size_t>::max)(); return mc; }
};

template <typename Element, size_t Capacity>
class throwing_varray
    : public boost::geometry::index::detail::varray<Element, Capacity>
{
    typedef boost::geometry::index::detail::varray<Element, Capacity> container;

public:
    typedef typename container::value_type value_type;
    typedef typename container::size_type size_type;
    typedef typename container::iterator iterator;
    typedef typename container::const_iterator const_iterator;
    typedef typename container::reverse_iterator reverse_iterator;
    typedef typename container::const_reverse_iterator const_reverse_iterator;
    typedef typename container::reference reference;
    typedef typename container::const_reference const_reference;

    inline void resize(size_type s)
    {
        throwing_varray_settings::throw_if_required();
        container::resize(s);
    }

    inline void reserve(size_type s)
    {
        throwing_varray_settings::throw_if_required();
        container::reserve(s);
    }

    void push_back(Element const& v)
    {
        throwing_varray_settings::throw_if_required();
        container::push_back(v);
    }
};

#endif // BOOST_GEOMETRY_INDEX_TEST_THROWING_HPP
