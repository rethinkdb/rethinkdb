/* Boost test/cmp_exp.cpp
 * test explicit comparison functions
 *
 * Copyright 2002-2003 Guillaume Melquiond
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or
 * copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include "cmp_header.hpp"

// comparisons between [1,2] and [3,4]

static void test_12_34() {
  const I a(1,2), b(3,4);

  BOOST_CHECK(cerlt(a, b));
  BOOST_CHECK(cerle(a, b));
  BOOST_CHECK(!cergt(a, b));
  BOOST_CHECK(!cerge(a, b));

  BOOST_CHECK(!cerlt(b, a));
  BOOST_CHECK(!cerle(b, a));
  BOOST_CHECK(cergt(b, a));
  BOOST_CHECK(cerge(b, a));

  BOOST_CHECK(poslt(a, b));
  BOOST_CHECK(posle(a, b));
  BOOST_CHECK(!posgt(a, b));
  BOOST_CHECK(!posge(a, b));

  BOOST_CHECK(!poslt(b, a));
  BOOST_CHECK(!posle(b, a));
  BOOST_CHECK(posgt(b, a));
  BOOST_CHECK(posge(b, a));

  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(cerne(a, b));
  BOOST_CHECK(!poseq(a, b));
  BOOST_CHECK(posne(a, b));

  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(cerne(b, a));
  BOOST_CHECK(!poseq(b, a));
  BOOST_CHECK(posne(b, a));

# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

// comparisons between [1,3] and [2,4]

static void test_13_24() {
  const I a(1,3), b(2,4);

  BOOST_CHECK(!cerlt(a, b));
  BOOST_CHECK(!cerle(a, b));
  BOOST_CHECK(!cergt(a, b));
  BOOST_CHECK(!cerge(a, b));

  BOOST_CHECK(!cerlt(b, a));
  BOOST_CHECK(!cerle(b, a));
  BOOST_CHECK(!cergt(b, a));
  BOOST_CHECK(!cerge(b, a));

  BOOST_CHECK(poslt(a, b));
  BOOST_CHECK(posle(a, b));
  BOOST_CHECK(posgt(a, b));
  BOOST_CHECK(posge(a, b));

  BOOST_CHECK(poslt(b, a));
  BOOST_CHECK(posle(b, a));
  BOOST_CHECK(posgt(b, a));
  BOOST_CHECK(posge(b, a));

  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(!cerne(a, b));
  BOOST_CHECK(poseq(a, b));
  BOOST_CHECK(posne(a, b));

  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(!cerne(b, a));
  BOOST_CHECK(poseq(b, a));
  BOOST_CHECK(posne(b, a));

# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

// comparisons between [1,2] and [2,3]

static void test_12_23() {
  const I a(1,2), b(2,3);

  BOOST_CHECK(!cerlt(a, b));
  BOOST_CHECK(cerle(a, b));
  BOOST_CHECK(!cergt(a, b));
  BOOST_CHECK(!cerge(a, b));

  BOOST_CHECK(!cerlt(b, a));
  BOOST_CHECK(!cerle(b, a));
  BOOST_CHECK(!cergt(b, a));
  BOOST_CHECK(cerge(b, a));

  BOOST_CHECK(poslt(a, b));
  BOOST_CHECK(posle(a, b));
  BOOST_CHECK(!posgt(a, b));
  BOOST_CHECK(posge(a, b));

  BOOST_CHECK(!poslt(b, a));
  BOOST_CHECK(posle(b, a));
  BOOST_CHECK(posgt(b, a));
  BOOST_CHECK(posge(b, a));

  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(!cerne(a, b));
  BOOST_CHECK(poseq(a, b));
  BOOST_CHECK(posne(a, b));

  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(!cerne(b, a));
  BOOST_CHECK(poseq(b, a));
  BOOST_CHECK(posne(b, a));

# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

// comparisons between [1,2] and 0

static void test_12_0() {
  const I a(1,2);
  const int b = 0;

  BOOST_CHECK(!cerlt(a, b));
  BOOST_CHECK(!cerle(a, b));
  BOOST_CHECK(cergt(a, b));
  BOOST_CHECK(cerge(a, b));

  BOOST_CHECK(cerlt(b, a));
  BOOST_CHECK(cerle(b, a));
  BOOST_CHECK(!cergt(b, a));
  BOOST_CHECK(!cerge(b, a));

  BOOST_CHECK(!poslt(a, b));
  BOOST_CHECK(!posle(a, b));
  BOOST_CHECK(posgt(a, b));
  BOOST_CHECK(posge(a, b));

  BOOST_CHECK(poslt(b, a));
  BOOST_CHECK(posle(b, a));
  BOOST_CHECK(!posgt(b, a));
  BOOST_CHECK(!posge(b, a));

  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(cerne(a, b));
  BOOST_CHECK(!poseq(a, b));
  BOOST_CHECK(posne(a, b));

  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(cerne(b, a));
  BOOST_CHECK(!poseq(b, a));
  BOOST_CHECK(posne(b, a));

# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

// comparisons between [1,2] and 1

static void test_12_1() {
  const I a(1,2);
  const int b = 1;

  BOOST_CHECK(!cerlt(a, b));
  BOOST_CHECK(!cerle(a, b));
  BOOST_CHECK(!cergt(a, b));
  BOOST_CHECK(cerge(a, b));

  BOOST_CHECK(!cerlt(b, a));
  BOOST_CHECK(cerle(b, a));
  BOOST_CHECK(!cergt(b, a));
  BOOST_CHECK(!cerge(b, a));

  BOOST_CHECK(!poslt(a, b));
  BOOST_CHECK(posle(a, b));
  BOOST_CHECK(posgt(a, b));
  BOOST_CHECK(posge(a, b));

  BOOST_CHECK(poslt(b, a));
  BOOST_CHECK(posle(b, a));
  BOOST_CHECK(!posgt(b, a));
  BOOST_CHECK(posge(b, a));

  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(!cerne(a, b));
  BOOST_CHECK(poseq(a, b));
  BOOST_CHECK(posne(a, b));

  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(!cerne(b, a));
  BOOST_CHECK(poseq(b, a));
  BOOST_CHECK(posne(b, a));

# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

// comparisons between [1,2] and 2

static void test_12_2() {
  const I a(1,2);
  const int b = 2;

  BOOST_CHECK(!cerlt(a, b));
  BOOST_CHECK(cerle(a, b));
  BOOST_CHECK(!cergt(a, b));
  BOOST_CHECK(!cerge(a, b));

  BOOST_CHECK(!cerlt(b, a));
  BOOST_CHECK(!cerle(b, a));
  BOOST_CHECK(!cergt(b, a));
  BOOST_CHECK(cerge(b, a));

  BOOST_CHECK(poslt(a, b));
  BOOST_CHECK(posle(a, b));
  BOOST_CHECK(!posgt(a, b));
  BOOST_CHECK(posge(a, b));

  BOOST_CHECK(!poslt(b, a));
  BOOST_CHECK(posle(b, a));
  BOOST_CHECK(posgt(b, a));
  BOOST_CHECK(posge(b, a));

  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(!cerne(a, b));
  BOOST_CHECK(poseq(a, b));
  BOOST_CHECK(posne(a, b));

  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(!cerne(b, a));
  BOOST_CHECK(poseq(b, a));
  BOOST_CHECK(posne(b, a));

# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

// comparisons between [1,2] and 3

static void test_12_3() {
  const I a(1,2);
  const int b = 3;

  BOOST_CHECK(cerlt(a, b));
  BOOST_CHECK(cerle(a, b));
  BOOST_CHECK(!cergt(a, b));
  BOOST_CHECK(!cerge(a, b));

  BOOST_CHECK(!cerlt(b, a));
  BOOST_CHECK(!cerle(b, a));
  BOOST_CHECK(cergt(b, a));
  BOOST_CHECK(cerge(b, a));

  BOOST_CHECK(poslt(a, b));
  BOOST_CHECK(posle(a, b));
  BOOST_CHECK(!posgt(a, b));
  BOOST_CHECK(!posge(a, b));

  BOOST_CHECK(!poslt(b, a));
  BOOST_CHECK(!posle(b, a));
  BOOST_CHECK(posgt(b, a));
  BOOST_CHECK(posge(b, a));

  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(cerne(a, b));
  BOOST_CHECK(!poseq(a, b));
  BOOST_CHECK(posne(a, b));

  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(cerne(b, a));
  BOOST_CHECK(!poseq(b, a));
  BOOST_CHECK(posne(b, a));

# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

static void test_12_12() {
  const I a(1,2), b(1,2);
  BOOST_CHECK(!cereq(a, b));
  BOOST_CHECK(!cerne(a, b));
  BOOST_CHECK(poseq(a, b));
  BOOST_CHECK(posne(a, b));
  BOOST_CHECK(!cereq(b, a));
  BOOST_CHECK(!cerne(b, a));
  BOOST_CHECK(poseq(b, a));
  BOOST_CHECK(posne(b, a));
# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

static void test_11_11() {
  const I a(1,1), b(1,1);
  BOOST_CHECK(cereq(a, b));
  BOOST_CHECK(!cerne(a, b));
  BOOST_CHECK(poseq(a, b));
  BOOST_CHECK(!posne(a, b));
  BOOST_CHECK(cereq(b, a));
  BOOST_CHECK(!cerne(b, a));
  BOOST_CHECK(poseq(b, a));
  BOOST_CHECK(!posne(b, a));
# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

static void test_11_1() {
  const I a(1,1);
  const int b = 1;
  BOOST_CHECK(cereq(a, b));
  BOOST_CHECK(!cerne(a, b));
  BOOST_CHECK(poseq(a, b));
  BOOST_CHECK(!posne(a, b));
  BOOST_CHECK(cereq(b, a));
  BOOST_CHECK(!cerne(b, a));
  BOOST_CHECK(poseq(b, a));
  BOOST_CHECK(!posne(b, a));
# ifdef __BORLANDC__
  ::detail::ignore_unused_variable_warning(a);
  ::detail::ignore_unused_variable_warning(b);
# endif
}

int test_main(int, char *[]) {
  test_12_34();
  test_13_24();
  test_12_23();
  test_12_0();
  test_12_1();
  test_12_2();
  test_12_3();
  test_12_12();
  test_11_11();
  test_11_1();

  return 0;
}
