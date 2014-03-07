/*=============================================================================
    Copyright (c) 1998-2003 Joel de Guzman
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
///////////////////////////////////////////////////////////////////////////////
//
//  This sample demonstrates error handling as seen in the
//  Error Handling" chapter in the User's Guide.
//
///////////////////////////////////////////////////////////////////////////////
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_exceptions.hpp>
#include <iostream>
#include <boost/assert.hpp>

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

struct handler
{
    template <typename ScannerT, typename ErrorT>
    error_status<>
    operator()(ScannerT const& /*scan*/, ErrorT const& /*error*/) const
    {
        cout << "exception caught...Test concluded successfully" << endl;
        return error_status<>(error_status<>::fail);
    }
};

int
main()
{
    cout << "/////////////////////////////////////////////////////////\n\n";
    cout << "\t\tExceptions Test...\n\n";
    cout << "/////////////////////////////////////////////////////////\n\n";

    assertion<int>  expect(0);
    guard<int>      my_guard;
    
    rule<> start = 
        my_guard(ch_p('a') >> 'b' >> 'c' >> expect( ch_p('d') ))
        [
            handler()
        ];

    bool r = parse("abcx", start).full;

    BOOST_ASSERT(!r);
    return 0;
}

