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

// Library code
struct FullName
{
    std::string first_name;
    std::string last_name;
};

std::istream&
operator>>( std::istream& istr, FullName& f )
{
    std::string token;
    istr >> token;

    std::size_t pos = token.find( ',' );
    if( pos != std::string::npos ) {
        f.first_name.assign( token.begin(), token.begin()+pos );
        f.last_name.assign( token.begin()+pos+1, token.end() );
    }
    else {
        f.first_name = token;
    }

    return istr;
}

std::ostream&
operator<<( std::ostream& ostr, FullName const& f )
{
    return ostr << f.first_name << ':' << f.last_name;
}

// End of library code

int main() {
    char* argv[] = { "basic", "-abcd", "John,Doe" };
    int argc = sizeof(argv)/sizeof(char*);

    try {
        cla::parser P;

        P << cla::named_parameter<FullName>( "abcd" );

        P.parse( argc, argv );

        std::cout << "abcd = " << P.get<FullName>( "abcd" ) << std::endl;
    }
    catch( rt::logic_error const& ex ) {
        std::cout << "Logic error: " << ex.msg() << std::endl;
        return -1;
    }

    return 0;
}

// EOF
