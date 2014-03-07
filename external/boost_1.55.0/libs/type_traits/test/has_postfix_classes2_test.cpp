//  (C) Copyright 2009-2011 Frederic Bron.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test.hpp"
#include "check_integral_constant.hpp"

#include <boost/type_traits/has_operator.hpp>
#include "has_postfix_classes.hpp"

TT_TEST_BEGIN(BOOST_TT_TRAIT_NAME)
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 &, void >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C036 const &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C037 const &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C038 const &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 &, ret & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C041 const &, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const, ret const & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 &, ret >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 &, ret const >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 &, ret const & >::value), 1);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const &, void >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const &, ret >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const &, ret const >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const &, ret & >::value), 0);
   BOOST_CHECK_INTEGRAL_CONSTANT((::boost::has_post_increment< C042 const &, ret const & >::value), 0);
TT_TEST_END
