
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/mpl/assert.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/equal.hpp>

#include <boost/function_types/components.hpp>

namespace ft = boost::function_types;
namespace mpl = boost::mpl;

class C; 
typedef C func();
typedef C (*func_ptr)(int);
typedef C (&func_ref)(int,int);
typedef C (C::*mem_func_ptr)();
typedef C (C::*c_mem_func_ptr)(int) const;
typedef C (C::*v_mem_func_ptr)(int,int) volatile;
typedef C (C::*cv_mem_func_ptr)(int,int,int) const volatile;
typedef int C::* mem_ptr;

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<func>::types, mpl::vector<C> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<func_ptr>::types, mpl::vector<C,int> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<func_ref>::types, mpl::vector<C,int,int> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<mem_func_ptr>::types, mpl::vector<C,C &> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<c_mem_func_ptr>::types, mpl::vector<C,C const &,int> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<v_mem_func_ptr>::types, mpl::vector<C,C volatile &,int,int> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<cv_mem_func_ptr>::types, mpl::vector<C,C const volatile &,int,int,int> >
));

BOOST_MPL_ASSERT((
  mpl::equal< ft::components<mem_ptr>::types, mpl::vector<int &,C&> >
));

