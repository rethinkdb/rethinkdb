/* test_piecewise_linear.cpp
 *
 * Copyright Steven Watanabe 2011
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: test_piecewise_linear.cpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#include <boost/random/piecewise_linear_distribution.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/range/algorithm/lower_bound.hpp>
#include <boost/range/numeric.hpp>
#include <vector>
#include <iostream>
#include <iomanip>

#include "statistic_tests.hpp"

class piecewise_linear
{
public:
    piecewise_linear(const std::vector<double>& intervals, const std::vector<double>& weights)
      : intervals(intervals),
        weights(weights),
        cumulative(1, 0.0)
    {
        for(std::size_t i = 0; i < weights.size() - 1; ++i) {
            cumulative.push_back((weights[i] + weights[i + 1]) / 2);
        }
        boost::partial_sum(cumulative, cumulative.begin());
        double sum = cumulative.back();
        for(std::vector<double>::iterator iter = cumulative.begin(), end = cumulative.end();
            iter != end; ++iter)
        {
            *iter /= sum;
        }
        for(std::vector<double>::iterator iter = this->weights.begin(), end = this->weights.end();
            iter != end; ++iter)
        {
            *iter /= sum;
        }
        assert(this->weights.size() == this->intervals.size());
        assert(this->weights.size() == this->cumulative.size());
    }

    double cdf(double x) const
    {
        std::size_t index = boost::lower_bound(intervals, x) - intervals.begin();
        if(index == 0) return 0;
        else if(index == intervals.size()) return 1;
        else {
            double start = cumulative[index - 1];
            double lower_weight = weights[index - 1];
            double upper_weight = weights[index];
            double lower = intervals[index - 1];
            double upper = intervals[index];
            double mid_weight = (lower_weight * (upper - x) + upper_weight * (x - lower)) / (upper - lower);
            double segment_area = (x - lower) * (mid_weight + lower_weight) / 2;
            return start + segment_area;
        }
    }
private:
    std::vector<double> intervals;
    std::vector<double> weights;
    std::vector<double> cumulative;
};

double cdf(const piecewise_linear& dist, double x)
{
    return dist.cdf(x);
}

bool do_test(int n, int max) {
    std::cout << "running piecewise_linear(p0, p1, ..., p" << n-1 << ")" << " " << max << " times: " << std::flush;

    std::vector<double> weights;
    {
        boost::mt19937 egen;
        for(int i = 0; i < n; ++i) {
            weights.push_back(egen());
        }
    }
    std::vector<double> intervals;
    for(int i = 0; i < n; ++i) {
        intervals.push_back(i);
    }
    
    piecewise_linear expected(intervals, weights);
    
    boost::random::piecewise_linear_distribution<> dist(intervals, weights);
    boost::mt19937 gen;
    kolmogorov_experiment test(max);
    boost::variate_generator<boost::mt19937&, boost::random::piecewise_linear_distribution<> > vgen(gen, dist);

    double prob = test.probability(test.run(vgen, expected));

    bool result = prob < 0.99;
    const char* err = result? "" : "*";
    std::cout << std::setprecision(17) << prob << err << std::endl;

    std::cout << std::setprecision(6);

    return result;
}

bool do_tests(int repeat, int max_n, int trials) {
    boost::mt19937 gen;
    boost::uniform_int<> idist(2, max_n);
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
    std::cerr << "Usage: test_piecewise_linear -r <repeat> -n <max n> -t <trials>" << std::endl;
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
