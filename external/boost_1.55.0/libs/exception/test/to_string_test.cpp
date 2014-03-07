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
    BOOST_TEST( "c2"==to_string(n2::c2()) );
    BOOST_TEST( "c3"==to_string(n3::c3()) );
    BOOST_TEST( "42"==to_string(42) );
    return boost::report_errors();
    }
