///////////////////////////////////////////////////////////////
//  Copyright 2012 John Maddock. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_

#ifndef BOOST_MP_DIGITS_HPP
#define BOOST_MP_DIGITS_HPP

namespace boost{ namespace multiprecision{ namespace detail{

inline unsigned long digits10_2_2(unsigned long d10)
{
   return (d10 * 1000uL) / 301uL + ((d10 * 1000uL) % 301 ? 2u : 1u);
}

inline unsigned long digits2_2_10(unsigned long d2)
{
   return (d2 * 301uL) / 1000uL;
}

}}} // namespaces

#endif
