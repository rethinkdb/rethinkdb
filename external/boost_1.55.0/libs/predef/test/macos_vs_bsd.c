/*
Copyright Redshift Software Inc. 2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

/*
 * OSX can masquerade as BSD when sys/param.h is previously included.
 * So we test that we only detect OSX in this combination.
 */
#if defined(__APPLE__)
#   include <sys/param.h>
#   include <boost/predef/os/bsd.h>
#   include <boost/predef/os/macos.h>
#   if !BOOST_OS_MACOS || BOOST_OS_BSD
#       error "BOOST_OS_MACOS not detected and/or BOOST_OS_BSD mis-detected."
#   endif
#endif
