/*=============================================================================
    Copyright (c) 2011 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/phoenix.hpp>

template <typename> struct wrap {};

int main()
{
    using boost::phoenix::val;
    using boost::phoenix::lambda;
    using boost::phoenix::let;
    using boost::phoenix::construct;
    using boost::phoenix::placeholders::_1;
    using boost::phoenix::local_names::_a;

    int const n = 10;
    std::vector<int> v1(n);

    let(_a = construct<int>(0))
    [
        generate(_1, lambda(_a = ref(_a))[_a++])
      , std::cout << val("result:\n")
      , for_each(_1, lambda[std::cout << _1 << ' '])
      , std::cout << val('\n')
    ](v1);
}
