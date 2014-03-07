// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef GEOMETRY_TEST_GEOMETRY_TEST_COMMON_HPP
#define GEOMETRY_TEST_GEOMETRY_TEST_COMMON_HPP

#if defined(_MSC_VER)
// We deliberately mix float/double's  so turn off warnings
#pragma warning( disable : 4244 )
// For (new since Boost 1.40) warning in Boost.Test on putenv/posix
#pragma warning( disable : 4996 )

//#pragma warning( disable : 4305 )
#endif // defined(_MSC_VER)

#include <boost/config.hpp>


#if defined(BOOST_INTEL_CXX_VERSION)
#define BOOST_GEOMETRY_TEST_ONLY_ONE_TYPE
#endif


#include <boost/foreach.hpp>


// Include some always-included-for-testing files
#if ! defined(BOOST_GEOMETRY_NO_BOOST_TEST)

// Until Boost.Test fixes it, silence warning issued by clang:
// warning: unused variable 'check_is_close' [-Wunused-variable]
#ifdef __clang__
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wunused-variable"
#endif

# include <boost/test/floating_point_comparison.hpp>
# include <boost/test/included/test_exec_monitor.hpp>
//#  include <boost/test/included/prg_exec_monitor.hpp>
# include <boost/test/impl/execution_monitor.ipp>

#ifdef __clang__
# pragma clang diagnostic pop
#endif

#endif


#if defined(HAVE_TTMATH)
#  include <boost/geometry/extensions/contrib/ttmath_stub.hpp>
#endif

#if defined(HAVE_CLN) || defined(HAVE_GMP)
#  include <boost/numeric_adaptor/numeric_adaptor.hpp>
#endif


#if defined(HAVE_GMP)
#  include <boost/numeric_adaptor/gmp_value_type.hpp>
#endif
#if defined(HAVE_CLN)
#  include <boost/numeric_adaptor/cln_value_type.hpp>
#endif

// For all tests:
// - do NOT use "using namespace boost::geometry" to make clear what is Boost.Geometry
// - use bg:: as short alias
#include <boost/geometry/core/coordinate_type.hpp>
#include <boost/geometry/core/closure.hpp>
#include <boost/geometry/core/point_order.hpp>
#include <boost/geometry/core/tag.hpp>
namespace bg = boost::geometry;


template <typename T>
struct string_from_type {};

template <> struct string_from_type<void>
{ static std::string name() { return "v"; }  };

template <> struct string_from_type<float>
{ static std::string name() { return "f"; }  };

template <> struct string_from_type<double>
{ static std::string name() { return "d"; }  };

template <> struct string_from_type<long double>
{ static std::string name() { return "e"; }  };

template <> struct string_from_type<int>
{ static std::string name() { return "i"; }  };

#if defined(HAVE_TTMATH)
    template <> struct string_from_type<ttmath_big>
    { static std::string name() { return "t"; }  };
#endif

#if defined(BOOST_RATIONAL_HPP) 
template <typename T> struct string_from_type<boost::rational<T> >
{ static std::string name() { return "r"; }  };
#endif


#if defined(HAVE_GMP)
    template <> struct string_from_type<boost::numeric_adaptor::gmp_value_type>
    { static std::string name() { return "g"; }  };
#endif

#if defined(HAVE_CLN)
    template <> struct string_from_type<boost::numeric_adaptor::cln_value_type>
    { static std::string name() { return "c"; }  };
#endif


template <typename CoordinateType, typename T1, typename T2>
inline T1 if_typed_tt(T1 value_tt, T2 value)
{
#if defined(HAVE_TTMATH)
    return boost::is_same<CoordinateType, ttmath_big>::type::value ? value_tt : value;
#else
    return value;
#endif
}

template <typename CoordinateType, typename Specified, typename T>
inline T if_typed(T value_typed, T value)
{
    return boost::is_same<CoordinateType, Specified>::value ? value_typed : value;
}

template <typename Geometry1, typename Geometry2>
inline std::string type_for_assert_message()
{
    bool const ccw =
        bg::point_order<Geometry1>::value == bg::counterclockwise
        || bg::point_order<Geometry2>::value == bg::counterclockwise;
    bool const open =
        bg::closure<Geometry1>::value == bg::open
        || bg::closure<Geometry2>::value == bg::open;

    std::ostringstream out;
    out << string_from_type<typename bg::coordinate_type<Geometry1>::type>::name()
        << (ccw ? " ccw" : "")
        << (open ? " open" : "");
    return out.str();
}

struct geographic_policy
{
    template <typename CoordinateType>
    static inline CoordinateType apply(CoordinateType const& value)
    {
        return value;
    }
};

struct mathematical_policy
{
    template <typename CoordinateType>
    static inline CoordinateType apply(CoordinateType const& value)
    {
        return 90 - value;
    }
    
};



#endif // GEOMETRY_TEST_GEOMETRY_TEST_COMMON_HPP
