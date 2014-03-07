//  (C) Copyright John Maddock 2003. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for the most recent version.


#ifndef BOOST_ABI_TEST_HPP
#define BOOST_ABI_TEST_HPP

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_PREFIX
#endif

//
// the following class is designed to break if the ABI
// it's compiled with does not match that of the client
// calling it....
//

struct empty{};

class abi_test : protected empty
{
private:
   empty e;
   char c;
   boost::int32_t i;
public:
   inline char inline_one()const
   { return c; }
   inline boost::int32_t inline_two()const
   { return i; }

   virtual char virtual_one()const;
   virtual boost::int32_t virtual_two()const;

   abi_test();
};



#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_ABI_TEST_HPP



