
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/function_types/function_type.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;

template<typename C, typename T>
void test_non_cv(T C::*)
{
  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::non_const,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_const >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_volatile >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::const_qualified,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::const_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::non_const,ft::volatile_qualified> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( 
    boost::is_same< 
      ft::function_type< mpl::vector<void,int>, ft::volatile_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::const_qualified,ft::volatile_qualified> >::type 
  , T 
  >));
}

template<typename C, typename T>
void test_c_non_v(T C::*)
{
  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::non_const,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_const >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_volatile >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::const_qualified,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::const_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::non_const,ft::volatile_qualified> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( 
    boost::is_same< 
      ft::function_type< mpl::vector<void,int>, ft::volatile_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::const_qualified,ft::volatile_qualified> >::type 
  , T 
  >));
}

template<typename C, typename T>
void test_v_non_c(T C::*)
{
  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::non_const,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_const >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_volatile >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::const_qualified,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::const_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::non_const,ft::volatile_qualified> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( 
    boost::is_same< 
      ft::function_type< mpl::vector<void,int>, ft::volatile_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::const_qualified,ft::volatile_qualified> >::type 
  , T 
  >));
}

template<typename C, typename T>
void test_cv(T C::*)
{
  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::non_const,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_const >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::non_volatile >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
        ft::tag<ft::const_qualified,ft::non_volatile> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, ft::const_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::non_const,ft::volatile_qualified> >::type 
  , T 
  >));

  BOOST_MPL_ASSERT_NOT(( 
    boost::is_same< 
      ft::function_type< mpl::vector<void,int>, ft::volatile_qualified >::type 
  , T 
  >));

  BOOST_MPL_ASSERT(( boost::is_same< 
    ft::function_type< mpl::vector<void,int>, 
       ft::tag<ft::const_qualified,ft::volatile_qualified> >::type 
  , T 
  >));
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

