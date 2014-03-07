/*
Copyright Redshift Software Inc. 2013
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
*/

/*
 * OSX can define the BSD symbols if sys/param.h is included
 * before detection. This causes the endian detection to misfire
 * as both MACOS and BSD are "detected" (currently). This just
 * tests that the sys/param.h include can be included before
 * endian detection and still have it work correctly.
 */

#if defined(__APPLE__)
#   include <sys/param.h>
#   include <boost/predef/os/bsd.h>
#   include <boost/predef/os/macos.h>
#   include <boost/predef/other/endian.h>
#endif
