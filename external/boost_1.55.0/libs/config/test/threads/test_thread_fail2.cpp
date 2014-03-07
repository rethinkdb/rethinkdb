//  (C) Copyright John Maddock 2003. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config.hpp>
// disable thread support:
#ifdef BOOST_HAS_THREADS
#  undef BOOST_HAS_THREADS
#endif
// this should now be a compiler error:
#include <boost/config/requires_threads.hpp>
// we should never get here...
