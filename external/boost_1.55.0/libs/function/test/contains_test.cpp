// Boost.Function library

//  Copyright Douglas Gregor 2004. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/test/minimal.hpp>
#include <boost/function.hpp>
#include <boost/ref.hpp>

static int forty_two() { return 42; }

struct Seventeen
{
  int operator()() const { return 17; }
};

struct ReturnInt
{
  explicit ReturnInt(int value) : value(value) {}

  int operator()() const { return value; }

  int value;
};

bool operator==(const ReturnInt& x, const ReturnInt& y)
{ return x.value == y.value; }

bool operator!=(const ReturnInt& x, const ReturnInt& y)
{ return x.value != y.value; }

namespace contain_test {

struct ReturnIntFE
{
  explicit ReturnIntFE(int value) : value(value) {}

  int operator()() const { return value; }

  int value;
};

}

#ifndef BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP

namespace contain_test {
# ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
bool function_equal(const ReturnIntFE& x, const ReturnIntFE& y)
{ return x.value == y.value; }
# else
bool function_equal_impl(const ReturnIntFE& x, const ReturnIntFE& y, int)
{ return x.value == y.value; }
# endif // #ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
}
#else // BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP
namespace boost {
# ifndef BOOST_NO_FUNCTION_TEMPLATE_ORDERING
bool 
function_equal(const contain_test::ReturnIntFE& x, 
               const contain_test::ReturnIntFE& y)
{ return x.value == y.value; }
# else
bool 
function_equal_impl(const contain_test::ReturnIntFE& x, 
                    const contain_test::ReturnIntFE& y, int)
{ return x.value == y.value; }
# endif
}
#endif

static void target_test()
{
  boost::function0<int> f;

  f = &forty_two;
  BOOST_CHECK(*f.target<int (*)()>() == &forty_two);
  BOOST_CHECK(!f.target<Seventeen>());

  f = Seventeen();
  BOOST_CHECK(!f.target<int (*)()>());
  BOOST_CHECK(f.target<Seventeen>());

  Seventeen this_seventeen;
  f = boost::ref(this_seventeen);
  BOOST_CHECK(!f.target<int (*)()>());
  BOOST_CHECK(f.target<Seventeen>());
  BOOST_CHECK(f.target<Seventeen>() == &this_seventeen);

  const Seventeen const_seventeen = this_seventeen;
  f = boost::ref(const_seventeen);
  BOOST_CHECK(!f.target<int (*)()>());
  BOOST_CHECK(f.target<const Seventeen>());
  BOOST_CHECK(f.target<const Seventeen>() == &const_seventeen);
  BOOST_CHECK(f.target<const volatile Seventeen>());
  BOOST_CHECK(!f.target<Seventeen>());
  BOOST_CHECK(!f.target<volatile Seventeen>());
}

static void equal_test()
{
  boost::function0<int> f;

  f = &forty_two;
  BOOST_CHECK(f == &forty_two);
  BOOST_CHECK(f != ReturnInt(17));
#if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
  BOOST_CHECK(&forty_two == f);
  BOOST_CHECK(ReturnInt(17) != f);
#endif

  BOOST_CHECK(f.contains(&forty_two));

  f = ReturnInt(17);
  BOOST_CHECK(f != &forty_two);
  BOOST_CHECK(f == ReturnInt(17));
  BOOST_CHECK(f != ReturnInt(16));
#if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
  BOOST_CHECK(&forty_two != f);
  BOOST_CHECK(ReturnInt(17) == f);
  BOOST_CHECK(ReturnInt(16) != f);
#endif

  BOOST_CHECK(f.contains(ReturnInt(17)));

  f = contain_test::ReturnIntFE(17);
  BOOST_CHECK(f != &forty_two);
  BOOST_CHECK(f == contain_test::ReturnIntFE(17));
  BOOST_CHECK(f != contain_test::ReturnIntFE(16));
#if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
  BOOST_CHECK(&forty_two != f);
  BOOST_CHECK(contain_test::ReturnIntFE(17) == f);
  BOOST_CHECK(contain_test::ReturnIntFE(16) != f);
#endif

  BOOST_CHECK(f.contains(contain_test::ReturnIntFE(17)));

#if !defined(BOOST_FUNCTION_NO_FUNCTION_TYPE_SYNTAX)
  boost::function<int(void)> g;

  g = &forty_two;
  BOOST_CHECK(g == &forty_two);
  BOOST_CHECK(g != ReturnInt(17));
#  if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
  BOOST_CHECK(&forty_two == g);
  BOOST_CHECK(ReturnInt(17) != g);
#  endif

  g = ReturnInt(17);
  BOOST_CHECK(g != &forty_two);
  BOOST_CHECK(g == ReturnInt(17));
  BOOST_CHECK(g != ReturnInt(16));
#  if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
  BOOST_CHECK(&forty_two != g);
  BOOST_CHECK(ReturnInt(17) == g);
  BOOST_CHECK(ReturnInt(16) != g);
#  endif
#endif
}

static void ref_equal_test()
{
  {
    ReturnInt ri(17);
    boost::function0<int> f = boost::ref(ri);

    // References and values are equal
    BOOST_CHECK(f == boost::ref(ri));
    BOOST_CHECK(f == ri);
    BOOST_CHECK(boost::ref(ri) == f);
    BOOST_CHECK(!(f != boost::ref(ri)));
    BOOST_CHECK(!(f != ri));
    BOOST_CHECK(!(boost::ref(ri) != f));
#if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
    BOOST_CHECK(ri == f);
    BOOST_CHECK(!(ri != f));
#endif

    // Values equal, references inequal
    ReturnInt ri2(17);
    BOOST_CHECK(f == ri2);
    BOOST_CHECK(f != boost::ref(ri2));
    BOOST_CHECK(boost::ref(ri2) != f);
    BOOST_CHECK(!(f != ri2));
    BOOST_CHECK(!(f == boost::ref(ri2)));
    BOOST_CHECK(!(boost::ref(ri2) == f));
#if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
    BOOST_CHECK(ri2 == f);
    BOOST_CHECK(!(ri2 != f));
#endif 
  }

#if !defined(BOOST_FUNCTION_NO_FUNCTION_TYPE_SYNTAX)
  {
    ReturnInt ri(17);
    boost::function<int(void)> f = boost::ref(ri);

    // References and values are equal
    BOOST_CHECK(f == boost::ref(ri));
    BOOST_CHECK(f == ri);
    BOOST_CHECK(boost::ref(ri) == f);
    BOOST_CHECK(!(f != boost::ref(ri)));
    BOOST_CHECK(!(f != ri));
    BOOST_CHECK(!(boost::ref(ri) != f));
#  if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
    BOOST_CHECK(ri == f);
    BOOST_CHECK(!(ri != f));
#  endif

    // Values equal, references inequal
    ReturnInt ri2(17);
    BOOST_CHECK(f == ri2);
    BOOST_CHECK(f != boost::ref(ri2));
    BOOST_CHECK(boost::ref(ri2) != f);
    BOOST_CHECK(!(f != ri2));
    BOOST_CHECK(!(f == boost::ref(ri2)));
    BOOST_CHECK(!(boost::ref(ri2) == f));
#  if !(defined(__GNUC__) && __GNUC__ == 3 && __GNUC_MINOR__ <= 3)
    BOOST_CHECK(ri2 == f);
    BOOST_CHECK(!(ri2 != f));
#  endif
  }
#endif
}

int test_main(int, char*[])
{
  target_test();
  equal_test();
  ref_equal_test();

  return 0;
}
