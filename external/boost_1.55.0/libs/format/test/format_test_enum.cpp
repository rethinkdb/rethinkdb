// ------------------------------------------------------------------------------
//  format_test_enum.cpp :  test format use with enums
// ------------------------------------------------------------------------------

//  Copyright Steven Watanabe 2009.
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/format for library home page

// ------------------------------------------------------------------------------

#include "boost/format.hpp"

#define BOOST_INCLUDE_MAIN 
#include <boost/test/test_tools.hpp>

enum enum_plain { PLAIN };
enum { ANONYMOUS };
enum enum_overloaded { OVERLOADED };
typedef enum { OVERLOADED_TYPEDEF } enum_overloaded_typedef;

std::ostream& operator<<(std::ostream& os, enum_overloaded) {
    os << "overloaded";
    return(os);
}

std::ostream& operator<<(std::ostream& os, enum_overloaded_typedef) {
    os << "overloaded";
    return(os);
}

int test_main(int, char*[]) {
    // in this case, we should implicitly convert to int
    BOOST_CHECK_EQUAL((boost::format("%d") % PLAIN).str(), "0");
    BOOST_CHECK_EQUAL((boost::format("%d") % ANONYMOUS).str(), "0");

    // but here we need to use the overloaded operator
    BOOST_CHECK_EQUAL((boost::format("%s") % OVERLOADED).str(), "overloaded");
    BOOST_CHECK_EQUAL((boost::format("%s") % OVERLOADED_TYPEDEF).str(), "overloaded");

    return 0;
}
