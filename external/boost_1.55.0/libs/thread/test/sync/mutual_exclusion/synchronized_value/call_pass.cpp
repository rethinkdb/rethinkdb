// Copyright (C) 2013 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/synchronized_value.hpp>

// class synchronized_value<T,M>

//    template <typename F>
//    inline typename boost::result_of<F(value_type&)>::type
//    operator()(BOOST_THREAD_RV_REF(F) fct);
//    template <typename F>
//    inline typename boost::result_of<F(value_type const&)>::type
//    operator()(BOOST_THREAD_RV_REF(F) fct) const;

#include <boost/config.hpp>
#if ! defined  BOOST_NO_CXX11_DECLTYPE
#define BOOST_RESULT_OF_USE_DECLTYPE
#endif

#define BOOST_THREAD_VERSION 4

#include <boost/thread/synchronized_value.hpp>

#include <boost/detail/lightweight_test.hpp>

struct S {
  int f() const {return 1;}
  int g() {return 1;}
};

void c(S const& s)
{
  BOOST_TEST(s.f()==1);
}

void nc(S & s)
{
  BOOST_TEST(s.f()==1);
  BOOST_TEST(s.g()==1);
}

struct cfctr {
  typedef void result_type;
  void operator()(S const& s) const {
    BOOST_TEST(s.f()==1);
  }
};
struct ncfctr {
  typedef void result_type;
  void operator()(S& s) const {
    BOOST_TEST(s.f()==1);
    BOOST_TEST(s.g()==1);
  }
};

struct cfctr3 {
  typedef void result_type;
  BOOST_THREAD_MOVABLE_ONLY(cfctr3)
  cfctr3()
  {}
  cfctr3(BOOST_THREAD_RV_REF(cfctr3))
  {}
  void operator()(S const& s) const {
    BOOST_TEST(s.f()==1);
  }
};
struct ncfctr3 {
  typedef void result_type;
  BOOST_THREAD_MOVABLE_ONLY(ncfctr3)
  ncfctr3()
  {}
  ncfctr3(BOOST_THREAD_RV_REF(ncfctr3))
  {}
  void operator()(S& s) const {
    BOOST_TEST(s.f()==1);
    BOOST_TEST(s.g()==1);
  }
};

cfctr3 make_cfctr3() {
  return BOOST_THREAD_MAKE_RV_REF(cfctr3());
}

ncfctr3 make_ncfctr3() {
  return BOOST_THREAD_MAKE_RV_REF(ncfctr3());
}

int main()
{
  {
      boost::synchronized_value<S> v;
      v(&nc);
      //v(&c);
  }
  {
      const boost::synchronized_value<S> v;
      v(&c);
  }
  {
      boost::synchronized_value<S> v;
      v(ncfctr());
  }
  {
      const boost::synchronized_value<S> v;
      v(cfctr());
  }
  {
      boost::synchronized_value<S> v;
      ncfctr fct;
      v(fct);
  }
  {
      const boost::synchronized_value<S> v;
      cfctr fct;
      v(fct);
  }
  {
      boost::synchronized_value<S> v;
      v(make_ncfctr3());
  }
  {
      const boost::synchronized_value<S> v;
      v(make_cfctr3());
  }
#if ! defined BOOST_NO_CXX11_LAMBDAS
  {
      boost::synchronized_value<S> v;
      v([](S& s) {
        BOOST_TEST(s.f()==1);
        BOOST_TEST(s.g()==1);
      });
  }
  {
      const boost::synchronized_value<S> v;
      v([](S const& s) {
        BOOST_TEST(s.f()==1);
      });
  }
#endif
  return boost::report_errors();
}

