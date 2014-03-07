// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/parameters.hpp>
#include <boost/parameter/name.hpp>
#include <boost/parameter/binding.hpp>
#include "deduced.hpp"

namespace parameter = boost::parameter;
namespace mpl = boost::mpl;

BOOST_PARAMETER_NAME(x)
BOOST_PARAMETER_NAME(y)
BOOST_PARAMETER_NAME(z)

int main()
{
    using namespace parameter;

    check<
        parameters<
            tag::x
          , tag::y
        >
    >(
        (_x = 0, _y = 1)
      , 0
      , 1
    );

    check<
        parameters<
            tag::x
          , required<deduced<tag::y>, boost::is_convertible<mpl::_, int> >
          , optional<deduced<tag::z>, boost::is_convertible<mpl::_, char const*> >
        >
    >(
        (_x = 0, _y = not_present, _z = "foo")
      , _x = 0
      , "foo"
    );

    check<
        parameters<
            tag::x
          , required<deduced<tag::y>, boost::is_convertible<mpl::_, int> >
          , optional<deduced<tag::z>, boost::is_convertible<mpl::_, char const*> >
        >
    >(
        (_x = 0, _y = 1, _z = "foo")
      , 0
      , "foo"
      , 1
    );

    check<
        parameters<
            tag::x
          , required<deduced<tag::y>, boost::is_convertible<mpl::_, int> >
          , optional<deduced<tag::z>, boost::is_convertible<mpl::_, char const*> >
        >
    >(
        (_x = 0, _y = 1, _z = "foo")
      , 0
      , 1
      , "foo"
    );

    check<
        parameters<
            tag::x
          , required<deduced<tag::y>, boost::is_convertible<mpl::_, int> >
          , optional<deduced<tag::z>, boost::is_convertible<mpl::_, char const*> >
        >
    >(
        (_x = 0, _y = 1, _z = "foo")
      , 0
      , _y = 1
      , "foo"
    );

    check<
        parameters<
            tag::x
          , required<deduced<tag::y>, boost::is_convertible<mpl::_, int> >
          , optional<deduced<tag::z>, boost::is_convertible<mpl::_, char const*> >
        >
    >(
        (_x = 0, _y = 1, _z = "foo")
      , _z = "foo"
      , _x = 0
      , 1
    );

    // Fails becasue of parameters.hpp:428
/*
    check<
        parameters<
            tag::x
          , required<deduced<tag::y>, boost::is_convertible<mpl::_, int> >
          , optional<deduced<tag::z>, boost::is_convertible<mpl::_, char const*> >
        >
    >(
        (_x = 0, _y = 1, _z = "foo")
      , _x = 0
      , (long*)0
      , 1
    );
*/

    return 0;
};

