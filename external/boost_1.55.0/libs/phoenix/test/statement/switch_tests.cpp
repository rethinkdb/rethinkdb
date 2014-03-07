/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <boost/detail/lightweight_test.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/statement.hpp>
#include <boost/phoenix/operator.hpp>

int
main()
{
    using boost::phoenix::arg_names::_1;
    using boost::phoenix::case_;
    using boost::phoenix::switch_;
    using boost::phoenix::ref;
    using boost::phoenix::val;

    using std::cout;
    using std::endl;
    using std::vector;
    using std::for_each;

    int init[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    vector<int> v(init, init+10);

    for_each(v.begin(), v.end(),
        switch_(_1)
        [
            // wierd case, why not just use if(...), but valid, nonetheless
            case_<4>(cout << val("<4>") << endl)
        ]
    );

    cout << endl;
    for_each(v.begin(), v.end(),
        switch_(_1)
        [
            // wierd case, but valid, nonetheless
            default_(cout << val("<any...>") << endl)
        ]
    );

    cout << endl;
    
    for_each(v.begin(), v.end(),
        switch_(_1)
        [
            case_<1>(cout << val("<1>") << endl),
            case_<2>(cout << val("<2>") << endl),
            case_<3>(cout << val("<3>") << endl),
            case_<4>(cout << val("<4>") << endl)
        ]
    );

    cout << endl;

    for_each(v.begin(), v.end(),
        switch_(_1)
        [
            case_<1>(cout << val("<1>") << endl),
            case_<2>(cout << val("<2>") << endl),
            case_<3>(cout << val("<3>") << endl),
            case_<4>(cout << val("<4>") << endl),
            default_(cout << val("<over 4>") << endl)
        ]
    );

    return boost::report_errors();
}
