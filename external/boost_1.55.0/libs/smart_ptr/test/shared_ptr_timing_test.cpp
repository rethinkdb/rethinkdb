#include <boost/config.hpp>

#if defined(BOOST_MSVC)
#pragma warning(disable: 4786)  // identifier truncated in debug info
#pragma warning(disable: 4710)  // function not inlined
#pragma warning(disable: 4711)  // function selected for automatic inline expansion
#pragma warning(disable: 4514)  // unreferenced inline removed
#endif

//
//  shared_ptr_timing_test.cpp - use to evaluate the impact of thread safety
//
//  Copyright (c) 2002 Peter Dimov and Multi Media Ltd.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/shared_ptr.hpp>
#include <iostream>
#include <vector>
#include <ctime>

int const n = 8 * 1024 * 1024;

int main()
{
    using namespace std;

    std::vector< boost::shared_ptr<int> > v;
    boost::shared_ptr<int> pi(new int);

    clock_t t = clock();

    for(int i = 0; i < n; ++i)
    {
        v.push_back(pi);
    }

    t = clock() - t;

    std::cout << static_cast<double>(t) / CLOCKS_PER_SEC << '\n';

    return 0;
}
