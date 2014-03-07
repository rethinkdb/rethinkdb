
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/function_types/is_function.hpp>

namespace ft = boost::function_types;

template<typename C, typename T>
void test_non_cv(T C::*)
{
  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::non_const > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::non_volatile > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::tag<ft::non_const,ft::non_volatile> > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::const_qualified > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::volatile_qualified > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::tag<ft::const_qualified,ft::volatile_qualified> > 
  ));
}

template<typename C, typename T>
void test_c_non_v(T C::*)
{
  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::const_qualified > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::non_volatile > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::tag<ft::const_qualified,ft::non_volatile> > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::non_const > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::volatile_qualified > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::tag<ft::non_const,ft::volatile_qualified> > 
  ));
}

template<typename C, typename T>
void test_v_non_c(T C::*)
{
  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::non_const > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::volatile_qualified > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::tag<ft::non_const,ft::volatile_qualified> > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::const_qualified > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::non_volatile > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::tag<ft::const_qualified,ft::non_volatile> > 
  ));
}

template<typename C, typename T>
void test_cv(T C::*)
{
  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::const_qualified > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::volatile_qualified > 
  ));

  BOOST_MPL_ASSERT(( 
    ft::is_function<T, ft::tag<ft::const_qualified,ft::volatile_qualified> > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::non_const > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::non_volatile > 
  ));

  BOOST_MPL_ASSERT_NOT(( 
    ft::is_function<T, ft::tag<ft::non_const,ft::non_volatile> > 
  ));
}


struct C
{
  void non_cv(int) { }
  void c_non_v(int) const { }
  void v_non_c(int) volatile { }
  void cv(int) const volatile { }
};

void instanitate()
{
  test_non_cv(& C::non_cv);
  test_c_non_v(& C::c_non_v);
  test_v_non_c(& C::v_non_c);
  test_cv(& C::cv);
}

