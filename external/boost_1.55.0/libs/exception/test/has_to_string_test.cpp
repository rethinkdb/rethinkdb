//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/to_string.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace
n1
    {
    struct
    c1
        {
        };
    }

namespace
n2
    {
    struct
    c2
        {
        };

    std::string
    to_string( c2 const & )
        {
        return "c2";
        }           
    }

namespace
n3
    {
    struct
    c3
        {
        };

    std::ostream &
    operator<<( std::ostream & s, c3 const & )
        {
        return s << "c3";
        }           
    }

int
main()
    {
    using namespace boost;
    BOOST_TEST( !has_to_string<n1::c1>::value );
    BOOST_TEST( has_to_string<n2::c2>::value );
    BOOST_TEST( has_to_string<n3::c3>::value );
    BOOST_TEST( has_to_string<int>::value );
    return boost::report_errors();
    }
