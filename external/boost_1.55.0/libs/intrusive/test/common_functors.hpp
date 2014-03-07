/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2006-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTRUSIVE_TEST_COMMON_FUNCTORS_HPP
#define BOOST_INTRUSIVE_TEST_COMMON_FUNCTORS_HPP

#include<boost/intrusive/detail/utilities.hpp>
#include<boost/intrusive/detail/mpl.hpp>

namespace boost      {
namespace intrusive  {
namespace test       {

template<class T>
class delete_disposer
{
   public:
   template <class Pointer>
      void operator()(Pointer p)
   {
      typedef typename std::iterator_traits<Pointer>::value_type value_type;
      BOOST_INTRUSIVE_INVARIANT_ASSERT(( detail::is_same<T, value_type>::value ));
      delete boost::intrusive::detail::to_raw_pointer(p);
   }
};

template<class T>
class new_cloner
{
   public:
      T *operator()(const T &t)
   {  return new T(t);  }
};

template<class T>
class new_default_factory
{
   public:
      T *operator()()
   {  return new T();  }
};

}  //namespace test       {
}  //namespace intrusive  {
}  //namespace boost      {

#endif
