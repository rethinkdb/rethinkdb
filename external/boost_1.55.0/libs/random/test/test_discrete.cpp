/* test_poisson.cpp
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_discrete.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/discrete_distribution.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <vector>
#include <iostream>
#include <numeric>

#include "chi_squared_test.hpp"

bool do_test(int n, long long max) {
    std::cout << "running discrete(p0, p1, ..., p" << n-1 << ")" << " " << max << " times: " << std::flush;

    std::vector<double> expected;
    {
        boost::mt19937 egen;
        for(int i = 0; i < n; ++i) {
            expected.push_back(egen());
        }
        double sum = std::accumulate(expected.begin(), expected.end(), 0.0);
        for(std::vector<double>::iterator iter = expected.begin(), end = expected.end(); iter != end; ++iter) {
            *iter /= sum;
        }
    }
    
    boost::random::discrete_distribution<> dist(expected);
    boost::mt19937 gen;
    std::vector<long long> results(expected.size());
    for(long long i = 0; i < max; ++i) {
        ++results[dist(gen)];
    }

    long long sum = std::accumulate(results.begin(), results.end(), 0ll);
    if(sum != max) {
        std::cout << "*** Failed: incorrect total: " << sum << " ***" << std::endl;
        return false;
    }
    double chsqr = chi_squared_test(results, expected, max);

    bool result = chsqr < 0.99;
    const char* err = result? "" : "*";
    std::cout << std::setprecision(17) << chsqr << err << std::endl;

    std::cout << std::setprecision(6);

    return result;
}

bool do_tests(int repeat, int max_n, long long trials) {
    boost::mt19937 gen;
    boost::uniform_int<> idist(1, max_n);
    int errors = 0;
    for(int i = 0; i < repeat; ++i) {
        if(!do_test(idist(gen), trials)) {
            ++errors;
        }
    }
    if(errors != 0) {
        std::cout << "*** " << errors << " errors detected ***" << std::endl;
    }
    return errors == 0;
}

int usage() {
    std::cerr << "Usage: test_discrete -r <repeat> -n <max n> -t <trials>" << std::endl;
    return 2;
}

template<class T>
bool handle_option(int& argc, char**& argv, char opt, T& value) {
    if(argv[0][1] == opt && argc > 1) {
        --argc;
        ++argv;
        value = boost::lexical_cast<T>(argv[0]);
        return true;
    } else {
        return false;
    }
}

int main(int argc, char** argv) {
    int repeat = 10;
    int max_n = 100000;
    long long trials = 1000000ll;

    if(argc > 0) {
        --argc;
        ++argv;
    }
    while(argc > 0) {
        if(argv[0][0] != '-') return usage();
        else if(!handle_option(argc, argv, 'r', repeat)
             && !handle_option(argc, argv, 'n', max_n)
             && !handle_option(argc, argv, 't', trials)) {
            return usage();
        }
        --argc;
        ++argv;
    }

    try {
        if(do_tests(repeat, max_n, trials)) {
            return 0;
        } else {
            return EXIT_FAILURE;
        }
    } catch(...) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return EXIT_FAILURE;
    }
}
