
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/local_function.hpp>
#include <boost/chrono.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include "profile_helpers.hpp"

int main(int argc, char* argv[]) {
    unsigned long size = 0, trials = 0;
    profile::args(argc, argv, size, trials);

    double sum = 0.0;
    int factor = 1;

    boost::chrono::system_clock::time_point start =
            boost::chrono::system_clock::now();
    void BOOST_LOCAL_FUNCTION(
            const double& num, bind& sum, const bind& factor) {
        sum += factor * num;
    } BOOST_LOCAL_FUNCTION_NAME(add)
    boost::chrono::duration<double> decl_sec =
            boost::chrono::system_clock::now() - start;

    std::vector<double> v(size);
    std::fill(v.begin(), v.end(), 1.0);

    boost::chrono::duration<double> trials_sec;
    for(unsigned long i = 0; i < trials; ++i) {
        boost::chrono::system_clock::time_point start =
                boost::chrono::system_clock::now();
        std::for_each(v.begin(), v.end(), add);
        trials_sec += boost::chrono::system_clock::now() - start;
    }

    profile::display(size, trials, sum, trials_sec.count(), decl_sec.count());
    return 0;
}

