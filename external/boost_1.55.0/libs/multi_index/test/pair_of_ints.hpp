/* Used in Boost.MultiIndex tests.
 *
 * Copyright 2003-2010 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#ifndef BOOST_MULTI_INDEX_TEST_PAIR_OF_INTS_HPP
#define BOOST_MULTI_INDEX_TEST_PAIR_OF_INTS_HPP

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/serialization/nvp.hpp>

struct pair_of_ints
{
  pair_of_ints(int first_=0,int second_=0):first(first_),second(second_){}

  bool operator==(const pair_of_ints& x)const
  {
    return first==x.first&&second==x.second;
  }

  bool operator!=(const pair_of_ints& x)const{return !(*this==x);}

  int first,second;
};

inline void increment_first(pair_of_ints& p)
{
  ++p.first;
}

inline void increment_second(pair_of_ints& p)
{
  ++p.second;
}

inline void increment_int(int& x)
{
  ++x;
}

inline int decrement_first(pair_of_ints& p)
{
  return --p.first;
}

inline int decrement_second(pair_of_ints& p)
{
  return --p.second;
}

inline int decrement_int(int& x)
{
  return --x;
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost{
namespace serialization{
#endif

template<class Archive>
void serialize(Archive& ar,pair_of_ints& p,const unsigned int)
{
  ar&boost::serialization::make_nvp("first",p.first);
  ar&boost::serialization::make_nvp("second",p.second);
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace serialization */
} /* namespace boost*/
#endif

#endif
