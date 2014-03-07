//  (C) Copyright Steve Cleary & John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <climits>
#include <cwchar>
#include <limits>
#include <boost/static_assert.hpp>

#if !defined(WCHAR_MIN)
#define WCHAR_MIN 0
#endif

namespace boost{

namespace my_conditions {
BOOST_STATIC_ASSERT(std::numeric_limits<int>::digits >= 32);
BOOST_STATIC_ASSERT(WCHAR_MIN >= 0);

} // namespace my_conditions

} // namespace boost

int main()
{
   return 0;
}




