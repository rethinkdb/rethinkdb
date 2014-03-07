// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_UTIL_MATH_HPP
#define BOOST_GEOMETRY_UTIL_MATH_HPP

#include <cmath>
#include <limits>

#include <boost/math/constants/constants.hpp>

#include <boost/geometry/util/select_most_precise.hpp>

namespace boost { namespace geometry
{

namespace math
{

#ifndef DOXYGEN_NO_DETAIL
namespace detail
{


template <typename Type, bool IsFloatingPoint>
struct equals
{
    static inline bool apply(Type const& a, Type const& b)
    {
        return a == b;
    }
};

template <typename Type>
struct equals<Type, true>
{
    static inline Type get_max(Type const& a, Type const& b, Type const& c)
    {
        return (std::max)((std::max)(a, b), c);
    }

    static inline bool apply(Type const& a, Type const& b)
    {
        if (a == b)
        {
            return true;
        }

        // See http://www.parashift.com/c++-faq-lite/newbie.html#faq-29.17,
        // FUTURE: replace by some boost tool or boost::test::close_at_tolerance
        return std::abs(a - b) <= std::numeric_limits<Type>::epsilon() * get_max(std::abs(a), std::abs(b), 1.0);
    }
};

template <typename Type, bool IsFloatingPoint>
struct smaller
{
    static inline bool apply(Type const& a, Type const& b)
    {
        return a < b;
    }
};

template <typename Type>
struct smaller<Type, true>
{
    static inline bool apply(Type const& a, Type const& b)
    {
        if (equals<Type, true>::apply(a, b))
        {
            return false;
        }
        return a < b;
    }
};


template <typename Type, bool IsFloatingPoint> 
struct equals_with_epsilon : public equals<Type, IsFloatingPoint> {};


/*!
\brief Short construct to enable partial specialization for PI, currently not possible in Math.
*/
template <typename T>
struct define_pi
{
    static inline T apply()
    {
        // Default calls Boost.Math
        return boost::math::constants::pi<T>();
    }
};

template <typename T>
struct relaxed_epsilon
{
    static inline T apply(const T& factor)
    {
        return factor * std::numeric_limits<T>::epsilon();
    }
};


} // namespace detail
#endif


template <typename T>
inline T pi() { return detail::define_pi<T>::apply(); }

template <typename T>
inline T relaxed_epsilon(T const& factor)
{
    return detail::relaxed_epsilon<T>::apply(factor);
}


// Maybe replace this by boost equals or boost ublas numeric equals or so

/*!
    \brief returns true if both arguments are equal.
    \ingroup utility
    \param a first argument
    \param b second argument
    \return true if a == b
    \note If both a and b are of an integral type, comparison is done by ==.
    If one of the types is floating point, comparison is done by abs and
    comparing with epsilon. If one of the types is non-fundamental, it might
    be a high-precision number and comparison is done using the == operator
    of that class.
*/

template <typename T1, typename T2>
inline bool equals(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::equals
        <
            select_type,
            boost::is_floating_point<select_type>::type::value
        >::apply(a, b);
}

template <typename T1, typename T2>
inline bool equals_with_epsilon(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::equals_with_epsilon
        <
            select_type, 
            boost::is_floating_point<select_type>::type::value
        >::apply(a, b);
}

template <typename T1, typename T2>
inline bool smaller(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::smaller
        <
            select_type,
            boost::is_floating_point<select_type>::type::value
        >::apply(a, b);
}

template <typename T1, typename T2>
inline bool larger(T1 const& a, T2 const& b)
{
    typedef typename select_most_precise<T1, T2>::type select_type;
    return detail::smaller
        <
            select_type,
            boost::is_floating_point<select_type>::type::value
        >::apply(b, a);
}



double const d2r = geometry::math::pi<double>() / 180.0;
double const r2d = 1.0 / d2r;

/*!
    \brief Calculates the haversine of an angle
    \ingroup utility
    \note See http://en.wikipedia.org/wiki/Haversine_formula
    haversin(alpha) = sin2(alpha/2)
*/
template <typename T>
inline T hav(T const& theta)
{
    T const half = T(0.5);
    T const sn = sin(half * theta);
    return sn * sn;
}

/*!
\brief Short utility to return the square
\ingroup utility
\param value Value to calculate the square from
\return The squared value
*/
template <typename T>
inline T sqr(T const& value)
{
    return value * value;
}

/*!
\brief Short utility to workaround gcc/clang problem that abs is converting to integer
       and that older versions of MSVC does not support abs of long long...
\ingroup utility
*/
template<typename T>
inline T abs(T const& value)
{
    T const zero = T();
    return value < zero ? -value : value;
}

/*!
\brief Short utility to calculate the sign of a number: -1 (negative), 0 (zero), 1 (positive)
\ingroup utility
*/
template <typename T>
static inline int sign(T const& value) 
{
    T const zero = T();
    return value > zero ? 1 : value < zero ? -1 : 0;
}


} // namespace math


}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_UTIL_MATH_HPP
