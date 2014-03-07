
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#ifndef PROFILE_HELPERS_HPP_
#define PROFILE_HELPERS_HPP_

#include <iostream>
#include <cassert>

namespace profile {

void args(int argc, char* argv[], unsigned long& size, unsigned long& trials) {
    size = 100000000; // Defaults.
    trials = 10; // Default.
    if (argc != 1 && argc != 2 && argc != 3) {
        std::cerr << "ERROR: Incorrect argument(s)" << std::endl;
        std::cerr << "Usage: " << argv[0] << " [SIZE] [TRIALS]" <<
                std::endl;
        std::cerr << "Defaults: SIZE = " << double(size) << ", TRIALS = " <<
                double(trials) << std::endl;
        exit(1);
    }
    if (argc >= 2) size = atol(argv[1]);
    if (argc >= 3) trials = atol(argv[2]);

    std::clog << "vector size = " << double(size) << std::endl;
    std::clog << "number of trials = " << double(trials) << std::endl;
    std::clog << "number of calls = " << double(size) * double(trials) <<
            std::endl;
}

void display(const unsigned long& size, const unsigned long& trials,
        const double& sum, const double& trials_sec,
        const double& decl_sec = 0.0) {
    std::clog << "sum = " << sum << std::endl;
    std::clog << "declaration run-time [s] = " << decl_sec << std::endl;
    std::clog << "trials run-time [s] = " << trials_sec << std::endl;

    double avg_sec = decl_sec + trials_sec / trials;
    std::clog << "average run-time [s] = declaration run-time + trials " <<
                "run-time / number of trials = " << std::endl;
    std::cout << avg_sec << std::endl; // To cout so it can be parsed easily.

    assert(sum == double(size) * double(trials));
}

} // namespace

#endif // #include guard

