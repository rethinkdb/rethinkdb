/* Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

/* Brute force implementation of perfect forwarding overloads.
 * Usage: include after having defined the argument macros:
 *   BOOST_FLYWEIGHT_PERFECT_FWD_NAME
 *   BOOST_FLYWEIGHT_PERFECT_FWD_BODY
 */

/* This user_definable macro limits the maximum number of arguments to
 * be perfect forwarded. Beware combinatorial explosion: manual perfect
 * forwarding for n arguments produces 2^n distinct overloads.
 */

#if !defined(BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS)
#define BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS 5
#endif

#if BOOST_FLYWEIGHT_LIMIT_PERFECT_FWD_ARGS<=5
#include <boost/flyweight/detail/pp_perfect_fwd.hpp>
#else
#include <boost/flyweight/detail/dyn_perfect_fwd.hpp>
#endif
