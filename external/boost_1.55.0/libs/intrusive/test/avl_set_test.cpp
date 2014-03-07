/////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Olaf Krzikalla 2004-2006.
// (C) Copyright Ion Gaztanaga  2006-2013.
//
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/intrusive for documentation.
//
/////////////////////////////////////////////////////////////////////////////
#include <boost/intrusive/detail/config_begin.hpp>
#include <boost/intrusive/avl_set.hpp>
#include "itestvalue.hpp"
#include "smart_ptr.hpp"
#include "generic_set_test.hpp"

namespace boost { namespace intrusive { namespace test {

#if !defined (BOOST_INTRUSIVE_VARIADIC_TEMPLATES)
template<class T, class O1, class O2, class O3, class O4>
#else
template<class T, class ...Options>
#endif
struct has_insert_before<boost::intrusive::avl_set<T,
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
   typedef avl_set_base_hook<void_pointer<VoidPointer> >    base_hook_type;
   typedef avl_set_base_hook
      <link_mode<auto_unlink>
      , void_pointer<VoidPointer>
      , tag<my_tag>
      , optimize_size<true> >                               auto_base_hook_type;
   typedef avl_set_member_hook
      <void_pointer<VoidPointer> >                          member_hook_type;
   typedef avl_set_member_hook
      < link_mode<auto_unlink>
      , void_pointer<VoidPointer> >                         auto_member_hook_type;
};

template< class ValueType
        , class Option1 =void
        , class Option2 =void
        , class Option3 =void
        >
struct GetContainer
{
   typedef boost::intrusive::avl_set
      < ValueType
      , Option1
      , Option2
      , Option3
      > type;
};

template<class VoidPointer, bool constant_time_size>
class test_main_template
{
   public:
   int operator()()
   {
      using namespace boost::intrusive;
      typedef testvalue<hooks<VoidPointer> , constant_time_size> value_type;

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
      return 0;
   }
};

template<class VoidPointer>
class test_main_template<VoidPointer, false>
{
   public:
   int operator()()
   {
      using namespace boost::intrusive;
      typedef testvalue<hooks<VoidPointer> , false> value_type;

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
                  , typename hooks<VoidPointer>::auto_base_hook_type
                  >::type
                , GetContainer
                >::test_all();

      test::test_generic_set < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::auto_member_hook_type
                               , &value_type::auto_node_
                               >
                  >::type
                , GetContainer
                >::test_all();

      return 0;
   }
};

int main( int, char* [] )
{
   test_main_template<void*, false>()();
   test_main_template<boost::intrusive::smart_ptr<void>, false>()();
   test_main_template<void*, true>()();
   test_main_template<boost::intrusive::smart_ptr<void>, true>()();

   return boost::report_errors();
}

#include <boost/intrusive/detail/config_end.hpp>
