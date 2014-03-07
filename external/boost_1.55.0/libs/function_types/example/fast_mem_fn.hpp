
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
//
// This example implements a very efficient, generic member function wrapper.
//
//
// Detailed description
// ====================
//
// For most platforms C++ runs on (maybe all hardware platforms, as opposed to
// virtual machines) there are indirect calls that take more time to execute 
// than direct ones. Further calling a function usually takes more time than 
// inlining it at the call site.
//
// A direct call is a machine instruction that calls a subroutine at a known
// address encoded in the instruction itself. C++ compilers usually emit one of
// these instructions for each function call to a nonvirtual function (a call to
// a virtual function requires either two direct calls or one indirect call).
// An indirect call is a machine instruction that calls a subroutine at an 
// address known at runtime. C++ compilers usually emit at least one of these 
// instructions for a call through a callable builtin variable.
//
// It is possible to use callable scalars as non-type template arguments. This
// way the compiler knows which function we want to call when generating the
// code for the call site, so it may inline (if it decides to do so) or use a 
// direct call instead of being forced to use a slow, indirect call.
//
// We define a functor class template that encodes the function to call in its
// type via a non-type template argument. Its (inline declared) overloaded 
// function call operator calls the function through that non-type template 
// argument. In the best case we end up inlining the callee directly at the
// point of the call.
//
// Decomposition of the wrapped member function's type is needed in order to 
// implement argument forwarding (just using a templated call operator we would 
// encounter what is known as "the forwarding problem" [Dimov1]). Further we
// can eliminate unecessary copies for each by-value parameter by using a 
// reference to its const qualified type for the corresponding parameter of the
// wrapper's function call operator.
//
// Finally we provide a macro that does have similar semantics to the function 
// template mem_fn of the Bind [2] library.
// We can't use a function template and use a macro instead, because we use a
// member function pointer that is a compile time constant. So we first have to
// deduce the type and create a template that accepts this type as a non-type 
// template argument, which is passed in in a second step. The macro hides this
// lengthy expression from the user.
//
//
// Limitations
// ===========
//
// The "this-argument" must be specified as a reference.
//
//
// Bibliography
// ============
//
// [Dimov1] Dimov, P., Hinnant H., Abrahams, D. The Forwarding Problem
//          http://std.dkuug.dk/jtc1/sc22/wg21/docs/papers/2002/n1385.htm
//
// [Dimov2] Dimov, P. Documentation of boost::mem_fn
//          http://www.boost.org/libs/bind/mem_fn.html

#ifndef BOOST_EXAMPLE_FAST_MEM_FN_HPP_INCLUDED
#ifndef BOOST_PP_IS_ITERATING


#include <boost/function_types/result_type.hpp>
#include <boost/function_types/function_arity.hpp>
#include <boost/function_types/parameter_types.hpp>
#include <boost/function_types/is_member_function_pointer.hpp>

#include <boost/mpl/transform_view.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/next.hpp>
#include <boost/mpl/deref.hpp>

#include <boost/utility/enable_if.hpp>

#include "detail/param_type.hpp"

namespace example
{

  namespace ft = boost::function_types;
  namespace mpl = boost::mpl;
  using namespace mpl::placeholders;

  // the functor class template
  template< typename MFPT, MFPT MemberFunction
    , size_t Arity = ::example::ft::function_arity<MFPT>::value
  >
  struct fast_mem_fn;

  // ------- ---- --- -- - - - -

  // deduce type and capture compile time value 
  #define BOOST_EXAMPLE_FAST_MEM_FN(mfp) \
      ::example::make_fast_mem_fn(mfp).make_fast_mem_fn<mfp>()

  template<typename MFPT>
  struct fast_mem_fn_maker
  {
    template<MFPT Callee>
    fast_mem_fn<MFPT,Callee> make_fast_mem_fn()
    {
      return fast_mem_fn<MFPT,Callee>();
    } 
  };

  template<typename MFPT>
      typename boost::enable_if<boost::is_member_function_pointer<MFPT>, 
  fast_mem_fn_maker<MFPT>                                                >::type
  make_fast_mem_fn(MFPT)
  {
    return fast_mem_fn_maker<MFPT>();
  }


  // ------- ---- --- -- - - - -

  namespace detail
  {
    // by-value forwarding optimization
    template<typename T>
    struct parameter_types
      : mpl::transform_view<ft::parameter_types<T>,param_type<_> >
    { }; 
  }

  // ------- ---- --- -- - - - -

  template< typename MFPT, MFPT MemberFunction >
  struct fast_mem_fn<MFPT, MemberFunction, 1>
  {
    // decompose the result and the parameter types (public for introspection)
    typedef typename ft::result_type<MFPT>::type result_type;
    typedef detail::parameter_types<MFPT> parameter_types;
  private:
    // iterate the parameter types 
    typedef typename mpl::begin<parameter_types>::type i0;
  public: 
    // forwarding function call operator
    result_type operator()( typename mpl::deref<i0>::type a0) const
    {
      return (a0.*MemberFunction)();
    };
  };

  template< typename MFPT, MFPT MemberFunction >
  struct fast_mem_fn<MFPT, MemberFunction, 2>
  {
    // decompose the result and the parameter types (public for introspection)
    typedef typename ft::result_type<MFPT>::type result_type;
    typedef detail::parameter_types<MFPT> parameter_types;
  private:
    // iterate the parameter types 
    typedef typename mpl::begin<parameter_types>::type i0;
    typedef typename mpl::next<i0>::type i1;
  public: 
    // forwarding function call operator
    result_type operator()( typename mpl::deref<i0>::type a0
                          , typename mpl::deref<i1>::type a1) const
    {
      return (a0.*MemberFunction)(a1);
    };
  };

  template< typename MFPT, MFPT MemberFunction >
  struct fast_mem_fn<MFPT, MemberFunction, 3>
  {
    // decompose the result and the parameter types (public for introspection)
    typedef typename ft::result_type<MFPT>::type result_type;
    typedef detail::parameter_types<MFPT> parameter_types;
  private:
    // iterate the parameter types 
    typedef typename mpl::begin<parameter_types>::type i0;
    typedef typename mpl::next<i0>::type i1;
    typedef typename mpl::next<i1>::type i2;
  public: 
    // forwarding function call operator
    result_type operator()( typename mpl::deref<i0>::type a0
                          , typename mpl::deref<i1>::type a1
                          , typename mpl::deref<i2>::type a2) const
    {
      return (a0.*MemberFunction)(a1,a2);
    };
  };

  // ...
}

// ------- ---- --- -- - - - -

// preprocessor-based code generator to continue the repetitive part, above

#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>

namespace example
{
  #if BOOST_FT_MAX_ARITY >= 4
  #   define  BOOST_PP_FILENAME_1 "fast_mem_fn.hpp"
  #   define  BOOST_PP_ITERATION_LIMITS (4,BOOST_FT_MAX_ARITY)
  #   include BOOST_PP_ITERATE()
  #endif
}

#define BOOST_EXAMPLE_FAST_MEM_FN_HPP_INCLUDED
#else

  #define N BOOST_PP_FRAME_ITERATION(1)
  template< typename MFPT, MFPT MemberFunction >
  struct fast_mem_fn<MFPT, MemberFunction, N >
  {
    // decompose the result and the parameter types (public for introspection)
    typedef typename ft::result_type<MFPT>::type result_type;
    typedef detail::parameter_types<MFPT> parameter_types;
  private:
    // iterate the parameter types 
    typedef typename mpl::begin<parameter_types>::type i0;
    #define  BOOST_PP_LOCAL_LIMITS (0,N-2)
    #define  BOOST_PP_LOCAL_MACRO(j) \
    typedef typename mpl::next< i ## j >::type BOOST_PP_CAT(i,BOOST_PP_INC(j)) ;
    #include BOOST_PP_LOCAL_ITERATE()
  public: 
    // forwarding function call operator
    result_type operator()( 
        BOOST_PP_ENUM_BINARY_PARAMS(N, typename mpl::deref<i,>::type a) )  const
    {
      return (a0.*MemberFunction)(BOOST_PP_ENUM_SHIFTED_PARAMS(N,a));
    };
  };
  #undef N

#endif
#endif

