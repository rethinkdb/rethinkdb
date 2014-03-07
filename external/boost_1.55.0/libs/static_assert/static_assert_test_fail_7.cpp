//  (C) Copyright John Maddock 2000.
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version including documentation.

#include <climits>
#include <boost/limits.hpp>
#include <boost/static_assert.hpp>

template <class UnsignedInt>
class myclass
{
private:
   BOOST_STATIC_ASSERT(sizeof(UnsignedInt) * CHAR_BIT >= 16);
   BOOST_STATIC_ASSERT(std::numeric_limits<UnsignedInt>::is_specialized
                        && std::numeric_limits<UnsignedInt>::is_integer
                        && !std::numeric_limits<UnsignedInt>::is_signed);
public:
   /* details here */
};

myclass<int>           m2; // this should fail
myclass<unsigned char> m3; // and so should this

int main()
{
   return 0;
}

