// Copyright Daniel Wallin 2006. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/parameter/parameters.hpp>
#include <boost/parameter/name.hpp>
#include <boost/parameter/binding.hpp>
#include <boost/type_traits.hpp>
#include "deduced.hpp"

namespace parameter = boost::parameter;
namespace mpl = boost::mpl;

BOOST_PARAMETER_NAME(x)
BOOST_PARAMETER_NAME(y)
BOOST_PARAMETER_NAME(z)

int main()
{
    using namespace parameter;
    using boost::is_same;
    using boost::remove_reference;
    using boost::add_reference;

    check<
        parameters<
            tag::x
          , optional<
                deduced<tag::y>
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))    
              , is_same<
                    mpl::_1
                  , remove_reference<binding<mpl::_2,tag::x> >
                > 
#else
              , is_same<
                    add_reference<mpl::_1>
                  , binding<mpl::_2,tag::x>
                > 
#endif
            >
        >
    >(
        (_x = 0, _y = 1)
      , 0
      , 1
    );

    check<
        parameters<
            tag::x
          , optional<
                deduced<tag::y>
#if BOOST_WORKAROUND(__BORLANDC__, BOOST_TESTED_AT(0x564))    
              , is_same<
                    mpl::_1
                  , remove_reference<binding<mpl::_2,tag::x> >
                > 
#else
              , is_same<
                    add_reference<mpl::_1>
                  , binding<mpl::_2,tag::x>
                > 
#endif
            >
        >
    >(
        (_x = 0U, _y = 1U)
      , 0U
      , 1U
    );

    check<
        parameters<
            tag::x
          , optional<
                deduced<tag::y>
              , is_same<
                    mpl::_1
                  , tag::x::_
                > 
            >
        >
    >(
        (_x = 0U, _y = 1U)
      , 0U
      , 1U
    );

    check<
        parameters<
            tag::x
          , optional<
                deduced<tag::y>
              , is_same<
                    mpl::_1
                  , tag::x::_1
                > 
            >
        >
    >(
        (_x = 0U, _y = 1U)
      , 0U
      , 1U
    );

    return 0;
}

