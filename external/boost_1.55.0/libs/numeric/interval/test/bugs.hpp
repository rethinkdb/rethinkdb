/* Boost test/bugs.hpp
 * Handles namespace resolution quirks in older compilers and braindead
 * warnings [Herve, June 3rd 2003]
 *
 * Copyright 2002-2003 Hervé Brönnimann
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <boost/config.hpp>

// Borland compiler complains about unused variables a bit easily and
// incorrectly

#ifdef __BORLANDC__
namespace detail {

  template <class T> inline void ignore_unused_variable_warning(const T&) { }

  inline void ignore_warnings() {
#   ifdef BOOST_NUMERIC_INTERVAL_CONSTANTS_HPP
    using namespace boost::numeric::interval_lib::constants;
    ignore_unused_variable_warning( pi_f_l );
    ignore_unused_variable_warning( pi_f_u );
    ignore_unused_variable_warning( pi_d_l );
    ignore_unused_variable_warning( pi_d_u );
#   endif
  }

}
#endif

// Some compilers are broken with respect to name resolution

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP) || defined( __BORLANDC__)

using namespace boost;
using namespace numeric;
using namespace interval_lib;

#endif
