//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2012-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_TEST_FORWARD_TO_INPUT_ITERATOR_HPP
#define BOOST_CONTAINER_TEST_FORWARD_TO_INPUT_ITERATOR_HPP

#include <iterator>

namespace boost{
namespace container {
namespace test{

template<class FwdIterator>
class input_iterator_wrapper
   : public std::iterator< std::input_iterator_tag
                         , typename std::iterator_traits<FwdIterator>::value_type
                         , typename std::iterator_traits<FwdIterator>::difference_type
                         , typename std::iterator_traits<FwdIterator>::pointer
                         , typename std::iterator_traits<FwdIterator>::reference
                         >
{
   FwdIterator m_it;

   public:
   input_iterator_wrapper()
      : m_it(0)
   {}

   explicit input_iterator_wrapper(FwdIterator it)
      : m_it(it)
   {}

   //Default copy constructor...
   //input_iterator_wrapper(const input_iterator_wrapper&);

   //Default assignment...
   //input_iterator_wrapper &operator=(const input_iterator_wrapper&);

   //Default destructor...
   //~input_iterator_wrapper();

   typename std::iterator_traits<FwdIterator>::reference operator*() const
   { return *m_it; }

   typename std::iterator_traits<FwdIterator>::pointer operator->() const
   { return m_it.operator->(); }

   input_iterator_wrapper& operator++()
   {  ++m_it;  return *this;  }

   input_iterator_wrapper operator++(int )
   {
      input_iterator_wrapper tmp(m_it);
      ++m_it;
      return tmp;
   }

   friend bool operator==(const input_iterator_wrapper &left, const input_iterator_wrapper &right)
   {  return left.m_it == right.m_it;  }

   friend bool operator!=(const input_iterator_wrapper &left, const input_iterator_wrapper &right)
   {  return left.m_it != right.m_it;  }
};

template<class FwdIterator>
input_iterator_wrapper<FwdIterator> make_input_from_forward_iterator(const FwdIterator &it)
{  return input_iterator_wrapper<FwdIterator>(it); }

}  //namespace test{
}  //namespace container {
}  //namespace boost{

#endif //BOOST_CONTAINER_TEST_FORWARD_TO_INPUT_ITERATOR_HPP
