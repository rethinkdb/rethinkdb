/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_function.hpp>

using namespace boost::phoenix;
using namespace boost::phoenix::arg_names;
using namespace std;

struct is_odd_
{
    template <typename Arg>
    struct result
    {
        typedef bool type;
    };

    template <typename Arg>
    bool operator()(Arg arg1) const
    {
        return arg1 % 2 != 0;
    }
};

function<is_odd_> is_odd;

int
main()
{
    int init[] = { 2, 10, 4, 5, 1, 6, 8, 3, 9, 7 };
    vector<int> c(init, init + 10);
    typedef vector<int>::iterator iterator;

    //  Find the first odd number in container c
    iterator it = find_if(c.begin(), c.end(), is_odd(arg1));

    if (it != c.end())
        cout << *it;    //  if found, print the result
    return 0;
}
