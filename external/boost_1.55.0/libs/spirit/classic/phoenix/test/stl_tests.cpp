/*=============================================================================
    Phoenix V1.2.1
    Copyright (c) 2001-2003 Joel de Guzman

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <iostream>
#include <vector>
#include <list>
#include <algorithm>
#include <boost/detail/lightweight_test.hpp>

#define PHOENIX_LIMIT 15
#include <boost/spirit/include/phoenix1_primitives.hpp>
#include <boost/spirit/include/phoenix1_composite.hpp>
#include <boost/spirit/include/phoenix1_functions.hpp>
#include <boost/spirit/include/phoenix1_operators.hpp>
#include <boost/spirit/include/phoenix1_binders.hpp>
#include <boost/spirit/include/phoenix1_special_ops.hpp>

using namespace phoenix;
using namespace std;

    ///////////////////////////////////////////////////////////////////////////////
    struct print_ { // a typical STL style monomorphic functor

        typedef void result_type;
        void operator()(int n0)         { cout << "got 1 arg " << n0 << " \n"; }
    };

    functor<print_> print = print_();

///////////////////////////////////////////////////////////////////////////////
int
main()
{

///////////////////////////////////////////////////////////////////////////////
//
//  STL algorithms
//
///////////////////////////////////////////////////////////////////////////////
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(4);
    v.push_back(5);

    for_each(v.begin(), v.end(), arg1 *= 2);
    for (int m = 0; m < 5; ++m, (cout << ','))
    {
        cout << v[m];
        BOOST_TEST(v[m] == (m+1)*2);
    }
    cout << endl;

    for_each(v.begin(), v.end(), print(arg1));

    vector<int>::iterator it = find_if(v.begin(), v.end(), arg1 > 5);
    if (it != v.end())
        cout << *it << endl;

///////////////////////////////////////////////////////////////////////////////
//
//  STL iterators and containers
//
///////////////////////////////////////////////////////////////////////////////

    BOOST_TEST((arg1[0])(v) == v[0]);
    BOOST_TEST((arg1[1])(v) == v[1]);

    list<int> l;
    l.push_back(1);
    l.push_back(2);
    l.push_back(3);
    l.push_back(4);
    l.push_back(5);

    list<int>::iterator first = l.begin();
    list<int>::iterator last = l.end();

    BOOST_TEST((*(++arg1))(first) == 2);
    BOOST_TEST((*(----arg1))(last) == 4);

///////////////////////////////////////////////////////////////////////////////
//
//  End asserts
//
///////////////////////////////////////////////////////////////////////////////

    return boost::report_errors();    
}
