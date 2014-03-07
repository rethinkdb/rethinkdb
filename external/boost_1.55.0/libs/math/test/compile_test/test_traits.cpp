//  Copyright John Maddock 2007.

//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/math/tools/traits.hpp>
#include <boost/static_assert.hpp>
#include <boost/math/distributions.hpp>

using namespace boost::math;

BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<double>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<int>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<bernoulli>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<beta_distribution<> >::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<binomial>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<cauchy>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<chi_squared>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<exponential>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<extreme_value>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<fisher_f>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<gamma_distribution<> >::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<lognormal>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<negative_binomial>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<normal>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<pareto>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<poisson>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<rayleigh>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<students_t>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<triangular>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<uniform>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_distribution<weibull>::value);

BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<double>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<int>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<bernoulli>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<beta_distribution<> >::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<binomial>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<cauchy>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<chi_squared>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<exponential>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<extreme_value>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<fisher_f>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<gamma_distribution<> >::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<lognormal>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<negative_binomial>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<normal>::value);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<pareto>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<poisson>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<rayleigh>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<students_t>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<triangular>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<uniform>::value == false);
BOOST_STATIC_ASSERT(::boost::math::tools::is_scaled_distribution<weibull>::value == false);

