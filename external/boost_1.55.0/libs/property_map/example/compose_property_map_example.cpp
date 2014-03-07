// Copyright (C) 2013 Eurodecision
// Authors: Guillaume Pinot
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <boost/property_map/compose_property_map.hpp>
#include <iostream>

int main()
{
    const int idx[] = {2, 0, 4, 1, 3};
    double v[] = {1., 3., 0., 4., 2.};
    boost::compose_property_map<double*, const int*> cpm(v, idx);

    for (int i = 0; i < 5; ++i)
        std::cout << get(cpm, i) << " ";
    std::cout << std::endl;

    for (int i = 0; i < 5; ++i)
        ++cpm[i];

    for (int i = 0; i < 5; ++i)
        std::cout << get(cpm, i) << " ";
    std::cout << std::endl;

    for (int i = 0; i < 5; ++i)
        put(cpm, i, 42.);

    for (int i = 0; i < 5; ++i)
        std::cout << get(cpm, i) << " ";
    std::cout << std::endl;

    return 0;
}
