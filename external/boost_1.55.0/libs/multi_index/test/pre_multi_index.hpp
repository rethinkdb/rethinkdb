/* Used in Boost.MultiIndex tests.
 *
 * Copyright 2003-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_TEST_PRE_MULTI_INDEX_HPP
#define BOOST_MULTI_INDEX_TEST_PRE_MULTI_INDEX_HPP

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/workaround.hpp>
#include <boost/multi_index/safe_mode_errors.hpp>

#if defined(__GNUC__)&&defined(__APPLE__)&&\
    (__GNUC__==4)&&(__GNUC_MINOR__==0)&&(__APPLE_CC__<=4061)
  /* Darwin 8.1 includes a prerelease version of GCC 4.0 that, alas,
   * has a regression as described in
   *   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=17435
   * This compiler bug (fixed in the official 4.0 release) ruins the
   * mechanisms upon which invariant-checking scope guards are built,
   * so we can't set the invariant checking mode.
   */
#else
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#endif

#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE

#if BOOST_WORKAROUND(__IBMCPP__,<=600)
#pragma info(nolan) /* suppress warnings about offsetof with non-POD types */
#endif

struct safe_mode_exception
{
  safe_mode_exception(boost::multi_index::safe_mode::error_code error_code_):
    error_code(error_code_)
  {}

  boost::multi_index::safe_mode::error_code error_code;
};

#define BOOST_MULTI_INDEX_SAFE_MODE_ASSERT(expr,error_code) \
if(!(expr)){throw safe_mode_exception(error_code);}

#endif
