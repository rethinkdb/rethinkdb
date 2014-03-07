//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/to_string_stub.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <iostream>

namespace
n1
    {
    class
    c1
        {
        };

    std::string
    to_string( c1 const & )
        {
        return "c1";
        }           
    }

namespace
n2
    {
    class
    c2
        {
        };

    std::ostream &
    operator<<( std::ostream & s, c2 const & )
        {
        s << "c2";
        return s;
        }           
    }

namespace
n3
    {
    class
    c3
        {
        };

    std::ostream &
    operator<<( std::ostream & s, c3 const & )
        {
        s << "bad";
        return s;
        }           

    std::string
    to_string( c3 const & )
        {
        return "c3";
        }           
    }

namespace
boost
    {
    class
    to_string_tester
        {
        };
    }

template <class T>
struct
my_stub
    {
    std::string
    operator()( T const & )
        {
        return "stub";
        }
    };

int
main()
    {
    using namespace boost;
    BOOST_TEST( to_string(42)=="42" );
    BOOST_TEST( to_string(n1::c1())=="c1" );
    BOOST_TEST( to_string(n2::c2())=="c2" );
    BOOST_TEST( to_string(n3::c3())=="c3" );
    BOOST_TEST( to_string_stub(42)=="42" );
    BOOST_TEST( to_string_stub(n1::c1())=="c1" );
    BOOST_TEST( to_string_stub(n2::c2())=="c2" );
    BOOST_TEST( to_string_stub(n3::c3())=="c3" );
    BOOST_TEST( to_string_stub(to_string_tester(),my_stub<to_string_tester>())=="stub" );
    BOOST_TEST( !to_string_stub(to_string_tester()).empty() );
    return boost::report_errors();
    }
