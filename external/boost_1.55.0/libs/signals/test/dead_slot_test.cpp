// Boost.Signals library

// Copyright (C) Douglas Gregor 2001-2006. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/signal.hpp>
#include <boost/bind.hpp>

typedef boost::signal1<int, int> sig_type;

class with_constant : public boost::BOOST_SIGNALS_NAMESPACE::trackable {
public:
  with_constant(int c) : constant(c) {}

  int add(int i) { return i + constant; }

private:
  int constant;
};

void do_delayed_connect(with_constant* wc,
                        sig_type& sig,
                        sig_type::slot_type slot)
{
  // Should invalidate the slot, so that we cannot connect to it
  delete wc;

  boost::BOOST_SIGNALS_NAMESPACE::connection c = sig.connect(slot);
  BOOST_CHECK(!c.connected());
}

int test_main(int, char*[])
{
  sig_type s1;
  with_constant* wc1 = new with_constant(7);

  do_delayed_connect(wc1, s1, boost::bind(&with_constant::add, wc1, _1));

  return 0;
}
