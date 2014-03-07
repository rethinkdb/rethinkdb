//  (C) Copyright John Maddock 2005.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifdef TEST_STD_HEADERS
#include <functional>
#else
#include <boost/tr1/functional.hpp>
#endif

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>

int main()
{
   typedef std::tr1::reference_wrapper<int> rr_type;
   typedef std::tr1::reference_wrapper<const int> crr_type;

   int i, j;
   rr_type r1 = std::tr1::ref(i);
   crr_type r2 = std::tr1::cref(i);
   r1 = std::tr1::ref(j);

   int& ref = r1.get();

   BOOST_STATIC_ASSERT((::boost::is_convertible<rr_type, int&>::value));
   BOOST_STATIC_ASSERT((::boost::is_convertible<crr_type, const int&>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<rr_type::type, int>::value));
   BOOST_STATIC_ASSERT((::boost::is_same<crr_type::type, const int>::value));
}
