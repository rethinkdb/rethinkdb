
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/function_types/components.hpp>
#include <boost/function_types/function_type.hpp>
#include <boost/function_types/member_function_pointer.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;

class C;
typedef void func(C &);
typedef int (C::* mem_func_ptr)(int);

typedef ft::components<func> func_components;
typedef ft::components<mem_func_ptr> mfp_components;

BOOST_MPL_ASSERT((
  mpl::equal< func_components::types, mpl::vector<void,C &> >
));
BOOST_MPL_ASSERT_RELATION(
  ::func_components::function_arity::value, ==, 1
);

BOOST_MPL_ASSERT((
  boost::is_same< ft::function_type< mpl::vector<void,C &> >::type, func >
));


BOOST_MPL_ASSERT((
  mpl::equal< mfp_components::types, mpl::vector<int,C &,int> >
));
BOOST_MPL_ASSERT_RELATION(
  ::mfp_components::function_arity::value, ==, 2
);

BOOST_MPL_ASSERT((
  boost::is_same< ft::member_function_pointer< mpl::vector<int,C &,int> >::type, mem_func_ptr >
));


