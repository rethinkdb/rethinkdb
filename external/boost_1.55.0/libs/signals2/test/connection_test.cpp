// Signals2 library
// tests for connection class

// Copyright Frank Mori Hess 2008
// Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

#include <boost/test/minimal.hpp>
#include <boost/signals2.hpp>

namespace bs2 = boost::signals2;

typedef bs2::signal<void ()> sig_type;

void myslot()
{}

void swap_test()
{
  sig_type sig;

  {
    bs2::connection conn1 = sig.connect(&myslot);
    BOOST_CHECK(conn1.connected());
    bs2::connection conn2;
    BOOST_CHECK(conn2.connected() == false);

    conn1.swap(conn2);
    BOOST_CHECK(conn2.connected());
    BOOST_CHECK(conn1.connected() == false);

    swap(conn1, conn2);
    BOOST_CHECK(conn1.connected());
    BOOST_CHECK(conn2.connected() == false);
  }

  {
    bs2::scoped_connection conn1;
    conn1 = sig.connect(&myslot);
    BOOST_CHECK(conn1.connected());
    bs2::scoped_connection conn2;
    BOOST_CHECK(conn2.connected() == false);

    conn1.swap(conn2);
    BOOST_CHECK(conn2.connected());
    BOOST_CHECK(conn1.connected() == false);

    swap(conn1, conn2);
    BOOST_CHECK(conn1.connected());
    BOOST_CHECK(conn2.connected() == false);
  }
}

void release_test()
{
  sig_type sig;
  bs2::connection conn;
  {
    bs2::scoped_connection scoped(sig.connect(&myslot));
    BOOST_CHECK(scoped.connected());
    conn = scoped.release();
  }
  BOOST_CHECK(conn.connected());

  bs2::connection conn2;
  {
    bs2::scoped_connection scoped(conn);
    BOOST_CHECK(scoped.connected());
    conn = scoped.release();
    BOOST_CHECK(conn.connected());
    BOOST_CHECK(scoped.connected() == false);
    conn.disconnect();

    // earlier release shouldn't affect new connection
    conn2 = sig.connect(&myslot);
    scoped = conn2;
  }
  BOOST_CHECK(conn2.connected() == false);
}

int test_main(int, char*[])
{
  release_test();
  swap_test();
  return 0;
}
