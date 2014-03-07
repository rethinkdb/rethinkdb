//Copyright (c) 2006-2009 Emil Dotchevski and Reverge Studios, Inc.

//Distributed under the Boost Software License, Version 1.0. (See accompanying
//file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/exception/detail/is_output_streamable.hpp>

namespace
n1
    {
    class
    c1
        {
        };
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

template <bool Test>
struct test;

template <>
struct
test<true>
    {
    };

int
main()
    {
    test<!boost::is_output_streamable<n1::c1>::value>();
    test<boost::is_output_streamable<n2::c2>::value>();
    test<boost::is_output_streamable<int>::value>();
    return 0;
    }
