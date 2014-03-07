// Copyright John Maddock 2006.
// Copyright Paul A. Bristow 2007, 2009
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef _MSC_VER
#  pragma warning(disable : 4756) // overflow in constant arithmetic
// Constants are too big for float case, but this doesn't matter for test.
#endif

#include <boost/math/concepts/real_concept.hpp>
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/array.hpp>
#include "functor.hpp"

#include "handle_test_result.hpp"
#include "table_type.hpp"

#ifndef SC_
#define SC_(x) static_cast<typename table_type<T>::type>(BOOST_JOIN(x, L))
#endif

template <class Real, typename T>
void do_test_ellint_e2(const T& data, const char* type_name, const char* test)
{
   typedef Real                   value_type;

   std::cout << "Testing: " << test << std::endl;

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
    value_type (*fp2)(value_type, value_type) = boost::math::ellint_2<value_type, value_type>;
#else
    value_type (*fp2)(value_type, value_type) = boost::math::ellint_2;
#endif
    boost::math::tools::test_result<value_type> result;

    result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(fp2, 1, 0),
      extract_result<Real>(2));
   handle_test_result(result, data[result.worst()], result.worst(),
      type_name, "boost::math::ellint_2", test);

   std::cout << std::endl;
}

template <class Real, typename T>
void do_test_ellint_e1(T& data, const char* type_name, const char* test)
{
    typedef Real                   value_type;
    boost::math::tools::test_result<value_type> result;

   std::cout << "Testing: " << test << std::endl;

#if defined(BOOST_MATH_NO_DEDUCED_FUNCTION_POINTERS)
   value_type (*fp1)(value_type) = boost::math::ellint_2<value_type>;
#else
   value_type (*fp1)(value_type) = boost::math::ellint_2;
#endif
   result = boost::math::tools::test_hetero<Real>(
      data,
      bind_func<Real>(fp1, 0),
      extract_result<Real>(1));
   handle_test_result(result, data[result.worst()], result.worst(),
      type_name, "boost::math::ellint_2", test);

   std::cout << std::endl;
}

template <typename T>
void test_spots(T, const char* type_name)
{
    // Function values calculated on http://functions.wolfram.com/
    // Note that Mathematica's EllipticE accepts k^2 as the second parameter.
    static const boost::array<boost::array<T, 3>, 10> data1 = {{
        {{ SC_(0), SC_(0), SC_(0) }},
        {{ SC_(-10), SC_(0), SC_(-10) }},
        {{ SC_(-1), SC_(-1), SC_(-0.84147098480789650665250232163029899962256306079837) }},
        {{ SC_(-4), T(900) / 1024, SC_(-3.1756145986492562317862928524528520686391383168377) }},
        {{ SC_(8), T(-600) / 1024, SC_(7.2473147180505693037677015377802777959345489333465) }},
        {{ SC_(1e-05), T(800) / 1024, SC_(9.999999999898274739584436515967055859383969942432E-6) }},
        {{ SC_(1e+05), T(100) / 1024, SC_(99761.153306972066658135668386691227343323331995888) }},
        {{ SC_(1e+10), SC_(-0.5), SC_(9.3421545766487137036576748555295222252286528414669e9) }},
        {{ static_cast<T>(ldexp(T(1), 66)), T(400) / 1024, SC_(7.0886102721911705466476846969992069994308167515242e19) }},
        {{ static_cast<T>(ldexp(T(1), 166)), T(900) / 1024, SC_(7.1259011068364515942912094521783688927118026465790e49) }},
    }};

    do_test_ellint_e2<T>(data1, type_name, "Elliptic Integral E: Mathworld Data");

#include "ellint_e2_data.ipp"

    do_test_ellint_e2<T>(ellint_e2_data, type_name, "Elliptic Integral E: Random Data");

    // Function values calculated on http://functions.wolfram.com/
    // Note that Mathematica's EllipticE accepts k^2 as the second parameter.
    static const boost::array<boost::array<T, 2>, 10> data2 = {{
        {{ SC_(-1), SC_(1) }},
        {{ SC_(0), SC_(1.5707963267948966192313216916397514420985846996876) }},
        {{ T(100) / 1024, SC_(1.5670445330545086723323795143598956428788609133377) }},
        {{ T(200) / 1024, SC_(1.5557071588766556854463404816624361127847775545087) }},
        {{ T(300) / 1024, SC_(1.5365278991162754883035625322482669608948678755743) }},
        {{ T(400) / 1024, SC_(1.5090417763083482272165682786143770446401437564021) }},
        {{ SC_(-0.5), SC_(1.4674622093394271554597952669909161360253617523272) }},
        {{ T(-600) / 1024, SC_(1.4257538571071297192428217218834579920545946473778) }},
        {{ T(-800) / 1024, SC_(1.2927868476159125056958680222998765985004489572909) }},
        {{ T(-900) / 1024, SC_(1.1966864890248739524112920627353824133420353430982) }},
    }};

    do_test_ellint_e1<T>(data2, type_name, "Elliptic Integral E: Mathworld Data");

#include "ellint_e_data.ipp"

    do_test_ellint_e1<T>(ellint_e_data, type_name, "Elliptic Integral E: Random Data");
}

