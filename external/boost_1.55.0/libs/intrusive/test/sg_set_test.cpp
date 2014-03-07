/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga  2007-2013
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/sg_set.hpp>
#include "itestvalue.hpp"
#include "smart_ptr.hpp"
#include "generic_set_test.hpp"

namespace boost { namespace intrusive { namespace test {

#if !defined (BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class O1, class O2, class O3, class O4>
#else
template<class T, class ...Options>
#endif
struct has_rebalance<boost::intrusive::sg_set<T,
   #if !defined (BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
   O1, O2, O3, O4
   #else
   Options...
   #endif
> >
{
   static const bool value = true;
};

#if !defined (BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class O1, class O2, class O3, class O4>
#else
template<class T, class ...Options>
#endif
struct has_insert_before<boost::intrusive::sg_set<T,
   #if !defined (BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
   O1, O2, O3, O4
   #else
   Options...
   #endif
> >
{
   static const bool value = true;
};

}}}


using namespace boost::intrusive;

struct my_tag;

template<class VoidPointer>
struct hooks
{
   typedef bs_set_base_hook<void_pointer<VoidPointer> >     base_hook_type;
   typedef bs_set_member_hook<void_pointer<VoidPointer> >   member_hook_type;
   typedef member_hook_type   auto_member_hook_type;
   struct auto_base_hook_type
      :  bs_set_base_hook<void_pointer<VoidPointer>, tag<my_tag> >
   {};
};

template< class ValueType
        , class Option1 =void
        , class Option2 =void
        , class Option3 =void
        >
struct GetContainer
{
   typedef boost::intrusive::sg_set
      < ValueType
      , Option1
      , Option2
      , Option3
      > type;
};

template< class ValueType
        , class Option1 =void
        , class Option2 =void
        , class Option3 =void
        >
struct GetContainerFixedAlpha
{
   typedef boost::intrusive::sg_set
      < ValueType
      , Option1
      , Option2
      , Option3
      , boost::intrusive::floating_point<false>
      > type;
};

template<class VoidPointer>
class test_main_template
{
   public:
   int operator()()
   {
      using namespace boost::intrusive;
      typedef testvalue<hooks<VoidPointer> , true> value_type;

      test::test_generic_set < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                , GetContainer
                >::test_all();
      test::test_generic_set < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                , GetContainer
                >::test_all();

      test::test_generic_set < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                , GetContainerFixedAlpha
                >::test_all();
      test::test_generic_set < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                , GetContainerFixedAlpha
                >::test_all();
      return 0;
   }
};

int main( int, char* [] )
{
   test_main_template<void*>()();
   test_main_template<boost::intrusive::smart_ptr<void> >()();
   return boost::report_errors();
}
#include <boost/intrusive/detail/config_end.hpp>
