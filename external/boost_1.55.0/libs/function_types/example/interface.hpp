
// (C) Copyright Tobias Schwinger
//
// Use modification and distribution are subject to the boost Software License,
// Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt).

//------------------------------------------------------------------------------
//
// This example implements interfaces. 
// 
// Detailed description
// ====================
//
// An interface is a collection of member function prototypes that may be
// implemented by classes. Objects of classes that implement the interface can 
// then be assigned to an interface variable through which the interface's 
// functions can be called.
//
// Interfaces are a feature of the Java programming language [Gosling] and the
// most obvious way to model interfaces in C++ is (multiple) inheritance.
// Using inheritance for this purpose, however, is neither the most efficient 
// nor the most flexible solution, because:
//
//   - all functions must be virtual,
//
//     => a function that calls another function of the interface must do so
//        via virtual dispatch (as opposed to inlining)
//     => a class can not implement an interface's (overloaded) function via
//        a function template
//
//   - inhertitance is intrusive
//
//     => object size increases 
//     => client's are always polymorphic
//     => dependencies cause tighter coupling
//
// Fortunately it is possible to eliminate all the drawbacks mentioned above
// based on an alternative implementation proposed by David Abrahams. 
// We'll add some detail to the original scheme (see [Abrahams]) such as 
// support for overloaded and const qualified functions.
// The implementation in this example uses Boost.FunctionTypes to shift 
// metaprogramming code from the preprocessor into templates, to reduce 
// preprocessing time and increase maintainability.
//
// 
// Limitations
// ===========
//
// There is no lifetime management as implemented by the Boost candidate
// Interfaces library (see [Turkanis]).
//
// This example does not compile with Visual C++. Template argument deduction
// from the result of the address-of operator does not work properly with this
// compiler. It is possible to partially work around the problem, but it isn't
// done here for the sake of readability.
//
//
// Bibliography
// ============
//
// [Gosling]  Gosling, J., Joy, B., Steele, G. The Java Language Specification
//   http://java.sun.com/docs/books/jls/third_edition/html/interfaces.html
//
// [Abrahams] Abrahams, D. Proposal: interfaces, Post to newsgroup comp.std.c++
//   http://groups.google.com/group/comp.std.c++/msg/85af30a61bf677e4
//
// [Turkanis] Turkanis, J., Diggins, C. Boost candidate Interfaces library
//   http://www.kangaroologic.com/interfaces/libs/interfaces/doc/index.html

#include <cstddef>

#include <boost/function_types/function_pointer.hpp>
#include <boost/function_types/member_function_pointer.hpp>

#include <boost/config.hpp>
#include <boost/detail/workaround.hpp>

#include <boost/utility/addressof.hpp>

#include <boost/mpl/at.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/single_view.hpp>
#include <boost/mpl/transform_view.hpp>

#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>

#include "detail/param_type.hpp"

namespace example 
{
  namespace ft = boost::function_types;
  namespace mpl = boost::mpl;
  using namespace mpl::placeholders;

  // join a single type and an MPL-sequence
  // in some ways similar to mpl::push_front (but mpl::push_front requires
  // an MPL Extensible Sequence and this template does not)
  template<typename T, typename Seq>
  struct concat_view
    : mpl::joint_view<mpl::single_view<T>, Seq>
  { };

  // metafunction returning a function pointer type for a vtable entry
  template<typename Inf>
  struct vtable_entry
    : ft::function_pointer
      < concat_view< typename Inf::result, mpl::transform_view< 
            typename Inf::params, param_type<_> > > >
  { };

  // the expression '& member<MetaInfo,Tag>::wrap<& Class::Function> ' in an
  // assignment context binds the member function Function of Class with the
  // properties described by MetaInfo and Tag to the corresponding vtable 
  // entry
  template<typename Inf, typename Tag>
  struct member
  {
    typedef typename ft::member_function_pointer 
      < concat_view<typename Inf::result,typename Inf::params>,Tag
      >::type
    mem_func_ptr;

    typedef typename mpl::at_c<typename Inf::params,0>::type context;

    template<mem_func_ptr MemFuncPtr>
    static typename Inf::result wrap(void* c)
    {
      return (reinterpret_cast<context*>(c)->*MemFuncPtr)();
    }
    template<mem_func_ptr MemFuncPtr, typename T0>
    static typename Inf::result wrap(void* c, T0 a0)
    {
      return (reinterpret_cast<context*>(c)->*MemFuncPtr)(a0);
    }
    template<mem_func_ptr MemFuncPtr, typename T0, typename T1>
    static typename Inf::result wrap(void* c, T0 a0, T1 a1)
    {
      return (reinterpret_cast<context*>(c)->*MemFuncPtr)(a0,a1);
    }
    // continue with the preprocessor (the scheme should be clear, by now)
    #define  BOOST_PP_LOCAL_MACRO(n)                                           \
    template<mem_func_ptr MemFuncPtr, BOOST_PP_ENUM_PARAMS(n,typename T)>      \
    static typename Inf::result wrap(void* c,                                  \
        BOOST_PP_ENUM_BINARY_PARAMS(n,T,a))                                    \
    {                                                                          \
      return (reinterpret_cast<context*>(c)->*MemFuncPtr)(                     \
        BOOST_PP_ENUM_PARAMS(n,a) );                                           \
    }
    #define  BOOST_PP_LOCAL_LIMITS (3,BOOST_FT_MAX_ARITY-1)
    #include BOOST_PP_LOCAL_ITERATE()
  };

  // extract a parameter by index
  template<typename Inf, std::size_t Index>
  struct param
    : param_type< typename mpl::at_c< typename Inf::params,Index>::type >
  { };
}

// the interface definition on the client's side
#define BOOST_EXAMPLE_INTERFACE(name,def)                                      \
    class name                                                                 \
    {                                                                          \
      struct vtable                                                            \
      {                                                                        \
        BOOST_EXAMPLE_INTERFACE__MEMBERS(def,VTABLE)                           \
      };                                                                       \
                                                                               \
      vtable const * ptr_vtable;                                               \
      void * ptr_that;                                                         \
                                                                               \
      template<class T> struct vtable_holder                                   \
      {                                                                        \
        static vtable const val_vtable;                                        \
      };                                                                       \
                                                                               \
    public:                                                                    \
                                                                               \
      template<class T>                                                        \
      inline name (T & that)                                                   \
        : ptr_vtable(& vtable_holder<T>::val_vtable)                           \
        , ptr_that(boost::addressof(that))                                     \
      { }                                                                      \
                                                                               \
      BOOST_EXAMPLE_INTERFACE__MEMBERS(def,FUNCTION)                           \
    };                                                                         \
                                                                               \
    template<typename T>                                                       \
    name ::vtable const name ::vtable_holder<T>::val_vtable                    \
        = { BOOST_EXAMPLE_INTERFACE__MEMBERS(def,INIT_VTABLE) }


#ifdef BOOST_PP_NIL // never defined -- a comment with syntax highlighting

BOOST_EXAMPLE_INTERFACE( interface_x,
  (( a_func, (void)(int), const_qualified ))
  (( another_func, (int), non_const   )) 
);

// expands to:
class interface_x 
{ 
  struct vtable 
  {
    // meta information for first function 
    template<typename T = void*> struct inf0 
    {
      typedef void result; 
      typedef ::boost::mpl::vector< T, int > params; 
    };
    // function pointer with void* context pointer and parameters optimized
    // for forwarding 
    ::example::vtable_entry<inf0<> >::type func0; 

    // second function
    template<typename T = void*> struct inf1 
    {
      typedef int result; 
      typedef ::boost::mpl::vector< T > params; 
    }; 
    ::example::vtable_entry<inf1<> >::type func1; 
  }; 

  // data members
  vtable const * ptr_vtable; 
  void * ptr_that; 

  // this template is instantiated for every class T this interface is created
  // from, causing the compiler to emit an initialized vtable for this type 
  // (see aggregate assignment, below)
  template<class T> struct vtable_holder 
  {
    static vtable const val_vtable; 
  }; 

public: 

  // converting ctor, creates an interface from an arbitrary class
  template<class T> 
  inline interface_x (T & that) 
    : ptr_vtable(& vtable_holder<T>::val_vtable)
    , ptr_that(boost::addressof(that)) 
  { } 

  // the member functions from the interface definition, parameters are 
  // optimized for forwarding

  inline vtable::inf0<> ::result a_func (
      ::example::param<vtable::inf0<>,1>::type p0) const 
  { 
    return ptr_vtable-> func0(ptr_that , p0); 
  } 

  inline vtable::inf1<> ::result another_func () 
  {
    return ptr_vtable-> func1(ptr_that ); 
  } 
}; 

template<typename T> 
interface_x ::vtable const interface_x ::vtable_holder<T>::val_vtable = 
{
  // instantiate function templates that wrap member function pointers (which
  // are known at compile time) by taking their addresses in assignment to
  // function pointer context
  & ::example::member< vtable::inf0<T>, ::example::ft:: const_qualified >
        ::template wrap < &T:: a_func > 
, & ::example::member< vtable::inf1<T>, ::example::ft:: non_const >
        ::template wrap < &T:: another_func > 
};
#endif

// preprocessing code details

// iterate all of the interface's members and invoke a macro (prefixed with
// BOOST_EXAMPLE_INTERFACE_)
#define BOOST_EXAMPLE_INTERFACE__MEMBERS(seq,macro)                            \
    BOOST_PP_REPEAT(BOOST_PP_SEQ_SIZE(seq),                                    \
        BOOST_EXAMPLE_INTERFACE__ ## macro,seq)

// extract signature sequence from entry 
#define BOOST_EXAMPLE_INTERFACE__VTABLE(z,i,seq)                               \
    BOOST_EXAMPLE_INTERFACE__VTABLE_I(z,i,                                     \
        BOOST_PP_TUPLE_ELEM(3,1,BOOST_PP_SEQ_ELEM(i,seq)))

// split the signature sequence result/params and insert T at the beginning of
// the params part
#define BOOST_EXAMPLE_INTERFACE__VTABLE_I(z,i,seq)                             \
    BOOST_EXAMPLE_INTERFACE__VTABLE_II(z,i,                                    \
        BOOST_PP_SEQ_HEAD(seq),(T)BOOST_PP_SEQ_TAIL(seq))

// emit the meta information structure and function pointer declaration
#define BOOST_EXAMPLE_INTERFACE__VTABLE_II(z,i,result_type,param_types)        \
    template<typename T = void*>                                               \
    struct BOOST_PP_CAT(inf,i)                                                 \
    {                                                                          \
      typedef result_type result;                                              \
      typedef ::boost::mpl::vector< BOOST_PP_SEQ_ENUM(param_types) > params;   \
    };                                                                         \
    ::example::vtable_entry<BOOST_PP_CAT(inf,i)<> >::type BOOST_PP_CAT(func,i);

// extract tuple entry from sequence and precalculate the name of the function
// pointer variable
#define BOOST_EXAMPLE_INTERFACE__INIT_VTABLE(z,i,seq)                          \
    BOOST_EXAMPLE_INTERFACE__INIT_VTABLE_I(i,seq,BOOST_PP_CAT(func,i),         \
        BOOST_PP_SEQ_ELEM(i,seq))

// emit a function pointer expression that encapsulates the corresponding
// member function of T
#define BOOST_EXAMPLE_INTERFACE__INIT_VTABLE_I(i,seq,func,desc)                \
    BOOST_PP_COMMA_IF(i) & ::example::member< BOOST_PP_CAT(vtable::inf,i)<T>,  \
        ::example::ft:: BOOST_PP_TUPLE_ELEM(3,2,desc) >::template wrap         \
                                          < &T:: BOOST_PP_TUPLE_ELEM(3,0,desc) >
        
// extract tuple entry from sequence
#define BOOST_EXAMPLE_INTERFACE__FUNCTION(z,i,seq)                             \
    BOOST_EXAMPLE_INTERFACE__FUNCTION_I(z,i,BOOST_PP_SEQ_ELEM(i,seq))

// precalculate function name, arity, name of meta info structure and cv-
// qualifiers
#define BOOST_EXAMPLE_INTERFACE__FUNCTION_I(z,i,desc)                          \
    BOOST_EXAMPLE_INTERFACE__FUNCTION_II(z,i,                                  \
        BOOST_PP_TUPLE_ELEM(3,0,desc),                                         \
        BOOST_PP_DEC(BOOST_PP_SEQ_SIZE(BOOST_PP_TUPLE_ELEM(3,1,desc))),        \
        BOOST_PP_CAT(vtable::inf,i)<>,                                         \
        BOOST_PP_CAT(BOOST_EXAMPLE_INTERFACE___,BOOST_PP_TUPLE_ELEM(3,2,desc)) \
    )

// emit the definition for a member function of the interface
#define BOOST_EXAMPLE_INTERFACE__FUNCTION_II(z,i,name,arity,types,cv)          \
    inline types ::result name                                                 \
      (BOOST_PP_ENUM_ ## z (arity,BOOST_EXAMPLE_INTERFACE__PARAM,types)) cv()  \
    {                                                                          \
      return ptr_vtable-> BOOST_PP_CAT(func,i)(ptr_that                        \
          BOOST_PP_ENUM_TRAILING_PARAMS_Z(z,arity,p));                         \
    }

// emit a parameter of the function definition
#define BOOST_EXAMPLE_INTERFACE__PARAM(z,j,types)                              \
    ::example::param<types,BOOST_PP_INC(j)>::type BOOST_PP_CAT(p,j)

// helper macros to map 'const_qualified' to 'const' an 'non_const' to ''
#define BOOST_EXAMPLE_INTERFACE___const_qualified BOOST_PP_IDENTITY(const)
#define BOOST_EXAMPLE_INTERFACE___non_const BOOST_PP_EMPTY


