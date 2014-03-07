//  (C) Copyright Gennadiy Rozental 2001-2008.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>
#include <iterator>

namespace boost {

template<typename T>
inline std::ostream&
operator<<( std::ostream& ostr, boost::optional<T> const& v )
{
    if( v )
        ostr << *v;

    return ostr;
}

}

int main() {
    {
        char* argv[] = { "basic", "-abcd", "-klmn", "10" };
        int argc = sizeof(argv)/sizeof(char*);

        try {
            cla::parser P;

            P << cla::named_parameter<int>( "abcd" ) - cla::optional_value
              << cla::named_parameter<int>( "klmn" );

            P.parse( argc, argv );

            boost::optional<int> v = P.get<boost::optional<int> >( "abcd" );

            std::cout << "abcd " << ( v ? "present" : "not present" ) << std::endl;
            std::cout << "klmn = " << P.get<int>( "klmn" ) << std::endl;
        }
        catch( rt::logic_error const& ex ) {
            std::cout << "Logic error: " << ex.msg() << std::endl;
            return -1;
        }
    }
    {
        char* argv[] = { "basic", "-abcd", "25", "-klmn", "10" };
        int argc = sizeof(argv)/sizeof(char*);

        try {
            cla::parser P;

            P << cla::named_parameter<int>( "abcd" ) - cla::optional_value
              << cla::named_parameter<int>( "klmn" );

            P.parse( argc, argv );

            boost::optional<int> v = P.get<boost::optional<int> >( "abcd" );

            if( v )
                std::cout << "*******\n""abcd = " << *v << std::endl;

            std::cout << "klmn = " << P.get<int>( "klmn" ) << std::endl;
        }
        catch( rt::logic_error const& ex ) {
            std::cout << "Logic error: " << ex.msg() << std::endl;
            return -1;
        }
    }
    {
        char* argv[] = { "basic", "-abcd", "25", "-abcd", "-abcd", "10" };
        int argc = sizeof(argv)/sizeof(char*);

        try {
            cla::parser P;

            P << cla::named_parameter<int>( "abcd" ) - (cla::optional_value, cla::multiplicable);

            P.parse( argc, argv );

            typedef std::list<boost::optional<int> > value_type;
            value_type v = P.get<value_type>( "abcd" );

            std::cout << "*******\n""abcd = ";
            std::copy( v.begin(), v.end(), std::ostream_iterator<boost::optional<int> >( std::cout, ", " ) );
            std::cout << std::endl;
        }
        catch( rt::logic_error const& ex ) {
            std::cout << "Logic error: " << ex.msg() << std::endl;
            return -1;
        }
    }

    return 0;
}

// EOF
