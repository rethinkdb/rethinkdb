/* test_piecewise_constant.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_piecewise_constant.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/piecewise_constant_distribution.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/numeric.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

#include "statistic_tests.hpp"

class piecewise_constant
{
public:
    piecewise_constant(const std::vector<double>& intervals, const std::vector<double>& weights)
      : intervals(intervals),
        cumulative(1, 0.0)
    {
        boost::partial_sum(weights, std::back_inserter(cumulative));
        for(std::vector<double>::iterator iter = cumulative.begin(), end = cumulative.end();
            iter != end; ++iter)
        {
            *iter /= cumulative.back();
        }
    }

    double cdf(double x) const
    {
        std::size_t index = boost::lower_bound(intervals, x) - intervals.begin();
        if(index == 0) return 0;
        else if(index == intervals.size()) return 1;
        else {
            double lower_weight = cumulative[index - 1];
            double upper_weight = cumulative[index];
            double lower = intervals[index - 1];
            double upper = intervals[index];
            return lower_weight + (x - lower) / (upper - lower) * (upper_weight - lower_weight);
        }
    }
private:
    std::vector<double> intervals;
    std::vector<double> cumulative;
};

double cdf(const piecewise_constant& dist, double x)
{
    return dist.cdf(x);
}

bool do_test(int n, int max) {
    std::cout << "running piecewise_constant(p0, p1, ..., p" << n-1 << ")" << " " << max << " times: " << std::flush;

    std::vector<double> weights;
    {
        boost::mt19937 egen;
        for(int i = 0; i < n; ++i) {
            weights.push_back(egen());
        }
    }
    std::vector<double> intervals;
    for(int i = 0; i <= n; ++i) {
        intervals.push_back(i);
    }

    piecewise_constant expected(intervals, weights);
    
    boost::random::piecewise_constant_distribution<> dist(intervals, weights);
    boost::mt19937 gen;
    kolmogorov_experiment test(max);
    boost::variate_generator<boost::mt19937&, boost::random::piecewise_constant_distribution<> > vgen(gen, dist);

    double prob = test.probability(test.run(vgen, expected));

    bool result = prob < 0.99;
    const char* err = result? "" : "*";
    std::cout << std::setprecision(17) << prob << err << std::endl;

    std::cout << std::setprecision(6);

    return result;
}

bool do_tests(int repeat, int max_n, int trials) {
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
    std::cerr << "Usage: test_piecewise_constant -r <repeat> -n <max n> -t <trials>" << std::endl;
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
    int max_n = 10;
    int trials = 1000000;

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
