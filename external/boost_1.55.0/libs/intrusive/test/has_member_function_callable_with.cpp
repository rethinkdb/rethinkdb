//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2011-2013. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/detail/workaround.hpp>
//Just for BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED
#include <boost/intrusive/detail/has_member_function_callable_with.hpp>
#include <cstddef>
#include <boost/move/move.hpp>

namespace boost{
namespace intrusive{
namespace intrusive_detail{
namespace has_member_function_callable_with {

struct dont_care
{
   dont_care(...);
};

struct private_type
{
   static private_type p;
   private_type const &operator,(int) const;
};

typedef char yes_type;
struct no_type{ char dummy[2]; };

template<typename T>
no_type is_private_type(T const &);
yes_type is_private_type(private_type const &);

}}}}


namespace boost{
namespace intrusive{
namespace intrusive_detail{

template <typename Type>
class has_member_function_named_func
{
   struct BaseMixin
   {
      void func();
   };

   struct Base : public ::boost::intrusive::detail::remove_cv<Type>::type, public BaseMixin {};
   template <typename T, T t> class Helper{};

   template <typename U>
   static has_member_function_callable_with::no_type  deduce
      (U*, Helper<void (BaseMixin::*)(), &U::func>* = 0);
   static has_member_function_callable_with::yes_type deduce(...);

   public:
   static const bool value =
      sizeof(has_member_function_callable_with::yes_type) == sizeof(deduce((Base*)(0)));
};

}}}

#if !defined(BOOST_CONTAINER_PERFECT_FORWARDING)

   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{

   template<typename Fun, bool HasFunc
            , class P0 = void , class P1 = void , class P2 = void>
   struct has_member_function_callable_with_func_impl;


   template<typename Fun , class P0 , class P1 , class P2>
   struct has_member_function_callable_with_func_impl
      <Fun, false , P0 , P1 , P2>
   {
      static const bool value = false;
   };


   }}}


   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{

      #if !defined(_MSC_VER) || (_MSC_VER < 1600)

         #if !defined(BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED)

         template<class F, std::size_t N = sizeof(boost::move_detail::declval<F>().func(), 0)>
         struct zeroarg_checker_func
         {
            has_member_function_callable_with::yes_type dummy;
            zeroarg_checker_func(int);
         };

         //For buggy compilers like MSVC 7.1, ((F*)0)->func() does not
         //fail but sizeof yields to 0.
         template<class F>
         struct zeroarg_checker_func<F, 0>
         {
            has_member_function_callable_with::no_type dummy;
            zeroarg_checker_func(int);
         };

         template<typename Fun>
         struct has_member_function_callable_with_func_impl
            <Fun, true , void , void , void>
         {
            template<class U>
            static zeroarg_checker_func<U> Test(zeroarg_checker_func<U>*);

            template <class U>
            static has_member_function_callable_with::no_type Test(...);

            static const bool value
               = sizeof(Test< Fun >(0)) == sizeof(has_member_function_callable_with::yes_type);
         };

         #else

         template<typename Fun>
         struct has_member_function_callable_with_func_impl
            <Fun, true , void , void , void>
         {
            //GCC [3.4-4.3) gives ICE when instantiating the 0 arg version so it is not supported.
            static const bool value = true;
         };

         #endif

      #else

         template<typename Fun>
         struct has_member_function_callable_with_func_impl
            <Fun, true , void , void , void>
         {
            template<class U>
            static decltype(boost::move_detail::declval<Fun>().func(), has_member_function_callable_with::yes_type()) Test(Fun* f);

            template<class U>
            static has_member_function_callable_with::no_type Test(...);
            static const bool value = sizeof(Test<Fun>((Fun*)0)) == sizeof(has_member_function_callable_with::yes_type);
         };

      #endif

   }}}


   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{


   template<typename Fun>
   struct funwrap1_func : Fun
   {
      using Fun::func;
      has_member_function_callable_with::private_type
         func( has_member_function_callable_with::dont_care) const;
   };

   template<typename Fun , class P0>
   struct has_member_function_callable_with_func_impl
      <Fun, true , P0 , void , void>
   {

      typedef funwrap1_func<Fun> FunWrap;

      static bool const value = (sizeof(has_member_function_callable_with::no_type) ==
                                 sizeof(has_member_function_callable_with::is_private_type
                                          ( (::boost::move_detail::declval< FunWrap >().func( ::boost::move_detail::declval<P0>() ), 0) )
                                       )
                                 );
   };

   }}}

   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{


   template<typename Fun>
   struct funwrap2_func: Fun
   {
      using Fun::func;
      has_member_function_callable_with::private_type
         func( has_member_function_callable_with::dont_care , has_member_function_callable_with::dont_care)  const;
   };

   template<typename Fun , class P0 , class P1>
   struct has_member_function_callable_with_func_impl
      <Fun, true , P0 , P1 , void>
   {
      typedef funwrap2_func<Fun> FunWrap;

      static bool const value = (sizeof(has_member_function_callable_with::no_type) ==
                                 sizeof(has_member_function_callable_with::is_private_type
                                          ( (::boost::move_detail::declval< FunWrap >().
                                                func( ::boost::move_detail::declval<P0>()
                                                   , ::boost::move_detail::declval<P1>() )
                                             , 0) )
                                       )
                                 );
   };
   }}}

   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{


   template<typename Fun>
   struct funwrap3_func: Fun
   {
      using Fun::func;
      has_member_function_callable_with::private_type
         func( has_member_function_callable_with::dont_care
            , has_member_function_callable_with::dont_care
            , has_member_function_callable_with::dont_care)  const;
   };

   template<typename Fun , class P0 , class P1 , class P2>
   struct has_member_function_callable_with_func_impl
      <Fun, true , P0 , P1 , P2 >
   {
      typedef funwrap3_func<Fun> FunWrap;

      static bool const value = (sizeof(has_member_function_callable_with::no_type) ==
                                 sizeof(has_member_function_callable_with::is_private_type
                                          ( (::boost::move_detail::declval< FunWrap >().
                                             func( ::boost::move_detail::declval<P0>()
                                                , ::boost::move_detail::declval<P1>()
                                                , ::boost::move_detail::declval<P2>() )
                                                , 0) )
                                       )
                                 );
   };

   template<typename Fun
            , class P0 = void , class P1 = void, typename P2 = void>
   struct has_member_function_callable_with_func
      : public has_member_function_callable_with_func_impl
         <Fun, has_member_function_named_func<Fun>::value, P0 , P1 , P2 >
   {};

   }}}

#else

   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{

   template<typename Fun, bool HasFunc, class ...Args>
   struct has_member_function_callable_with_func_impl;

   template<typename Fun, class ...Args>
   struct has_member_function_callable_with_func_impl
      <Fun, false, Args...>
   {
      static const bool value = false;
   };


   }}}


   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{

   template<class F, std::size_t N = sizeof(boost::move_detail::declval<F>().func(), 0)>
   struct zeroarg_checker_func
   {
      has_member_function_callable_with::yes_type dummy;
      zeroarg_checker_func(int);
   };

   //For buggy compilers like MSVC 7.1, ((F*)0)->func() does not
   //fail but sizeof yields to 0.
   template<class F>
   struct zeroarg_checker_func<F, 0>
   {
      has_member_function_callable_with::no_type dummy;
      zeroarg_checker_func(int);
   };

   template<typename Fun>
   struct has_member_function_callable_with_func_impl
      <Fun, true>
   {
      template<class U>
      static zeroarg_checker_func<U> Test(zeroarg_checker_func<U>*);

      template <class U>
      static has_member_function_callable_with::no_type Test(...);

      static const bool value = sizeof(Test< Fun >(0))
                           == sizeof(has_member_function_callable_with::yes_type);
   };

   }}}


   namespace boost{
   namespace intrusive{
   namespace intrusive_detail{


   template<typename Fun, class ...DontCares>
   struct funwrap_func : Fun
   {
      using Fun::func;
      has_member_function_callable_with::private_type
         func(DontCares...) const;
   };

   template<typename Fun, class ...Args>
   struct has_member_function_callable_with_func_impl
      <Fun, true , Args...>
   {
      template<class T>
      struct make_dontcare
      {
         typedef has_member_function_callable_with::dont_care type;
      };

      typedef funwrap_func<Fun, typename make_dontcare<Args>::type...> FunWrap;

      static bool const value = (sizeof(has_member_function_callable_with::no_type) ==
                                 sizeof(has_member_function_callable_with::is_private_type
                                          ( (::boost::move_detail::declval< FunWrap >().func( ::boost::move_detail::declval<Args>()... ), 0) )
                                       )
                                 );
   };

   template<typename Fun, class ...Args>
   struct has_member_function_callable_with_func
      : public has_member_function_callable_with_func_impl
         <Fun, has_member_function_named_func<Fun>::value, Args... >
   {};

   }}}

#endif

struct functor
{
   void func();
   void func(const int&);
   void func(const int&, const int&);
   void func(const int&, const int&, const int&);
};

struct functor2
{
   void func(char*);
   void func(int, char*);
   void func(int, char*, double);
   void func(const int&, char*, void *);
};

struct functor3
{

};

struct functor4
{
   void func(...);
   template<class T>
   void func(int, const T &);

   template<class T>
   void func(const T &);

   template<class T, class U>
   void func(int, const T &, const U &);
};

int main()
{
   using namespace boost::intrusive::intrusive_detail;

   #if !defined(BOOST_INTRUSIVE_DETAIL_HAS_MEMBER_FUNCTION_CALLABLE_WITH_0_ARGS_UNSUPPORTED)
   {
   int check1[ has_member_function_callable_with_func<functor>::value  ? 1 : -1];
   int check2[!has_member_function_callable_with_func<functor2>::value ? 1 : -1];
   int check3[!has_member_function_callable_with_func<functor3>::value ? 1 : -1];
   int check4[ has_member_function_callable_with_func<functor4>::value ? 1 : -1];
   (void)check1;
   (void)check2;
   (void)check3;
   (void)check4;
   }
   #endif

   {
   int check1[ has_member_function_callable_with_func<functor,  int>::value   ? 1 : -1];
   int check2[!has_member_function_callable_with_func<functor,  char*>::value ? 1 : -1];
   int check3[!has_member_function_callable_with_func<functor2, int>::value   ? 1 : -1];
   int check4[ has_member_function_callable_with_func<functor2, char*>::value ? 1 : -1];
   int check5[!has_member_function_callable_with_func<functor3, int>::value   ? 1 : -1];
   int check6[!has_member_function_callable_with_func<functor3, char*>::value ? 1 : -1];
   int check7[ has_member_function_callable_with_func<functor4, int>::value ? 1 : -1];
   int check8[ has_member_function_callable_with_func<functor4, char*>::value ? 1 : -1];
   (void)check1;
   (void)check2;
   (void)check3;
   (void)check4;
   (void)check5;
   (void)check6;
   (void)check7;
   (void)check8;
   }

   {
   int check1[ has_member_function_callable_with_func<functor,  int, int>::value   ? 1 : -1];
   int check2[!has_member_function_callable_with_func<functor,  int, char*>::value ? 1 : -1];
   int check3[!has_member_function_callable_with_func<functor2, int, int>::value   ? 1 : -1];
   int check4[ has_member_function_callable_with_func<functor2, int, char*>::value ? 1 : -1];
   int check5[!has_member_function_callable_with_func<functor3, int, int>::value   ? 1 : -1];
   int check6[!has_member_function_callable_with_func<functor3, int, char*>::value ? 1 : -1];
   int check7[ has_member_function_callable_with_func<functor4, int, char*>::value ? 1 : -1];
   (void)check1;
   (void)check2;
   (void)check3;
   (void)check4;
   (void)check5;
   (void)check6;
   (void)check7;
   }

   {
    int check1[ has_member_function_callable_with_func<functor,  int, int, int>::value   ? 1 : -1];
    int check2[!has_member_function_callable_with_func<functor,  int, char*, int>::value ? 1 : -1];
    int check3[!has_member_function_callable_with_func<functor2, int, int, int>::value   ? 1 : -1];
    int check4[ has_member_function_callable_with_func<functor2, int, char*, void*>::value ? 1 : -1];
    int check5[!has_member_function_callable_with_func<functor3, int, int, int>::value   ? 1 : -1];
    int check6[!has_member_function_callable_with_func<functor3, int, char*, void*>::value ? 1 : -1];
    int check7[ has_member_function_callable_with_func<functor4, int, char*, int>::value ? 1 : -1];
   (void)check1;
   (void)check2;
   (void)check3;
   (void)check4;
   (void)check5;
   (void)check6;
   (void)check7;
   }

   return 0;

}
#include <boost/intrusive/detail/config_end.hpp>
