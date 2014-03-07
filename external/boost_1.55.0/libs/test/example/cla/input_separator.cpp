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

struct FullName
{
    std::string first_name;
    std::string last_name;
};

std::istream&
operator>>( std::istream& istr, FullName& f )
{
    return istr >> f.first_name >> std::ws >> f.last_name;
}

std::ostream&
operator<<( std::ostream& ostr, FullName const& f )
{
    return ostr << f.first_name << ' ' << f.last_name;
}

int main() {
    char* argv[] = { "basic", "name=John Doe:name=Joan Doe:name=Your Name" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;

        P - ( cla::input_separator = ':' )
            << cla::named_parameter<FullName>( "name" ) - ( cla::multiplicable, cla::separator = "=", cla::prefix = "" );

        P.parse( argc, argv );

        std::list<FullName> arg_values = P.get<std::list<FullName> >( "name" );

        std::cout << "names: ";
        std::copy( arg_values.begin(), arg_values.end(), std::ostream_iterator<FullName>( std::cout, ", " ) );
        std::cout << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
