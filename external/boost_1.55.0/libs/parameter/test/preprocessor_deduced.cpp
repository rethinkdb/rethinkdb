// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/name.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/tuple/tuple.hpp>
#include <string>
#include "basics.hpp"

#ifndef BOOST_NO_SFINAE
# include <boost/utility/enable_if.hpp>
#endif

namespace test {

namespace mpl = boost::mpl;

using mpl::_;
using boost::is_convertible;

BOOST_PARAMETER_NAME(expected)
BOOST_PARAMETER_NAME(x)
BOOST_PARAMETER_NAME(y)
BOOST_PARAMETER_NAME(z)

// Sun has problems with this syntax:
//
//   template1< r* ( template2<x> ) >
//
// Workaround: factor template2<x> into a separate typedef
typedef is_convertible<_, int> predicate1;
typedef is_convertible<_, std::string> predicate2;

#if BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x580))

BOOST_PARAMETER_FUNCTION((int), f, tag,
    (required
       (expected, *)
    )
    (deduced
       (required
          (x, *(predicate1))
          (y, *(predicate2))
       )
    )
)
#else
BOOST_PARAMETER_FUNCTION((int), f, tag,
    (required
       (expected, *)
    )
    (deduced
       (required
          (x, *(is_convertible<_, int>))
          (y, *(is_convertible<_, std::string>))
       )
    )
)
#endif 
{
    assert(equal(x, boost::tuples::get<0>(expected)));
    assert(equal(y, boost::tuples::get<1>(expected)));
    return 1;
}

struct X 
{
    X(int x = -1)
      : x(x)
    {}
    
    bool operator==(X const& other) const
    {
        return x == other.x;
    }
    
    int x;
};

typedef is_convertible<_, X> predicate3;  // SunPro workaround; see above

BOOST_PARAMETER_FUNCTION((int), g, tag,
    (required
      (expected, *)
    )
    (deduced
       (required
          (x, *(is_convertible<_, int>))
          (y, *(is_convertible<_, std::string>))
       )
       (optional
          (z, *(predicate3), X())
       )
    )
)
{
    assert(equal(x, boost::tuples::get<0>(expected)));
    assert(equal(y, boost::tuples::get<1>(expected)));
    assert(equal(z, boost::tuples::get<2>(expected)));
    return 1;
}

BOOST_PARAMETER_FUNCTION(
    (int), sfinae, tag,
    (deduced
      (required
        (x, *(predicate2))
      )
    )
)
{
    return 1;
}

#ifndef BOOST_NO_SFINAE
// On compilers that actually support SFINAE, add another overload
// that is an equally good match and can only be in the overload set
// when the others are not.  This tests that the SFINAE is actually
// working.  On all other compilers we're just checking that
// everything about SFINAE-enabled code will work, except of course
// the SFINAE.
template<class A0>
typename boost::enable_if<boost::is_same<int,A0>, int>::type
sfinae(A0 const& a0)
{
    return 0;
}
#endif

} // namespace test

using boost::make_tuple;

// make_tuple doesn't work with char arrays.
char const* str(char const* s)
{
    return s;
}

int main()
{
    using namespace test;

    f(make_tuple(0, str("foo")), _x = 0, _y = "foo");
    f(make_tuple(0, str("foo")), _x = 0, _y = "foo");
    f(make_tuple(0, str("foo")), 0, "foo");
    f(make_tuple(0, str("foo")), "foo", 0);
    f(make_tuple(0, str("foo")), _y = "foo", 0);
    f(make_tuple(0, str("foo")), _x = 0, "foo");
    f(make_tuple(0, str("foo")), 0, _y = "foo");

    g(make_tuple(0, str("foo"), X()), _x = 0, _y = "foo");
    g(make_tuple(0, str("foo"), X()), 0, "foo");
    g(make_tuple(0, str("foo"), X()), "foo", 0);
    g(make_tuple(0, str("foo"), X()), _y = "foo", 0);   
    g(make_tuple(0, str("foo"), X()), _x = 0, "foo");
    g(make_tuple(0, str("foo"), X()), 0, _y = "foo");

    g(make_tuple(0, str("foo"), X(1)), 0, _y = "foo", X(1));
    g(make_tuple(0, str("foo"), X(1)), X(1), 0, _y = "foo");

#ifndef BOOST_NO_SFINAE
    assert(sfinae("foo") == 1);
    assert(sfinae(0) == 0);
#endif

    return 0;
}

