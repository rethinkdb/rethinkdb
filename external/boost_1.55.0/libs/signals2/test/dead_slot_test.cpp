// Boost.Signals library

// Copyright (C) Douglas Gregor 2001-2006. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/shared_ptr.hpp>
#include <boost/test/minimal.hpp>
#include <boost/signals2.hpp>
#include <boost/bind.hpp>

typedef boost::signals2::signal<int (int)> sig_type;

class with_constant {
public:
  with_constant(int c) : constant(c) {}

  int add(int i) { return i + constant; }

private:
  int constant;
};

void do_delayed_connect(boost::shared_ptr<with_constant> &wc,
                        sig_type& sig,
                        sig_type::slot_type slot)
{
  // Should invalidate the slot, so that we cannot connect to it
  wc.reset();

  boost::signals2::connection c = sig.connect(slot);
  BOOST_CHECK(!c.connected());
}

int test_main(int, char*[])
{
  sig_type s1;
  boost::shared_ptr<with_constant> wc1(new with_constant(7));

  do_delayed_connect(wc1, s1, sig_type::slot_type(&with_constant::add, wc1.get(), _1).track(wc1));

  return 0;
}
