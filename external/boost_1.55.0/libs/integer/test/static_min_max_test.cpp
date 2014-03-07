//  Boost static_min_max.hpp test program  -----------------------------------//

//  (C) Copyright Daryle Walker 2001.
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

//  Revision History
//  23 Sep 2001  Initial version (Daryle Walker)

#include <boost/detail/lightweight_test.hpp>  // for main, BOOST_TEST

#include <boost/cstdlib.hpp>                 // for boost::exit_success
#include <boost/integer/static_min_max.hpp>  // for boost::static_signed_min, etc.

#include <iostream>  // for std::cout (std::endl indirectly)


// Main testing function
int
main
(
    int         ,   // "argc" is unused
    char *      []  // "argv" is unused
)
{    
    using std::cout;
    using std::endl;
    using boost::static_signed_min;
    using boost::static_signed_max;
    using boost::static_unsigned_min;
    using boost::static_unsigned_max;

    // Two positives
    cout << "Doing tests with two positive values." << endl;

    BOOST_TEST( (static_signed_min< 9, 14>::value) ==  9 );
    BOOST_TEST( (static_signed_max< 9, 14>::value) == 14 );
    BOOST_TEST( (static_signed_min<14,  9>::value) ==  9 );
    BOOST_TEST( (static_signed_max<14,  9>::value) == 14 );

    BOOST_TEST( (static_unsigned_min< 9, 14>::value) ==  9 );
    BOOST_TEST( (static_unsigned_max< 9, 14>::value) == 14 );
    BOOST_TEST( (static_unsigned_min<14,  9>::value) ==  9 );
    BOOST_TEST( (static_unsigned_max<14,  9>::value) == 14 );

    // Two negatives
    cout << "Doing tests with two negative values." << endl;

    BOOST_TEST( (static_signed_min<  -8, -101>::value) == -101 );
    BOOST_TEST( (static_signed_max<  -8, -101>::value) ==   -8 );
    BOOST_TEST( (static_signed_min<-101,   -8>::value) == -101 );
    BOOST_TEST( (static_signed_max<-101,   -8>::value) ==   -8 );

    // With zero
    cout << "Doing tests with zero and a positive or negative value." << endl;

    BOOST_TEST( (static_signed_min< 0, 14>::value) ==  0 );
    BOOST_TEST( (static_signed_max< 0, 14>::value) == 14 );
    BOOST_TEST( (static_signed_min<14,  0>::value) ==  0 );
    BOOST_TEST( (static_signed_max<14,  0>::value) == 14 );

    BOOST_TEST( (static_unsigned_min< 0, 14>::value) ==  0 );
    BOOST_TEST( (static_unsigned_max< 0, 14>::value) == 14 );
    BOOST_TEST( (static_unsigned_min<14,  0>::value) ==  0 );
    BOOST_TEST( (static_unsigned_max<14,  0>::value) == 14 );

    BOOST_TEST( (static_signed_min<   0, -101>::value) == -101 );
    BOOST_TEST( (static_signed_max<   0, -101>::value) ==    0 );
    BOOST_TEST( (static_signed_min<-101,    0>::value) == -101 );
    BOOST_TEST( (static_signed_max<-101,    0>::value) ==    0 );

    // With identical
    cout << "Doing tests with two identical values." << endl;

    BOOST_TEST( (static_signed_min<0, 0>::value) == 0 );
    BOOST_TEST( (static_signed_max<0, 0>::value) == 0 );
    BOOST_TEST( (static_unsigned_min<0, 0>::value) == 0 );
    BOOST_TEST( (static_unsigned_max<0, 0>::value) == 0 );

    BOOST_TEST( (static_signed_min<14, 14>::value) == 14 );
    BOOST_TEST( (static_signed_max<14, 14>::value) == 14 );
    BOOST_TEST( (static_unsigned_min<14, 14>::value) == 14 );
    BOOST_TEST( (static_unsigned_max<14, 14>::value) == 14 );

    BOOST_TEST( (static_signed_min< -101, -101>::value) == -101 );
    BOOST_TEST( (static_signed_max< -101, -101>::value) == -101 );

    return boost::report_errors();
}
