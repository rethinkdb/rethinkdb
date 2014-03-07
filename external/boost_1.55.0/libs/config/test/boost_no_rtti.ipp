//  (C) Copyright John Maddock 2008.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/config for most recent version.

//  MACRO:         BOOST_NO_RTTI
//  TITLE:         RTTI unavailable
//  DESCRIPTION:   The compiler does not support RTTI in this mode

#include <typeinfo>

class A
{
public:
   A(){}
   virtual void t();
};

void A::t()
{
}

class B : public A
{
public:
   B(){}
   virtual void t();
};

void B::t()
{
}

namespace boost_no_rtti
{

int check_f(const A& a)
{
   return typeid(a) == typeid(B) ? 0 : 1;
}

int test()
{
   try{
      B b;
      return check_f(b);
   }
   catch(...)
   {
      return 1;
   }
}

}

