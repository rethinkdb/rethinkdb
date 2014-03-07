//  (C) Copyright 2009-2011 Frederic Bron.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"

#include <boost/type_traits/has_operator.hpp>
#include "has_prefix_classes.hpp"

TT_TEST_BEGIN(BOOST_TT_TRAIT_NAME)
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 &, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const &, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C000 const &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C001 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C002 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 &, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const &, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C005 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_pre_increment< C006 const &, ret const & >::value), 1);
TT_TEST_END
