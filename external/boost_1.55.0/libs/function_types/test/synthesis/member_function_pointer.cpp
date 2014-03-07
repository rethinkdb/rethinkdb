
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/function_types/member_function_pointer.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;
using boost::is_same;

class C;

typedef int (C::* expected1)(int);
typedef int (C::* expected2)(int) const;
typedef int (C::* expected3)(int) volatile;
typedef int (C::* expected4)(int) const volatile;


BOOST_MPL_ASSERT((
  is_same< ft::member_function_pointer< mpl::vector<int,C,int> >::type
         , expected1 >
));

BOOST_MPL_ASSERT((
  is_same< ft::member_function_pointer< mpl::vector<int,C,int>, 
              ft::tag<ft::non_const, ft::non_volatile> >::type
         , expected1 >
));

BOOST_MPL_ASSERT((
  is_same< ft::member_function_pointer< mpl::vector<int,C,int>
                                      , ft::const_qualified    >::type
         , expected2 >
));

BOOST_MPL_ASSERT((
  is_same< ft::member_function_pointer< mpl::vector<int,C,int>, 
              ft::tag<ft::const_qualified, ft::non_volatile> >::type
         , expected2 >
));

BOOST_MPL_ASSERT((
  is_same< ft::member_function_pointer< mpl::vector<int,C,int>
                                      , ft::volatile_qualified >::type
         , expected3 >
));

BOOST_MPL_ASSERT((
  is_same< ft::member_function_pointer< mpl::vector<int,C,int>, 
              ft::tag<ft::non_const, ft::volatile_qualified> >::type
         , expected3 >
));

BOOST_MPL_ASSERT((
  is_same< ft::member_function_pointer< mpl::vector<int,C,int>,
              ft::tag<ft::const_qualified, ft::volatile_qualified> >::type
         , expected4 >
));

