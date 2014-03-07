
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/equal.hpp>

#include <boost/function_types/parameter_types.hpp>


namespace ft = boost::function_types;
namespace mpl = boost::mpl;

class C;
typedef C func(C);
typedef C (*func_ptr)(C,int);
typedef C (&func_ref)();
typedef C (C::*mem_func_ptr)(C,int);
typedef C (C::*c_mem_func_ptr)(C,C) const;
typedef C (C::*v_mem_func_ptr)(C) volatile;
typedef C (C::*cv_mem_func_ptr)() const volatile;
typedef C C::*mem_ptr;

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<func>, mpl::vector<C> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<func_ptr>::type, mpl::vector<C,int> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<func_ref>, mpl::vector<> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<mem_func_ptr>, mpl::vector<C &,C,int> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<c_mem_func_ptr>, mpl::vector<C const &,C,C> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<v_mem_func_ptr>, mpl::vector<C volatile &,C> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<cv_mem_func_ptr>, mpl::vector<C const volatile &> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::parameter_types<mem_ptr>, mpl::vector<C &> >
));
