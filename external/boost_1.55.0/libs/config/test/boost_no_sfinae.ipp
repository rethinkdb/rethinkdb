// (C) Copyright Eric Friedman 2003. 
// Some modifications by Jeremiah Willcock and Jaakko Jarvi.
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  MACRO:         BOOST_NO_SFINAE
//  TITLE:         SFINAE (substitution failure is not an error)
//  DESCRIPTION:   SFINAE not supported.


namespace boost_no_sfinae {

namespace f1_a {
template <typename T>
int f1(T*, float)
{
  return 0;
}
} using f1_a::f1;

namespace f1_b {
template <typename T>
int f1(T*, int, typename T::int_* = 0)
{
  return 1;
}
} using f1_b::f1;

namespace f2_a {
template <typename T>
int f2(T*, float)
{
  return 2;
}
} using f2_a::f2;

namespace f2_b {
template <typename T>
typename T::int_ f2(T*, int)
{
  return 3;
}
} using f2_b::f2;

struct test_t
{
  typedef int int_;
};

struct test2_t {};

int test()
{
  test_t* t = 0;
  test2_t* t2 = 0;
  bool correct = 
    (f1(t, 0) == 1) &&
    (f1(t2, 0) == 0) &&
    (f2(t, 0) == 3) &&
    (f2(t2, 0) == 2);
  return !correct;
}

}



