
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
//
// Reimplementation of the Boost result_of utility (see [Gregor01] and 
// [Gregor02]).
//
//
// Detailed description
// ====================
//
// This example implements the functionality of the Boost result_of utility.
// Because of FunctionTypes we get away without repetitive code and the Boost
// Preprocessor library.
//
//
// Bibliography
// ============
//
// [Gregor01] Gregor, D. The Boost result_of utility
//            http://www.boost.org/libs/utility
//
// [Gregor02] Gregor, D. A uniform method for computing function object return
//            types (revision 1)
//            http://anubis.dkuug.dk/jtc1/sc22/wg21/docs/papers/2003/n1454.html

#include <boost/function_types/result_type.hpp>
#include <boost/function_types/is_callable_builtin.hpp>

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/has_xxx.hpp>

namespace example
{
  namespace ft = boost::function_types;
  namespace mpl = boost::mpl;

  template<typename F> struct result_of;

  namespace detail
  {
    BOOST_MPL_HAS_XXX_TRAIT_DEF(result_type)

    template<typename F>
    struct result_type_member
    {
      typedef typename F::result_type type;
    };

    template<typename F, typename Desc>
    struct result_member_template
    {
      typedef typename F::template result<Desc>::type type;
    };

#if !BOOST_WORKAROUND(__BORLANDC__,BOOST_TESTED_AT(0x564))
    template<typename F> 
    struct result_member_template< F, F(void) > 
    { 
      typedef void type; 
    };
#endif

    template<typename F, typename Desc>
    struct result_of_impl
      : mpl::eval_if
        < ft::is_callable_builtin<F>
        , ft::result_type<F>
        , mpl::eval_if
          < has_result_type<F> 
          , result_type_member<F>
          , result_member_template<F,Desc>
        > > 
    { };
  }

  template<typename Desc>
  struct result_of
    : detail::result_of_impl< typename ft::result_type<Desc>::type, Desc >
  { };
}

