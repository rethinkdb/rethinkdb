/*=============================================================================
    Copyright (c) 2001-2007 Joel de Guzman

    Distributed under the Boost Software License, Version 1.0. (See accompanying 
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/
#ifndef PHOENIX_CORE_LIMITS_HPP
#define PHOENIX_CORE_LIMITS_HPP

#include <boost/preprocessor/dec.hpp>

#if !defined(PHOENIX_LIMIT)
# define PHOENIX_LIMIT 10
#elif (PHOENIX_LIMIT < 5)
# error "PHOENIX_LIMIT is set too low"
#endif

#if !defined(PHOENIX_ARG_LIMIT)
# define PHOENIX_ARG_LIMIT PHOENIX_LIMIT
#elif (PHOENIX_ARG_LIMIT < 5)
# error "PHOENIX_ARG_LIMIT is set too low"
#endif

#if !defined(PHOENIX_ACTOR_LIMIT)
# define PHOENIX_ACTOR_LIMIT PHOENIX_LIMIT
#elif (PHOENIX_ACTOR_LIMIT > PHOENIX_ARG_LIMIT)
# error "PHOENIX_ACTOR_LIMIT > PHOENIX_ARG_LIMIT"
#elif (PHOENIX_ACTOR_LIMIT < 3)
# error "PHOENIX_ACTOR_LIMIT is set too low"
#endif

#if !defined(PHOENIX_COMPOSITE_LIMIT)
# define PHOENIX_COMPOSITE_LIMIT PHOENIX_LIMIT
#elif (PHOENIX_COMPOSITE_LIMIT < 5)
# error "PHOENIX_COMPOSITE_LIMIT is set too low"
#endif

#if !defined(PHOENIX_MEMBER_LIMIT)
# define PHOENIX_MEMBER_LIMIT BOOST_PP_DEC(BOOST_PP_DEC(PHOENIX_COMPOSITE_LIMIT))
#elif (PHOENIX_MEMBER_LIMIT > PHOENIX_COMPOSITE_LIMIT)
# error "PHOENIX_MEMBER_LIMIT > PHOENIX_COMPOSITE_LIMIT"
#elif (PHOENIX_MEMBER_LIMIT < 3)
# error "PHOENIX_MEMBER_LIMIT is set too low"
#endif

#if !defined(PHOENIX_CATCH_LIMIT)
# define PHOENIX_CATCH_LIMIT BOOST_PP_DEC(PHOENIX_COMPOSITE_LIMIT)
#elif (PHOENIX_CATCH_LIMIT < 1)
# error "PHOENIX_CATCH_LIMIT is set too low"
#endif

#if !defined(PHOENIX_DYNAMIC_LIMIT)
# define PHOENIX_DYNAMIC_LIMIT PHOENIX_LIMIT
#elif (PHOENIX_DYNAMIC_LIMIT < 1)
# error "PHOENIX_DYNAMIC_LIMIT is set too low"
#endif

#if !defined(PHOENIX_LOCAL_LIMIT)
# define PHOENIX_LOCAL_LIMIT PHOENIX_LIMIT
#elif (PHOENIX_LOCAL_LIMIT < 3)
# error "PHOENIX_LOCAL_LIMIT is set too low"
#endif


#if !defined(FUSION_MAX_VECTOR_SIZE)
# define FUSION_MAX_VECTOR_SIZE PHOENIX_LIMIT
#elif (FUSION_MAX_VECTOR_SIZE < PHOENIX_LIMIT)
# error "FUSION_MAX_VECTOR_SIZE < PHOENIX_LIMIT"
#endif

// this include will bring in mpl::vectorN and 
// fusion::vectorN where N is PHOENIX_LIMIT
#include <boost/fusion/include/vector.hpp>

// for some reason, this must be included now to make
// detail/type_deduction.hpp compile. $$$ TODO: Investigate further $$$
#include <boost/mpl/vector/vector20.hpp>

#endif
