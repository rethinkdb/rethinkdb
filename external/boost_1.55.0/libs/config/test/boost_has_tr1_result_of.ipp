//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_HAS_TR1_RESULT_OF
//  TITLE:         std::tr1::result_of
//  DESCRIPTION:   The std lib has a tr1-conforming result_of template.

#include <functional>

namespace boost_has_tr1_result_of{

typedef std::tr1::result_of<int*(int)> r;
typedef r::type rr;

int test()
{
   return 0;
}

}
