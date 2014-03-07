//  (C) Copyright 2009-2011 Frederic Bron.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"

#include <boost/type_traits/has_operator.hpp>
#include "has_postfix_classes.hpp"

TT_TEST_BEGIN(BOOST_TT_TRAIT_NAME)
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 &, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const &, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C045 const &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C046 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C047 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 &, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const &, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C050 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C051 const &, ret const & >::value), 1);
TT_TEST_END
