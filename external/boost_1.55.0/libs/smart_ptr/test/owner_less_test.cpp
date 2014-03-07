//
//  owner_less_test.cpp
//
//  A regression test for owner_less
//
//  Copyright (c) 2008 Frank Mori Hess
//
//  Distributed under the Boost Software License, Version 1.0.
//
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/detail/lightweight_test.hpp>
#include <boost/smart_ptr/owner_less.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

int main()
{
  boost::owner_less<boost::shared_ptr<int> > comp;
  {
    boost::shared_ptr<int> x;
    boost::shared_ptr<int> y;
    boost::weak_ptr<int> w;
    BOOST_TEST(!(comp(x, w) || comp(w, x)));
  }
  {
    boost::shared_ptr<int> z((int*)0);
    boost::weak_ptr<int> w;
    BOOST_TEST(comp(z, w) || comp(w, z));
    {
      boost::shared_ptr<int> zz(z);
      w = boost::weak_ptr<int>(zz);
      BOOST_TEST(!(comp(z, zz) || comp(z, zz)));
      BOOST_TEST(!(comp(z, w) || comp(z, w)));
    }
    BOOST_TEST(!(comp(z, w) || comp(w, z)));
  }
  {
    boost::shared_ptr<int> x;
    boost::shared_ptr<int> z((int*)0);
    BOOST_TEST(comp(x, z) || comp(z, x));
  }
  {
    boost::shared_ptr<int> a((int*)0);
    boost::shared_ptr<int> b((int*)0);
    BOOST_TEST(comp(a, b) || comp(b, a));
    boost::weak_ptr<int> w(a);
    BOOST_TEST(!(comp(a, w) || comp(w, a)));
    BOOST_TEST(comp(b, w) || comp(w, b));
  }

  boost::owner_less<boost::weak_ptr<int> > weak_comp;
  {
    boost::shared_ptr<int> a((int*)0);
    boost::weak_ptr<int> wa(a);
    boost::shared_ptr<int> b((int*)0);
    boost::weak_ptr<int> wb(b);
    BOOST_TEST(!(weak_comp(a, wa) || weak_comp(wa, a)));
    BOOST_TEST(!(weak_comp(b, wb) || weak_comp(wb, b)));
    BOOST_TEST(weak_comp(wa, wb) || weak_comp(wb, wa));
    BOOST_TEST(weak_comp(wa, b) || weak_comp(b, wa));
  }

  return boost::report_errors();
}
