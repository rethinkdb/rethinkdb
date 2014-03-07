/* test_real_distribution.ipp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_real_distribution.ipp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#ifndef BOOST_MATH_DISTRIBUTION_INIT
#ifdef BOOST_RANDOM_ARG2_TYPE
#define BOOST_MATH_DISTRIBUTION_INIT (BOOST_RANDOM_ARG1_NAME, BOOST_RANDOM_ARG2_NAME)
#else
#define BOOST_MATH_DISTRIBUTION_INIT (BOOST_RANDOM_ARG1_NAME)
#endif
#endif

#ifndef BOOST_RANDOM_DISTRIBUTION_INIT
#ifdef BOOST_RANDOM_ARG2_TYPE
#define BOOST_RANDOM_DISTRIBUTION_INIT (BOOST_RANDOM_ARG1_NAME, BOOST_RANDOM_ARG2_NAME)
#else
#define BOOST_RANDOM_DISTRIBUTION_INIT (BOOST_RANDOM_ARG1_NAME)
#endif
#endif

#ifndef BOOST_RANDOM_P_CUTOFF
#define BOOST_RANDOM_P_CUTOFF 0.99
#endif

#include <boost/random/mersenne_twister.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/range/numeric.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <iostream>
#include <vector>

#include "statistic_tests.hpp"
#include "chi_squared_test.hpp"

bool do_test(BOOST_RANDOM_ARG1_TYPE BOOST_RANDOM_ARG1_NAME,
#ifdef BOOST_RANDOM_ARG2_TYPE
             BOOST_RANDOM_ARG2_TYPE BOOST_RANDOM_ARG2_NAME,
#endif
             long long max, boost::mt19937& gen) {
    std::cout << "running " BOOST_PP_STRINGIZE(BOOST_RANDOM_DISTRIBUTION_NAME) "("
        << BOOST_RANDOM_ARG1_NAME;
#ifdef BOOST_RANDOM_ARG2_NAME
    std::cout << ", " << BOOST_RANDOM_ARG2_NAME;
#endif
    std::cout << ")" << " " << max << " times: " << std::flush;

    BOOST_MATH_DISTRIBUTION expected BOOST_MATH_DISTRIBUTION_INIT;
    
    BOOST_RANDOM_DISTRIBUTION dist BOOST_RANDOM_DISTRIBUTION_INIT;

#ifdef BOOST_RANDOM_DISTRIBUTION_MAX

    BOOST_RANDOM_DISTRIBUTION::result_type max_value = BOOST_RANDOM_DISTRIBUTION_MAX;

    std::vector<double> expected_pdf(max_value+1);
    {
        for(int i = 0; i <= max_value; ++i) {
            expected_pdf[i] = pdf(expected, i);
        }
        expected_pdf.back() += 1 - boost::accumulate(expected_pdf, 0.0);
    }
    
    std::vector<long long> results(max_value + 1);
    for(long long i = 0; i < max; ++i) {
        ++results[(std::min)(dist(gen), max_value)];
    }

    long long sum = boost::accumulate(results, 0ll);
    if(sum != max) {
        std::cout << "*** Failed: incorrect total: " << sum << " ***" << std::endl;
        return false;
    }
    double prob = chi_squared_test(results, expected_pdf, max);

#else

    kolmogorov_experiment test(boost::numeric_cast<int>(max));
    boost::variate_generator<boost::mt19937&, BOOST_RANDOM_DISTRIBUTION > vgen(gen, dist);

    double prob = test.probability(test.run(vgen, expected));

#endif

    bool result = prob < BOOST_RANDOM_P_CUTOFF;
    const char* err = result? "" : "*";
    std::cout << std::setprecision(17) << prob << err << std::endl;

    std::cout << std::setprecision(6);

    return result;
}

template<class Dist1
#ifdef BOOST_RANDOM_ARG2_NAME
       , class Dist2
#endif
>
bool do_tests(int repeat, Dist1 d1,
#ifdef BOOST_RANDOM_ARG2_NAME
              Dist2 d2,
#endif
              long long trials) {
    boost::mt19937 gen;
    int errors = 0;
    for(int i = 0; i < repeat; ++i) {
        if(!do_test(d1(gen),
#ifdef BOOST_RANDOM_ARG2_NAME
            d2(gen),
#endif
            trials, gen)) {
            ++errors;
        }
    }
    if(errors != 0) {
        std::cout << "*** " << errors << " errors detected ***" << std::endl;
    }
    return errors == 0;
}

int usage() {
    std::cerr << "Usage: test_" BOOST_PP_STRINGIZE(BOOST_RANDOM_DISTRIBUTION_NAME)
        " -r <repeat>"
        " -" BOOST_PP_STRINGIZE(BOOST_RANDOM_ARG1_NAME)
        " <max " BOOST_PP_STRINGIZE(BOOST_RANDOM_ARG1_NAME) ">"
#ifdef BOOST_RANDOM_ARG2_NAME
        " -" BOOST_PP_STRINGIZE(BOOST_RANDOM_ARG2_NAME)
        " <max " BOOST_PP_STRINGIZE(BOOST_RANDOM_ARG2_NAME) ">"
#endif
        " -t <trials>" << std::endl;
    return 2;
}

template<class T>
bool handle_option(int& argc, char**& argv, const char* opt, T& value) {
    if(std::strcmp(argv[0], opt) == 0 && argc > 1) {
        --argc;
        ++argv;
        value = boost::lexical_cast<T>(argv[0]);
        return true;
    } else {
        return false;
    }
}

int main(int argc, char** argv) {
    int repeat = 1;
    BOOST_RANDOM_ARG1_TYPE max_arg1 = BOOST_RANDOM_ARG1_DEFAULT;
#ifdef BOOST_RANDOM_ARG2_TYPE
    BOOST_RANDOM_ARG2_TYPE max_arg2 = BOOST_RANDOM_ARG2_DEFAULT;
#endif
    long long trials = 100000;

    if(argc > 0) {
        --argc;
        ++argv;
    }
    while(argc > 0) {
        if(argv[0][0] != '-') return usage();
        else if(!handle_option(argc, argv, "-r", repeat)
             && !handle_option(argc, argv, "-" BOOST_PP_STRINGIZE(BOOST_RANDOM_ARG1_NAME), max_arg1)
#ifdef BOOST_RANDOM_ARG2_TYPE
             && !handle_option(argc, argv, "-" BOOST_PP_STRINGIZE(BOOST_RANDOM_ARG2_NAME), max_arg2)
#endif
             && !handle_option(argc, argv, "-t", trials)) {
            return usage();
        }
        --argc;
        ++argv;
    }

    try {
        if(do_tests(repeat,
                    BOOST_RANDOM_ARG1_DISTRIBUTION(max_arg1),
#ifdef BOOST_RANDOM_ARG2_TYPE
                    BOOST_RANDOM_ARG2_DISTRIBUTION(max_arg2),
#endif
                    trials)) {
            return 0;
        } else {
            return EXIT_FAILURE;
        }
    } catch(...) {
        std::cerr << boost::current_exception_diagnostic_information() << std::endl;
        return EXIT_FAILURE;
    }
}
