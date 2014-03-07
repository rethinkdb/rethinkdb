// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/preprocessor.hpp>
#include <boost/parameter/name.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/tuple/tuple.hpp>
#include <string>
#include "basics.hpp"
#include <boost/utility/enable_if.hpp>

namespace test {

namespace mpl = boost::mpl;

using mpl::_;
using boost::is_convertible;

BOOST_PARAMETER_NAME(x)

// Sun has problems with this syntax:
//
//   template1< r* ( template2<x> ) >
//
// Workaround: factor template2<x> into a separate typedef

#if BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x580))

typedef is_convertible<_,char const*> predicate;

BOOST_PARAMETER_FUNCTION((int), sfinae, tag,
  (deduced
     (optional (x, *(predicate), 0))
  )
)
{
    return 1;
}

#else

BOOST_PARAMETER_FUNCTION((int), sfinae, tag,
  (deduced
     (optional (x, *(is_convertible<_,char const*>), 0))
  )
)
{
    return 1;
}

#endif

template<class A0>
typename boost::enable_if<boost::is_same<int,A0>, int>::type
sfinae(A0 const& a0)
{
    return 0;
}

} // namespace test

int main()
{
    using namespace test;

    assert(sfinae() == 1);
    assert(sfinae("foo") == 1);
    assert(sfinae(1) == 0);

    return 0;
}

