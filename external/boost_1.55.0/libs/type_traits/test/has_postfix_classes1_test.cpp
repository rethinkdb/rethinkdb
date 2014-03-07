//  (C) Copyright 2009-2011 Frederic Bron.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"

#include <boost/type_traits/has_operator.hpp>
#include "has_postfix_classes.hpp"

TT_TEST_BEGIN(BOOST_TT_TRAIT_NAME)
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 &, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const &, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C009 const &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C010 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C011 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 &, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const &, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C014 const &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C015 const &, ret const & >::value), 1);
TT_TEST_END
