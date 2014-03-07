/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Olaf Krzikalla 2004-2006.
// (C) Copyright Ion Gaztanaga  2006-2013.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTRUSIVE_DETAIL_ITESTVALUE_HPP
#define BOOST_INTRUSIVE_DETAIL_ITESTVALUE_HPP

#include <iostream>
#include <boost/intrusive/options.hpp>
#include <boost/functional/hash.hpp>

namespace boost{
namespace intrusive{

struct testvalue_filler
{
   void *dummy_[3];
};

template<class Hooks, bool ConstantTimeSize>
struct testvalue
   :  testvalue_filler
   ,  Hooks::base_hook_type
   ,  Hooks::auto_base_hook_type
{
   typename Hooks::member_hook_type        node_;
   typename Hooks::auto_member_hook_type   auto_node_;
   int value_;

   static const bool constant_time_size = ConstantTimeSize;

   testvalue()
   {}

   testvalue(int i)
      :  value_(i)
   {}

   testvalue (const testvalue& src)
      :  value_ (src.value_)
   {}

   // testvalue is used in std::vector and thus prev and next
   // have to be handled appropriately when copied:
   testvalue & operator= (const testvalue& src)
   {
      Hooks::base_hook_type::operator=(src);
      Hooks::auto_base_hook_type::operator=(src);
      this->node_       = src.node_;
      this->auto_node_  = src.auto_node_;
      value_ = src.value_;
      return *this;
   }

   void swap_nodes(testvalue &other)
   {
      Hooks::base_hook_type::swap_nodes(other);
      Hooks::auto_base_hook_type::swap_nodes(other);
      node_.swap_nodes(other.node_);
      auto_node_.swap_nodes(other.auto_node_);
   }

   bool is_linked() const
   {
      return Hooks::base_hook_type::is_linked() ||
      Hooks::auto_base_hook_type::is_linked() ||
      node_.is_linked() ||
      auto_node_.is_linked();
   }

   ~testvalue()
   {}

   bool operator< (const testvalue &other) const
   {  return value_ < other.value_;  }

   bool operator==(const testvalue &other) const
   {  return value_ == other.value_;  }

   bool operator!=(const testvalue &other) const
   {  return value_ != other.value_;  }

   friend bool operator< (int other1, const testvalue &other2)
   {  return other1 < other2.value_;  }

   friend bool operator< (const testvalue &other1, int other2)
   {  return other1.value_ < other2;  }

   friend bool operator== (int other1, const testvalue &other2)
   {  return other1 == other2.value_;  }

   friend bool operator== (const testvalue &other1, int other2)
   {  return other1.value_ == other2;  }

   friend bool operator!= (int other1, const testvalue &other2)
   {  return other1 != other2.value_;  }

   friend bool operator!= (const testvalue &other1, int other2)
   {  return other1.value_ != other2;  }
};

template<class Hooks, bool ConstantTimeSize>
std::size_t hash_value(const testvalue<Hooks, ConstantTimeSize> &t)
{
   boost::hash<int> hasher;
   return hasher(t.value_);
}

template<class Hooks, bool ConstantTimeSize>
bool priority_order( const testvalue<Hooks, ConstantTimeSize> &t1
                   , const testvalue<Hooks, ConstantTimeSize> &t2)
{
   std::size_t hash1 = hash_value(t1);
   boost::hash_combine(hash1, &t1);
   std::size_t hash2 = hash_value(t2);
   boost::hash_combine(hash2, &t2);
   return hash1 < hash2;
}

template<class Hooks, bool constant_time_size>
std::ostream& operator<<
   (std::ostream& s, const testvalue<Hooks, constant_time_size>& t)
{  return s << t.value_;   }

struct even_odd
{
   template<class Hooks, bool constant_time_size>
   bool operator()
      (const testvalue<Hooks, constant_time_size>& v1
      ,const testvalue<Hooks, constant_time_size>& v2) const
   {
      if ((v1.value_ & 1) == (v2.value_ & 1))
         return v1.value_ < v2.value_;
      else
         return v2.value_ & 1;
   }
};

struct is_even
{
   template<class Hooks, bool constant_time_size>
   bool operator()
      (const testvalue<Hooks, constant_time_size>& v1) const
   {  return (v1.value_ & 1) == 0;  }
};
/*
struct int_testvalue_comp
{
   template<class Hooks, bool constant_time_size>
   bool operator()
      (const testvalue<Hooks, constant_time_size>& v1, const int &i) const
   {  return v1.value_ < i; }
   template<class Hooks, bool constant_time_size>
   bool operator()
      (const int &i, const testvalue<Hooks, constant_time_size>& v1) const
   {  return i < v1.value_; }
};

struct int_testvalue_pcomp
{
   template<class Hooks, bool constant_time_size>
   bool operator()
      (const testvalue<Hooks, constant_time_size>& v1, const int &i) const
   {  return v1.value_ < i; }
   template<class Hooks, bool constant_time_size>
   bool operator()
      (const int &i, const testvalue<Hooks, constant_time_size>& v1) const
   {  return i < v1.value_; }
};
*/

}  //namespace boost{
}  //namespace intrusive{

#endif
