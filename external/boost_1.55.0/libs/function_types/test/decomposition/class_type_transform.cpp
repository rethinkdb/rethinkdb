
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------

#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/add_pointer.hpp>

#include <boost/mpl/assert.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/mpl/always.hpp>

#include <boost/function_types/components.hpp>
#include <boost/function_types/parameter_types.hpp>

using namespace boost;
namespace ft = function_types;
using boost::mpl::placeholders::_;

class C; 
typedef C (C::*mem_func_ptr)();
class X;

BOOST_MPL_ASSERT((
  is_same< mpl::at_c< 
    ft::components<mem_func_ptr, add_pointer<_> >
  ,1 >::type, C* >
));
BOOST_MPL_ASSERT((
  is_same< mpl::at_c< 
    ft::components<mem_func_ptr, add_pointer< add_pointer<_> > >
  ,1 >::type, C** >
));
BOOST_MPL_ASSERT((
  is_same< mpl::at_c< 
    ft::components<mem_func_ptr, mpl::always<X> >
  ,1 >::type, X >
));

BOOST_MPL_ASSERT((
  is_same< mpl::at_c< 
    ft::parameter_types<mem_func_ptr, add_pointer<_> >
  ,0 >::type, C* >
));
BOOST_MPL_ASSERT((
  is_same< mpl::at_c< 
    ft::parameter_types<mem_func_ptr, add_pointer< add_pointer<_> > >
  ,0 >::type, C** >
));
BOOST_MPL_ASSERT((
  is_same< mpl::at_c< 
    ft::parameter_types<mem_func_ptr, mpl::always<X> >
  ,0 >::type, X >
));




