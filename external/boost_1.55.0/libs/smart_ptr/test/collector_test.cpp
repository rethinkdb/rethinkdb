//
//  collector_test.cpp
//
//  Copyright (c) 2003 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/shared_ptr.hpp>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <ctime>

// sp_collector.cpp exported functions

std::size_t find_unreachable_objects(bool report);
void free_unreachable_objects();

struct X
{
    void* fill[32];
    boost::shared_ptr<X> p;
};

void report()
{
    std::cout << "Calling find_unreachable_objects:\n";

    std::clock_t t = std::clock();

    std::size_t n = find_unreachable_objects(false);

    t = std::clock() - t;

    std::cout << n << " unreachable objects.\n";
    std::cout << "  " << static_cast<double>(t) / CLOCKS_PER_SEC << " seconds.\n";
}

void free()
{
    std::cout << "Calling free_unreachable_objects:\n";

    std::clock_t t = std::clock();

    free_unreachable_objects();

    t = std::clock() - t;

    std::cout << "  " << static_cast<double>(t) / CLOCKS_PER_SEC << " seconds.\n";
}

int main()
{
    std::vector< boost::shared_ptr<X> > v1, v2;

    int const n = 256 * 1024;

    std::cout << "Filling v1 and v2\n";

    for(int i = 0; i < n; ++i)
    {
        v1.push_back(boost::shared_ptr<X>(new X));
        v2.push_back(boost::shared_ptr<X>(new X));
    }

    report();

    std::cout << "Creating the cycles\n";

    for(int i = 0; i < n - 1; ++i)
    {
        v2[i]->p = v2[i+1];
    }

    v2[n-1]->p = v2[0];

    report();

    std::cout << "Resizing v2 to size of 1\n";

    v2.resize(1);
    report();

    std::cout << "Clearing v2\n";

    v2.clear();
    report();

    std::cout << "Clearing v1\n";

    v1.clear();
    report();

    free();
    report();
}
