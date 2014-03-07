/* chi_squared_test.hpp header file
 *
 * Copyright Steven Watanabe 2010
 * Distributed under the Boost Software License, Version 1.0. (See
 * accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * $Id: chi_squared_test.hpp 71018 2011-04-05 21:27:52Z steven_watanabe $
 *
 */

#ifndef BOOST_RANDOM_TEST_CHI_SQUARED_TEST_HPP_INCLUDED
#define BOOST_RANDOM_TEST_CHI_SQUARED_TEST_HPP_INCLUDED

#include <vector>

#include <boost/math/special_functions/pow.hpp>
#include <boost/math/distributions/chi_squared.hpp>

// This only works for discrete distributions with fixed
// upper and lower bounds.

template<class IntType>
struct chi_squared_collector {

    static const IntType cutoff = 5;

    chi_squared_collector()
      : chi_squared(0),
        variables(0),
        prev_actual(0),
        prev_expected(0),
        current_actual(0),
        current_expected(0)
    {}

    void operator()(IntType actual, double expected) {
        current_actual += actual;
        current_expected += expected;

        if(current_expected >= cutoff) {
            if(prev_expected != 0) {
                update(prev_actual, prev_expected);
            }
            prev_actual = current_actual;
            prev_expected = current_expected;

            current_actual = 0;
            current_expected = 0;
        }
    }

    void update(IntType actual, double expected) {
        chi_squared += boost::math::pow<2>(actual - expected) / expected;
        ++variables;
    }

    double cdf() {
        if(prev_expected != 0) {
            update(prev_actual + current_actual, prev_expected + current_expected);
            prev_actual = 0;
            prev_expected = 0;
            current_actual = 0;
            current_expected = 0;
        }
        if(variables <= 1) {
            return 0;
        } else {
            return boost::math::cdf(boost::math::chi_squared(variables - 1), chi_squared);
        }
    }

    double chi_squared;
    std::size_t variables;
    
    IntType prev_actual;
    double prev_expected;
    
    IntType current_actual;
    double current_expected;
};

template<class IntType>
double chi_squared_test(const std::vector<IntType>& results, const std::vector<double>& probabilities, IntType iterations) {
    chi_squared_collector<IntType> calc;
    for(std::size_t i = 0; i < results.size(); ++i) {
        calc(results[i], iterations * probabilities[i]);
    }
    return calc.cdf();
}

#endif
