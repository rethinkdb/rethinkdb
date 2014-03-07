/* test_bernoulli.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_bernoulli.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/bernoulli_distribution.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_01.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/math/distributions/binomial.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <vector>
#include <iostream>
#include <numeric>

#include "chi_squared_test.hpp"

bool do_test(double p, long long max) {
    std::cout << "running bernoulli(" << p << ")" << " " << max << " times: " << std::flush;

    boost::math::binomial expected(static_cast<double>(max), p);
    
    boost::random::bernoulli_distribution<> dist(p);
    boost::mt19937 gen;
    long long count = 0;
    for(long long i = 0; i < max; ++i) {
        if(dist(gen)) ++count;
    }

    double prob = cdf(expected, count);

    bool result = prob < 0.99 && prob > 0.01;
    const char* err = result? "" : "*";
    std::cout << std::setprecision(17) << prob << err << std::endl;

    std::cout << std::setprecision(6);

    return result;
}

bool do_tests(int repeat, long long trials) {
    boost::mt19937 gen;
    boost::uniform_01<> rdist;
    int errors = 0;
    for(int i = 0; i < repeat; ++i) {
        if(!do_test(rdist(gen), trials)) {
            ++errors;
        }
    }
    if(errors != 0) {
        std::cout << "*** " << errors << " errors detected ***" << std::endl;
    }
    return errors == 0;
}

int usage() {
    std::cerr << "Usage: test_bernoulli_distribution -r <repeat> -t <trials>" << std::endl;
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
    long long trials = 1000000ll;

    if(argc > 0) {
        --argc;
        ++argv;
    }
    while(argc > 0) {
        if(argv[0][0] != '-') return usage();
        else if(!handle_option(argc, argv, 'r', repeat)
             && !handle_option(argc, argv, 't', trials)) {
            return usage();
        }
        --argc;
        ++argv;
    }

    try {
        if(do_tests(repeat, trials)) {
            return 0;
        } else {
            return EXIT_FAILURE;
        }
    } catch(...) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return EXIT_FAILURE;
    }
}
