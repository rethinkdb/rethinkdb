// Boost.Geometry Index
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_INDEX_EQUAL_TO_HPP
#define BOOST_GEOMETRY_INDEX_EQUAL_TO_HPP

#include <boost/geometry/algorithms/equals.hpp>

namespace boost { namespace geometry { namespace index {

namespace detail {

template <typename Geometry, typename Tag>
struct equals
{
    static bool apply(Geometry const& g1, Geometry const& g2)
    {
        return geometry::equals(g1, g2);
    }
};

template <typename T>
struct equals<T, void>
{
    static bool apply(T const& v1, T const& v2)
    {
        return v1 == v2;
    }
};

template <typename Tuple, size_t I, size_t N>
struct tuple_equals
{
    inline static bool apply(Tuple const& t1, Tuple const& t2)
    {
        typedef typename boost::tuples::element<I, Tuple>::type T;
        return
            equals<
                T, typename geometry::traits::tag<T>::type
            >::apply(boost::get<I>(t1), boost::get<I>(t2))
            &&
            tuple_equals<Tuple, I+1, N>::apply(t1, t2);
    }
};

template <typename Tuple, size_t I>
struct tuple_equals<Tuple, I, I>
{
    inline static bool apply(Tuple const&, Tuple const&)
    {
        return true;
    }
};

} // namespace detail

/*!
\brief The function object comparing Values.

It compares Geometries using geometry::equals() function. Other types are compared using operator==.
The default version handles Values which are Indexables.
This template is also specialized for std::pair<T1, T2> and boost::tuple<...>.

\tparam Value       The type of objects which are compared by this function object.
*/
template <typename Value>
struct equal_to
{
    /*! \brief The type of result returned by function object. */
    typedef bool result_type;
    
    /*!
    \brief Compare values. If Value is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    bool operator()(Value const& l, Value const& r) const
    {
        return detail::equals<Value, typename geometry::traits::tag<Value>::type>::apply(l ,r);
    }
};

/*!
\brief The function object comparing Values.

This specialization compares values of type std::pair<T1, T2>.
It compares pairs' first values, then second values.

\tparam T1       The first type.
\tparam T2       The second type.
*/
template <typename T1, typename T2>
struct equal_to< std::pair<T1, T2> >
{
    /*! \brief The type of result returned by function object. */
    typedef bool result_type;

    /*!
    \brief Compare values. If pair<> Value member is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    bool operator()(std::pair<T1, T2> const& l, std::pair<T1, T2> const& r) const
    {
        typedef detail::equals<T1, typename geometry::traits::tag<T1>::type> equals1;
        typedef detail::equals<T2, typename geometry::traits::tag<T2>::type> equals2;

        return equals1::apply(l.first, r.first) && equals2::apply(l.second, r.second);
    }
};

/*!
\brief The function object comparing Values.

This specialization compares values of type boost::tuple<...>.
It compares all members of the tuple from the first one to the last one.
*/
template <typename T0, typename T1, typename T2, typename T3, typename T4,
          typename T5, typename T6, typename T7, typename T8, typename T9>
struct equal_to< boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> >
{
    typedef boost::tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> value_type;

    /*! \brief The type of result returned by function object. */
    typedef bool result_type;

    /*!
    \brief Compare values. If tuple<> Value member is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    bool operator()(value_type const& l, value_type const& r) const
    {
        return detail::tuple_equals<
            value_type, 0, boost::tuples::length<value_type>::value
        >::apply(l ,r);
    }
};

}}} // namespace boost::geometry::index

#if !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#include <tuple>

namespace boost { namespace geometry { namespace index {

namespace detail {

template <typename Tuple, size_t I, size_t N>
struct std_tuple_equals
{
    inline static bool apply(Tuple const& t1, Tuple const& t2)
    {
        typedef typename std::tuple_element<I, Tuple>::type T;
        return
            equals<
                T, typename geometry::traits::tag<T>::type
            >::apply(std::get<I>(t1), std::get<I>(t2))
            &&
            std_tuple_equals<Tuple, I+1, N>::apply(t1, t2);
    }
};

template <typename Tuple, size_t I>
struct std_tuple_equals<Tuple, I, I>
{
    inline static bool apply(Tuple const&, Tuple const&)
    {
        return true;
    }
};

} // namespace detail

/*!
\brief The function object comparing Values.

This specialization compares values of type std::tuple<Args...>.
It's defined if the compiler supports tuples and variadic templates.
It compares all members of the tuple from the first one to the last one.
*/
template <typename ...Args>
struct equal_to< std::tuple<Args...> >
{
    typedef std::tuple<Args...> value_type;

    /*! \brief The type of result returned by function object. */
    typedef bool result_type;

    /*!
    \brief Compare values. If tuple<> Value member is a Geometry geometry::equals() function is used.
    
    \param l First value.
    \param r Second value.
    \return true if values are equal.
    */
    bool operator()(value_type const& l, value_type const& r) const
    {
        return detail::std_tuple_equals<
            value_type, 0, std::tuple_size<value_type>::value
        >::apply(l ,r);
    }
};

}}} // namespace boost::geometry::index

#endif // !defined(BOOST_NO_CXX11_HDR_TUPLE) && !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)

#endif // BOOST_GEOMETRY_INDEX_EQUAL_TO_HPP
