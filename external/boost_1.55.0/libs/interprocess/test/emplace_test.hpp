//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_INTERPROCESS_TEST_EMPLACE_TEST_HPP
#define BOOST_INTERPROCESS_TEST_EMPLACE_TEST_HPP

#include <iostream>
#include <typeinfo>
#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/interprocess/detail/move.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/aligned_storage.hpp>

namespace boost{
namespace interprocess{
namespace test{

class EmplaceInt
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(EmplaceInt)

   public:
   EmplaceInt()
      : a_(0), b_(0), c_(0), d_(0), e_(0)
   {}
   EmplaceInt(int a, int b = 0, int c = 0, int d = 0, int e = 0)
      : a_(a), b_(b), c_(c), d_(d), e_(e)
   {}

   EmplaceInt(BOOST_RV_REF(EmplaceInt) o)
      : a_(o.a_), b_(o.b_), c_(o.c_), d_(o.d_), e_(o.e_)
   {}

   EmplaceInt& operator=(BOOST_RV_REF(EmplaceInt) o)
   {
      this->a_ = o.a_;
      this->b_ = o.b_;
      this->c_ = o.c_;
      this->d_ = o.d_;
      this->e_ = o.e_;
      return *this;
   }

   friend bool operator==(const EmplaceInt &l, const EmplaceInt &r)
   {
      return l.a_ == r.a_ &&
             l.b_ == r.b_ &&
             l.c_ == r.c_ &&
             l.d_ == r.d_ &&
             l.e_ == r.e_;
   }

   friend bool operator<(const EmplaceInt &l, const EmplaceInt &r)
   {  return l.sum() < r.sum(); }

   friend bool operator>(const EmplaceInt &l, const EmplaceInt &r)
   {  return l.sum() > r.sum(); }

   friend bool operator!=(const EmplaceInt &l, const EmplaceInt &r)
   {  return !(l == r); }

   friend std::ostream &operator <<(std::ostream &os, const EmplaceInt &v)
   {
      os << "EmplaceInt: " << v.a_ << ' ' << v.b_ << ' ' << v.c_ << ' ' << v.d_ << ' ' << v.e_;
      return os;
   }

   //private:
   int sum() const
   {  return this->a_ + this->b_ + this->c_ + this->d_ + this->e_; }

   int a_, b_, c_, d_, e_;
   int padding[6];
};


}  //namespace test {

namespace test {

enum EmplaceOptions{
   EMPLACE_BACK         = 1 << 0,
   EMPLACE_FRONT        = 1 << 1,
   EMPLACE_BEFORE       = 1 << 2,
   EMPLACE_AFTER        = 1 << 3,
   EMPLACE_ASSOC        = 1 << 4,
   EMPLACE_HINT         = 1 << 5,
   EMPLACE_ASSOC_PAIR   = 1 << 6,
   EMPLACE_HINT_PAIR    = 1 << 7
};

template<class Container>
bool test_expected_container(const Container &ec, const EmplaceInt *Expected, unsigned int only_first_n)
{
   typedef typename Container::const_iterator const_iterator;
   const_iterator itb(ec.begin()), ite(ec.end());
   unsigned int cur = 0;
   if(only_first_n > ec.size()){
      return false;
   }
   for(; itb != ite && only_first_n--; ++itb, ++cur){
      const EmplaceInt & cr = *itb;
      if(cr != Expected[cur]){
         return false;
      }
   }
   return true;
}

template<class Container>
bool test_expected_container(const Container &ec, const std::pair<EmplaceInt, EmplaceInt> *Expected, unsigned int only_first_n)
{
   typedef typename Container::const_iterator const_iterator;
   const_iterator itb(ec.begin()), ite(ec.end());
   unsigned int cur = 0;
   if(only_first_n > ec.size()){
      return false;
   }
   for(; itb != ite && only_first_n--; ++itb, ++cur){
      if(itb->first != Expected[cur].first){
         std::cout << "Error in first: " << itb->first << ' ' << Expected[cur].first << std::endl;
         return false;

      }
      else if(itb->second != Expected[cur].second){
         std::cout << "Error in second: " << itb->second << ' ' << Expected[cur].second << std::endl;
         return false;
      }
   }
   return true;
}

static EmplaceInt expected [10];

typedef std::pair<EmplaceInt, EmplaceInt> EmplaceIntPair;
static boost::aligned_storage<sizeof(EmplaceIntPair)*10>::type pair_storage;

static EmplaceIntPair* initialize_emplace_int_pair()
{
   EmplaceIntPair* ret = reinterpret_cast<EmplaceIntPair*>(&pair_storage);
   for(unsigned int i = 0; i != 10; ++i){
      new(&ret->first)EmplaceInt();
      new(&ret->second)EmplaceInt();
   }
   return ret;
}

static EmplaceIntPair * expected_pair = initialize_emplace_int_pair();


template<class Container>
bool test_emplace_back(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_back." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;

   {
   new(&expected [0]) EmplaceInt();
   new(&expected [1]) EmplaceInt(1);
   new(&expected [2]) EmplaceInt(1, 2);
   new(&expected [3]) EmplaceInt(1, 2, 3);
   new(&expected [4]) EmplaceInt(1, 2, 3, 4);
   new(&expected [5]) EmplaceInt(1, 2, 3, 4, 5);
   Container c;
   c.emplace_back();
   if(!test_expected_container(c, &expected[0], 1))
      return false;
   c.emplace_back(1);
   if(!test_expected_container(c, &expected[0], 2))
      return false;
   c.emplace_back(1, 2);
   if(!test_expected_container(c, &expected[0], 3))
      return false;
   c.emplace_back(1, 2, 3);
   if(!test_expected_container(c, &expected[0], 4))
      return false;
   c.emplace_back(1, 2, 3, 4);
   if(!test_expected_container(c, &expected[0], 5))
      return false;
   c.emplace_back(1, 2, 3, 4, 5);
   if(!test_expected_container(c, &expected[0], 6))
      return false;
   }

   return true;
}

template<class Container>
bool test_emplace_back(ipcdetail::false_)
{  return true;   }

template<class Container>
bool test_emplace_front(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_front." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;

   {
      new(&expected [0]) EmplaceInt(1, 2, 3, 4, 5);
      new(&expected [1]) EmplaceInt(1, 2, 3, 4);
      new(&expected [2]) EmplaceInt(1, 2, 3);
      new(&expected [3]) EmplaceInt(1, 2);
      new(&expected [4]) EmplaceInt(1);
      new(&expected [5]) EmplaceInt();
      Container c;
      c.emplace_front();
      if(!test_expected_container(c, &expected[0] + 5, 1))
         return false;
      c.emplace_front(1);
      if(!test_expected_container(c, &expected[0] + 4, 2))
         return false;
      c.emplace_front(1, 2);
      if(!test_expected_container(c, &expected[0] + 3, 3))
         return false;
      c.emplace_front(1, 2, 3);
      if(!test_expected_container(c, &expected[0] + 2, 4))
         return false;
      c.emplace_front(1, 2, 3, 4);
      if(!test_expected_container(c, &expected[0] + 1, 5))
         return false;
      c.emplace_front(1, 2, 3, 4, 5);
      if(!test_expected_container(c, &expected[0] + 0, 6))
         return false;
   }
   return true;
}

template<class Container>
bool test_emplace_front(ipcdetail::false_)
{  return true;   }

template<class Container>
bool test_emplace_before(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_before." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;

   {
      new(&expected [0]) EmplaceInt();
      new(&expected [1]) EmplaceInt(1);
      new(&expected [2]) EmplaceInt();
      Container c;
      c.emplace(c.cend(), 1);
      c.emplace(c.cbegin());
      if(!test_expected_container(c, &expected[0], 2))
         return false;
      c.emplace(c.cend());
      if(!test_expected_container(c, &expected[0], 3))
         return false;
   }
   {
      new(&expected [0]) EmplaceInt();
      new(&expected [1]) EmplaceInt(1);
      new(&expected [2]) EmplaceInt(1, 2);
      new(&expected [3]) EmplaceInt(1, 2, 3);
      new(&expected [4]) EmplaceInt(1, 2, 3, 4);
      new(&expected [5]) EmplaceInt(1, 2, 3, 4, 5);
      //emplace_front-like
      Container c;
      c.emplace(c.cbegin(), 1, 2, 3, 4, 5);
      c.emplace(c.cbegin(), 1, 2, 3, 4);
      c.emplace(c.cbegin(), 1, 2, 3);
      c.emplace(c.cbegin(), 1, 2);
      c.emplace(c.cbegin(), 1);
      c.emplace(c.cbegin());
      if(!test_expected_container(c, &expected[0], 6))
         return false;
      c.clear();
      //emplace_back-like
      typename Container::const_iterator i = c.emplace(c.cend());
      if(!test_expected_container(c, &expected[0], 1))
         return false;
      i = c.emplace(++i, 1);
      if(!test_expected_container(c, &expected[0], 2))
         return false;
      i = c.emplace(++i, 1, 2);
      if(!test_expected_container(c, &expected[0], 3))
         return false;
      i = c.emplace(++i, 1, 2, 3);
      if(!test_expected_container(c, &expected[0], 4))
         return false;
      i = c.emplace(++i, 1, 2, 3, 4);
      if(!test_expected_container(c, &expected[0], 5))
         return false;
      i = c.emplace(++i, 1, 2, 3, 4, 5);
      if(!test_expected_container(c, &expected[0], 6))
         return false;
      c.clear();
      //emplace in the middle
      c.emplace(c.cbegin());
      i = c.emplace(c.cend(), 1, 2, 3, 4, 5);
      i = c.emplace(i, 1, 2, 3, 4);
      i = c.emplace(i, 1, 2, 3);
      i = c.emplace(i, 1, 2);
      i = c.emplace(i, 1);

      if(!test_expected_container(c, &expected[0], 6))
         return false;
   }
   return true;
}

template<class Container>
bool test_emplace_before(ipcdetail::false_)
{  return true;   }

template<class Container>
bool test_emplace_after(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_after." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;
   {
      new(&expected [0]) EmplaceInt();
      new(&expected [1]) EmplaceInt(1);
      new(&expected [2]) EmplaceInt();
      Container c;
      typename Container::const_iterator i = c.emplace_after(c.cbefore_begin(), 1);
      c.emplace_after(c.cbefore_begin());
      if(!test_expected_container(c, &expected[0], 2))
         return false;
      c.emplace_after(i);
      if(!test_expected_container(c, &expected[0], 3))
         return false;
   }
   {
      new(&expected [0]) EmplaceInt();
      new(&expected [1]) EmplaceInt(1);
      new(&expected [2]) EmplaceInt(1, 2);
      new(&expected [3]) EmplaceInt(1, 2, 3);
      new(&expected [4]) EmplaceInt(1, 2, 3, 4);
      new(&expected [5]) EmplaceInt(1, 2, 3, 4, 5);
      //emplace_front-like
      Container c;
      c.emplace_after(c.cbefore_begin(), 1, 2, 3, 4, 5);
      c.emplace_after(c.cbefore_begin(), 1, 2, 3, 4);
      c.emplace_after(c.cbefore_begin(), 1, 2, 3);
      c.emplace_after(c.cbefore_begin(), 1, 2);
      c.emplace_after(c.cbefore_begin(), 1);
      c.emplace_after(c.cbefore_begin());
      if(!test_expected_container(c, &expected[0], 6))
         return false;
      c.clear();
      //emplace_back-like
      typename Container::const_iterator i = c.emplace_after(c.cbefore_begin());
      if(!test_expected_container(c, &expected[0], 1))
         return false;
      i = c.emplace_after(i, 1);
      if(!test_expected_container(c, &expected[0], 2))
         return false;
      i = c.emplace_after(i, 1, 2);
      if(!test_expected_container(c, &expected[0], 3))
         return false;
      i = c.emplace_after(i, 1, 2, 3);
      if(!test_expected_container(c, &expected[0], 4))
         return false;
      i = c.emplace_after(i, 1, 2, 3, 4);
      if(!test_expected_container(c, &expected[0], 5))
         return false;
      i = c.emplace_after(i, 1, 2, 3, 4, 5);
      if(!test_expected_container(c, &expected[0], 6))
         return false;
      c.clear();
      //emplace_after in the middle
      i = c.emplace_after(c.cbefore_begin());
      c.emplace_after(i, 1, 2, 3, 4, 5);
      c.emplace_after(i, 1, 2, 3, 4);
      c.emplace_after(i, 1, 2, 3);
      c.emplace_after(i, 1, 2);
      c.emplace_after(i, 1);

      if(!test_expected_container(c, &expected[0], 6))
         return false;
   }
   return true;
}

template<class Container>
bool test_emplace_after(ipcdetail::false_)
{  return true;   }

template<class Container>
bool test_emplace_assoc(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_assoc." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;

   new(&expected [0]) EmplaceInt();
   new(&expected [1]) EmplaceInt(1);
   new(&expected [2]) EmplaceInt(1, 2);
   new(&expected [3]) EmplaceInt(1, 2, 3);
   new(&expected [4]) EmplaceInt(1, 2, 3, 4);
   new(&expected [5]) EmplaceInt(1, 2, 3, 4, 5);
   {
      Container c;
      c.emplace();
      if(!test_expected_container(c, &expected[0], 1))
         return false;
      c.emplace(1);
      if(!test_expected_container(c, &expected[0], 2))
         return false;
      c.emplace(1, 2);
      if(!test_expected_container(c, &expected[0], 3))
         return false;
      c.emplace(1, 2, 3);
      if(!test_expected_container(c, &expected[0], 4))
         return false;
      c.emplace(1, 2, 3, 4);
      if(!test_expected_container(c, &expected[0], 5))
         return false;
      c.emplace(1, 2, 3, 4, 5);
      if(!test_expected_container(c, &expected[0], 6))
         return false;
   }
   return true;
}

template<class Container>
bool test_emplace_assoc(ipcdetail::false_)
{  return true;   }

template<class Container>
bool test_emplace_hint(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_hint." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;

   new(&expected [0]) EmplaceInt();
   new(&expected [1]) EmplaceInt(1);
   new(&expected [2]) EmplaceInt(1, 2);
   new(&expected [3]) EmplaceInt(1, 2, 3);
   new(&expected [4]) EmplaceInt(1, 2, 3, 4);
   new(&expected [5]) EmplaceInt(1, 2, 3, 4, 5);

   {
      Container c;
      typename Container::const_iterator it;
      it = c.emplace_hint(c.begin());
      if(!test_expected_container(c, &expected[0], 1))
         return false;
      it = c.emplace_hint(it, 1);
      if(!test_expected_container(c, &expected[0], 2))
         return false;
      it = c.emplace_hint(it, 1, 2);
      if(!test_expected_container(c, &expected[0], 3))
         return false;
      it = c.emplace_hint(it, 1, 2, 3);
      if(!test_expected_container(c, &expected[0], 4))
         return false;
      it = c.emplace_hint(it, 1, 2, 3, 4);
      if(!test_expected_container(c, &expected[0], 5))
         return false;
      it = c.emplace_hint(it, 1, 2, 3, 4, 5);
      if(!test_expected_container(c, &expected[0], 6))
         return false;
   }

   return true;
}

template<class Container>
bool test_emplace_hint(ipcdetail::false_)
{  return true;   }

template<class Container>
bool test_emplace_assoc_pair(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_assoc_pair." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;

   new(&expected_pair[0].first) EmplaceInt();
   new(&expected_pair[0].second) EmplaceInt();
   new(&expected_pair[1].first) EmplaceInt(1);
   new(&expected_pair[1].second) EmplaceInt();
   new(&expected_pair[2].first) EmplaceInt(2);
   new(&expected_pair[2].second) EmplaceInt(2);
//   new(&expected_pair[3].first) EmplaceInt(3);
//   new(&expected_pair[3].second) EmplaceInt(2, 3);
//   new(&expected_pair[4].first) EmplaceInt(4);
//   new(&expected_pair[4].second) EmplaceInt(2, 3, 4);
//   new(&expected_pair[5].first) EmplaceInt(5);
//   new(&expected_pair[5].second) EmplaceInt(2, 3, 4, 5);
   {  //piecewise construct missing
      /*
      Container c;
      c.emplace();
      if(!test_expected_container(c, &expected_pair[0], 1)){
         std::cout << "Error after c.emplace();\n";
         return false;
      }
      c.emplace(1);
      if(!test_expected_container(c, &expected_pair[0], 2)){
         std::cout << "Error after c.emplace(1);\n";
         return false;
      }
      c.emplace(2, 2);
      if(!test_expected_container(c, &expected_pair[0], 3)){
         std::cout << "Error after c.emplace(2, 2);\n";
         return false;
      }
      c.emplace(3, 2, 3);
      if(!test_expected_container(c, &expected_pair[0], 4)){
         std::cout << "Error after c.emplace(3, 2, 3);\n";
         return false;
      }
      c.emplace(4, 2, 3, 4);
      if(!test_expected_container(c, &expected_pair[0], 5)){
         std::cout << "Error after c.emplace(4, 2, 3, 4);\n";
         return false;
      }
      c.emplace(5, 2, 3, 4, 5);
      if(!test_expected_container(c, &expected_pair[0], 6)){
         std::cout << "Error after c.emplace(5, 2, 3, 4, 5);\n";
         return false;
      }*/
   }
   return true;
}

template<class Container>
bool test_emplace_assoc_pair(ipcdetail::false_)
{  return true;   }

template<class Container>
bool test_emplace_hint_pair(ipcdetail::true_)
{
   std::cout << "Starting test_emplace_hint_pair." << std::endl << "  Class: "
      << typeid(Container).name() << std::endl;

   new(&expected_pair[0].first) EmplaceInt();
   new(&expected_pair[0].second) EmplaceInt();
   new(&expected_pair[1].first) EmplaceInt(1);
   new(&expected_pair[1].second) EmplaceInt();
   new(&expected_pair[2].first) EmplaceInt(2);
   new(&expected_pair[2].second) EmplaceInt(2);/*
   new(&expected_pair[3].first) EmplaceInt(3);
   new(&expected_pair[3].second) EmplaceInt(2, 3);
   new(&expected_pair[4].first) EmplaceInt(4);
   new(&expected_pair[4].second) EmplaceInt(2, 3, 4);
   new(&expected_pair[5].first) EmplaceInt(5);
   new(&expected_pair[5].second) EmplaceInt(2, 3, 4, 5);*/
   {/*
      Container c;
      typename Container::const_iterator it;
      it = c.emplace_hint(c.begin());
      if(!test_expected_container(c, &expected_pair[0], 1)){
         std::cout << "Error after c.emplace(1);\n";
         return false;
      }
      it = c.emplace_hint(it, 1);
      if(!test_expected_container(c, &expected_pair[0], 2)){
         std::cout << "Error after c.emplace(it, 1);\n";
         return false;
      }
      it = c.emplace_hint(it, 2, 2);
      if(!test_expected_container(c, &expected_pair[0], 3)){
         std::cout << "Error after c.emplace(it, 2, 2);\n";
         return false;
      }
      it = c.emplace_hint(it, 3, 2, 3);
      if(!test_expected_container(c, &expected_pair[0], 4)){
         std::cout << "Error after c.emplace(it, 3, 2, 3);\n";
         return false;
      }
      it = c.emplace_hint(it, 4, 2, 3, 4);
      if(!test_expected_container(c, &expected_pair[0], 5)){
         std::cout << "Error after c.emplace(it, 4, 2, 3, 4);\n";
         return false;
      }
      it = c.emplace_hint(it, 5, 2, 3, 4, 5);
      if(!test_expected_container(c, &expected_pair[0], 6)){
         std::cout << "Error after c.emplace(it, 5, 2, 3, 4, 5);\n";
         return false;
      }*/
   }
   return true;
}

template<class Container>
bool test_emplace_hint_pair(ipcdetail::false_)
{  return true;   }

template <EmplaceOptions O, EmplaceOptions Mask>
struct emplace_active
{
   static const bool value = (0 != (O & Mask));
   typedef ipcdetail::bool_<value> type;
   operator type() const{  return type(); }
};

template<class Container, EmplaceOptions O>
bool test_emplace()
{
   if(!test_emplace_back<Container>(emplace_active<O, EMPLACE_BACK>()))
      return false;
   if(!test_emplace_front<Container>(emplace_active<O, EMPLACE_FRONT>()))
      return false;
   if(!test_emplace_before<Container>(emplace_active<O, EMPLACE_BEFORE>()))
      return false;
   if(!test_emplace_after<Container>(emplace_active<O, EMPLACE_AFTER>()))
      return false;
   if(!test_emplace_assoc<Container>(emplace_active<O, EMPLACE_ASSOC>()))
      return false;
   if(!test_emplace_hint<Container>(emplace_active<O, EMPLACE_HINT>()))
      return false;
   if(!test_emplace_assoc_pair<Container>(emplace_active<O, EMPLACE_ASSOC_PAIR>()))
      return false;
   if(!test_emplace_hint_pair<Container>(emplace_active<O, EMPLACE_HINT_PAIR>()))
      return false;
   return true;
}

}  //namespace test{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif   //#ifndef BOOST_INTERPROCESS_TEST_EMPLACE_TEST_HPP
