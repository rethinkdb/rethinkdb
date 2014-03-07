/* Boost.Flyweight test of flyweight forwarding ctors.
 *
 * Copyright 2006-2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#include "test_multictor.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/workaround.hpp>
#include <boost/flyweight.hpp> 
#include <boost/functional/hash.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include "test_basic_template.hpp"

using boost::flyweight;

#if BOOST_WORKAROUND(BOOST_MSVC,<1300)
#define NONCONST const
#else
#define NONCONST
#endif

struct multictor
{
  typedef multictor type;

  multictor():
    t(0,0,0.0,"",false){}
  multictor(NONCONST int& x0):
    t(x0,0,0.0,"",false){}
  multictor(int x0,NONCONST char& x1):
    t(x0,x1,0.0,"",false){}
  multictor(int x0,char x1,NONCONST double& x2):
    t(x0,x1,x2,"",false){}
  multictor(int x0,char x1,double x2,NONCONST std::string& x3):
    t(x0,x1,x2,x3,false){}
  multictor(int x0,char x1,double x2,const std::string& x3,NONCONST bool& x4):
    t(x0,x1,x2,x3,x4){}

  friend bool operator==(const type& x,const type& y){return x.t==y.t;}
  friend bool operator< (const type& x,const type& y){return x.t< y.t;}
  friend bool operator!=(const type& x,const type& y){return x.t!=y.t;}
  friend bool operator> (const type& x,const type& y){return x.t> y.t;}
  friend bool operator>=(const type& x,const type& y){return x.t>=y.t;}
  friend bool operator<=(const type& x,const type& y){return x.t<=y.t;}

  boost::tuples::tuple<int,char,double,std::string,bool> t;
};

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost{
#endif

inline std::size_t hash_value(const multictor& x)
{
  std::size_t res=0;
  boost::hash_combine(res,boost::tuples::get<0>(x.t));
  boost::hash_combine(res,boost::tuples::get<1>(x.t));
  boost::hash_combine(res,boost::tuples::get<2>(x.t));
  boost::hash_combine(res,boost::tuples::get<3>(x.t));
  boost::hash_combine(res,boost::tuples::get<4>(x.t));
  return res;
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
} /* namespace boost */
#endif

void test_multictor()
{
  flyweight<multictor> f;
  multictor            m;
  BOOST_TEST(f==m);

  int x0=1;
  flyweight<multictor> f0(x0);
  multictor            m0(x0);
  BOOST_TEST(f0==m0);

  char x1='a';
  flyweight<multictor> f1(1,x1);
  multictor            m1(1,x1);
  BOOST_TEST(f1==m1);

  double x2=3.1416;
  flyweight<multictor> f2(1,'a',x2);
  multictor            m2(1,'a',x2);
  BOOST_TEST(f2==m2);

  std::string x3("boost");
  flyweight<multictor> f3(1,'a',3.1416,x3);
  multictor            m3(1,'a',3.1416,x3);
  BOOST_TEST(f3==m3);

  bool x4=true;
  flyweight<multictor> f4(1,'a',3.1416,"boost",x4);
  multictor            m4(1,'a',3.1416,"boost",x4);
  BOOST_TEST(f4==m4);
}
