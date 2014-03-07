/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

/* no include guards */

#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/for_each_product.hpp> 
#include <boost/preprocessor/seq/size.hpp>

#define BOOST_FLYWEIGHT_CONST(b) BOOST_PP_CAT(BOOST_FLYWEIGHT_CONST,b)
#define BOOST_FLYWEIGHT_CONST0
#define BOOST_FLYWEIGHT_CONST1 const

/* if mask[n]==0 --> Tn& tn
 * if mask[n]==1 --> const Tn& tn
 */

#define BOOST_FLYWEIGHT_PERFECT_FWD_ARG(z,n,mask)                  \
BOOST_FLYWEIGHT_CONST(BOOST_PP_SEQ_ELEM(n,mask))                   \
BOOST_PP_CAT(T,n)& BOOST_PP_CAT(t,n)

/* overload accepting size(mask) args, where the template args are
 * marked const or not according to the given mask (a seq of 0 or 1)
 */

#define BOOST_FLYWEIGHT_PERFECT_FWD(r,mask)                        \
template<BOOST_PP_ENUM_PARAMS(BOOST_PP_SEQ_SIZE(mask),typename T)> \
BOOST_FLYWEIGHT_PERFECT_FWD_NAME(                                  \
  BOOST_PP_ENUM(                                                   \
    BOOST_PP_SEQ_SIZE(mask),BOOST_FLYWEIGHT_PERFECT_FWD_ARG,mask)) \
BOOST_FLYWEIGHT_PERFECT_FWD_BODY(BOOST_PP_SEQ_SIZE(mask))

#define BOOST_FLYWEIGHT_01(z,n,_) ((0)(1))

/* Perfect forwarding overloads accepting 1 to n args */
 
#define BOOST_FLYWEIGHT_PERFECT_FWDS_N(z,n,_)                      \
BOOST_PP_SEQ_FOR_EACH_PRODUCT(                                     \
  BOOST_FLYWEIGHT_PERFECT_FWD,                                     \
  BOOST_PP_REPEAT(n,BOOST_FLYWEIGHT_01,~))

#define BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS                      \
BOOST_PP_REPEAT_FROM_TO(                                           \
  1,BOOST_PP_ADD(BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS,1),        \
  BOOST_FLYWEIGHT_PERFECT_FWDS_N,~)

/* generate the overloads */

BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS

/* clean up */

#undef BOOST_FLYWEIGHT_PERFECT_FWD_OVERLOADS
#undef BOOST_FLYWEIGHT_01
#undef BOOST_FLYWEIGHT_PERFECT_FWD
#undef BOOST_FLYWEIGHT_PERFECT_FWD_ARG
#undef BOOST_FLYWEIGHT_CONST1 
#undef BOOST_FLYWEIGHT_CONST0
#undef BOOST_FLYWEIGHT_CONST

/* user supplied argument macros */

#undef BOOST_FLYWEIGHT_PERFECT_FWD_BODY
#undef BOOST_FLYWEIGHT_PERFECT_FWD_NAME
