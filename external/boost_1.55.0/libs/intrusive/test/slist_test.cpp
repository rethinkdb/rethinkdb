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
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include "itestvalue.hpp"
#include "smart_ptr.hpp"
#include "common_functors.hpp"
#include <vector>
#include <boost/detail/lightweight_test.hpp>
#include "test_macros.hpp"
#include "test_container.hpp"

using namespace boost::intrusive;

struct my_tag;

template<class VoidPointer>
struct hooks
{
   typedef slist_base_hook<void_pointer<VoidPointer> >                base_hook_type;
   typedef slist_base_hook< link_mode<auto_unlink>
                         , void_pointer<VoidPointer>, tag<my_tag> >  auto_base_hook_type;
   typedef slist_member_hook<void_pointer<VoidPointer>, tag<my_tag> > member_hook_type;
   typedef slist_member_hook< link_mode<auto_unlink>
                           , void_pointer<VoidPointer> >             auto_member_hook_type;
};

template<class ValueTraits, bool Linear, bool CacheLast>
struct test_slist
{
   typedef typename ValueTraits::value_type value_type;
   static void test_all(std::vector<value_type>& values);
   static void test_front(std::vector<value_type>& values);
   static void test_back(std::vector<value_type>& values, detail::bool_<true>);
   static void test_back(std::vector<value_type>& values, detail::bool_<false>);
   static void test_sort(std::vector<value_type>& values);
   static void test_merge(std::vector<value_type>& values);
   static void test_remove_unique(std::vector<value_type>& values);
   static void test_insert(std::vector<value_type>& values);
   static void test_shift(std::vector<value_type>& values);
   static void test_swap(std::vector<value_type>& values);
   static void test_slow_insert(std::vector<value_type>& values);
   static void test_clone(std::vector<value_type>& values);
   static void test_container_from_end(std::vector<value_type> &, detail::bool_<true>){}
   static void test_container_from_end(std::vector<value_type> &values, detail::bool_<false>);
};

template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_all (std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   {
      list_type list(values.begin(), values.end());
      test::test_container(list);
      list.clear();
      list.insert(list.end(), values.begin(), values.end());
      test::test_sequence_container(list, values);
   }
   test_front(values);
   test_back(values, detail::bool_<CacheLast>());
   test_sort(values);
   test_merge (values);
   test_remove_unique(values);
   test_insert(values);
   test_shift(values);
   test_slow_insert (values);
   test_swap(values);
   test_clone(values);
   test_container_from_end(values, detail::bool_<Linear>());
}

//test: push_front, pop_front, front, size, empty:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_front(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist;
   BOOST_TEST (testlist.empty());

   testlist.push_front (values[0]);
   BOOST_TEST (testlist.size() == 1);
   BOOST_TEST (&testlist.front() == &values[0]);

   testlist.push_front (values[1]);
   BOOST_TEST (testlist.size() == 2);
   BOOST_TEST (&testlist.front() == &values[1]);

   testlist.pop_front();
   BOOST_TEST (testlist.size() == 1);
   BOOST_TEST (&testlist.front() == &values[0]);

   testlist.pop_front();
   BOOST_TEST (testlist.empty());
}

//test: push_front, pop_front, front, size, empty:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_back(std::vector<typename ValueTraits::value_type>& values, detail::bool_<true>)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist;
   BOOST_TEST (testlist.empty());

   testlist.push_back (values[0]);
   BOOST_TEST (testlist.size() == 1);
   BOOST_TEST (&testlist.front() == &values[0]);
   BOOST_TEST (&testlist.back() == &values[0]);
   testlist.push_back(values[1]);
   BOOST_TEST(*testlist.previous(testlist.end()) == values[1]);
   BOOST_TEST (&testlist.front() == &values[0]);
   BOOST_TEST (&testlist.back() == &values[1]);
}

//test: push_front, pop_front, front, size, empty:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_back(std::vector<typename ValueTraits::value_type>&, detail::bool_<false>)
{}


//test: merge due to error in merge implementation:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_merge (std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist1, testlist2;
   testlist1.push_front (values[0]);
   testlist2.push_front (values[4]);
   testlist2.push_front (values[3]);
   testlist2.push_front (values[2]);
   testlist1.merge (testlist2);

   int init_values [] = { 1, 3, 4, 5 };
   TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );
}

//test: merge due to error in merge implementation:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_remove_unique (std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   {
      list_type list(values.begin(), values.end());
      list.remove_if(is_even());
      int init_values [] = { 1, 3, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
   }
   {
      std::vector<typename ValueTraits::value_type> values2(values);
      list_type list(values.begin(), values.end());
      list.insert_after(list.before_begin(), values2.begin(), values2.end());
      list.sort();
      int init_values [] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
      list.unique();
      int init_values2 [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values2, list.begin() );
   }
}

//test: constructor, iterator, sort, reverse:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_sort(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist (values.begin(), values.end());

   {  int init_values [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }

   testlist.sort (even_odd());
   {  int init_values [] = { 2, 4, 1, 3, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }

   testlist.reverse();
   {  int init_values [] = { 5, 3, 1, 4, 2 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
}

//test: assign, insert_after, const_iterator, erase_after, s_iterator_to, previous:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_insert(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist;
   testlist.assign (&values[0] + 2, &values[0] + 5);

   const list_type& const_testlist = testlist;
   {  int init_values [] = { 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   typename list_type::iterator i = ++testlist.begin();
   BOOST_TEST (i->value_ == 4);

   testlist.insert_after (i, values[0]);
   {  int init_values [] = { 3, 4, 1, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   i = testlist.iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);
   i = list_type::s_iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);
   i = testlist.previous (i);
   BOOST_TEST (&*i == &values[0]);

   testlist.erase_after (i);
   BOOST_TEST (&*i == &values[0]);
   {  int init_values [] = { 3, 4, 1 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }
}

//test: insert, const_iterator, erase, siterator_to:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_slow_insert (std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist;
   testlist.push_front (values[4]);
   testlist.insert (testlist.begin(), &values[0] + 2, &values[0] + 4);

   const list_type& const_testlist = testlist;
   {  int init_values [] = { 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   typename list_type::iterator i = ++testlist.begin();
   BOOST_TEST (i->value_ == 4);

   testlist.insert (i, values[0]);
   {  int init_values [] = { 3, 1, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   i = testlist.iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);

   i = list_type::s_iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);

   i = testlist.erase (i);
   BOOST_TEST (i == testlist.end());

   {  int init_values [] = { 3, 1, 4 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   testlist.erase (++testlist.begin(), testlist.end());
   BOOST_TEST (testlist.size() == 1);
   BOOST_TEST (testlist.front().value_ == 3);
}

template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_shift(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist;

   const int num_values = (int)values.size();
   std::vector<int> expected_values(num_values);

   //Shift forward all possible positions 3 times
   for(int s = 1; s <= num_values; ++s){
      expected_values.resize(s);
      for(int i = 0; i < s*3; ++i){
         testlist.insert_after(testlist.before_begin(), &values[0], &values[0] + s);
         testlist.shift_forward(i);
         for(int j = 0; j < s; ++j){
            expected_values[(j + s - i%s) % s] = (j + 1);
         }

         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin())
         testlist.clear();
      }

      //Shift backwards all possible positions
      for(int i = 0; i < s*3; ++i){
         testlist.insert_after(testlist.before_begin(), &values[0], &values[0] + s);
         testlist.shift_backwards(i);
         for(int j = 0; j < s; ++j){
            expected_values[(j + i) % s] = (j + 1);
         }

         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin())
         testlist.clear();
      }
   }
}

//test: insert_after (seq-version), swap, splice_after:
template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_swap(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   {
      list_type testlist1 (&values[0], &values[0] + 2);
      list_type testlist2;
      testlist2.insert_after (testlist2.before_begin(), &values[0] + 2, &values[0] + 5);
      testlist1.swap(testlist2);
      {  int init_values [] = { 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
         testlist2.splice_after (testlist2.begin(), testlist1);
      {  int init_values [] = { 1, 3, 4, 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
      BOOST_TEST (testlist1.empty());

      testlist1.splice_after (testlist1.before_begin(), testlist2, ++testlist2.begin());
      {  int init_values [] = { 4 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 1, 3, 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      testlist1.splice_after (testlist1.begin(), testlist2,
                              testlist2.before_begin(), ++++testlist2.begin());
      {  int init_values [] = { 4, 1, 3, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
   }
   {  //Now test swap when testlist2 is empty
      list_type testlist1 (&values[0], &values[0] + 2);
      list_type testlist2;
      testlist1.swap(testlist2);
      BOOST_TEST (testlist1.empty());
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
   }
   {  //Now test swap when testlist1 is empty
      list_type testlist2 (&values[0], &values[0] + 2);
      list_type testlist1;
      testlist1.swap(testlist2);
      BOOST_TEST (testlist2.empty());
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
   }
   {  //Now test when both are empty
      list_type testlist1, testlist2;
      testlist2.swap(testlist1);
      BOOST_TEST (testlist1.empty() && testlist2.empty());
   }

   if(!list_type::linear)
   {
      list_type testlist1 (&values[0], &values[0] + 2);
      list_type testlist2 (&values[0] + 3, &values[0] + 5);

      values[0].swap_nodes(values[2]);
      {  int init_values [] = { 3, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      values[2].swap_nodes(values[4]);
      {  int init_values [] = { 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 4, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }
   }
   if(!list_type::linear)
   {
      list_type testlist1 (&values[0], &values[1]);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      values[1].swap_nodes(values[2]);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      values[0].swap_nodes(values[2]);

      {  int init_values [] = { 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      values[0].swap_nodes(values[2]);

      {  int init_values [] = { 1 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
   }
}

template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_clone(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;

      list_type testlist1 (&values[0], &values[0] + values.size());
      list_type testlist2;

      testlist2.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2 == testlist1);
      testlist2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());
}

template<class ValueTraits, bool Linear, bool CacheLast>
void test_slist<ValueTraits, Linear, CacheLast>
   ::test_container_from_end(std::vector<typename ValueTraits::value_type>& values
                            ,detail::bool_<false>)
{
   typedef typename ValueTraits::value_type value_type;
   typedef slist
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      , linear<Linear>
      , cache_last<CacheLast>
      > list_type;
   list_type testlist1 (&values[0], &values[0] + values.size());
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.end()));
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.cend()));
}

template<class VoidPointer, bool constant_time_size>
class test_main_template
{
   public:
   int operator()()
   {
      typedef testvalue<hooks<VoidPointer> , constant_time_size> value_type;
      std::vector<value_type> data (5);
      for (int i = 0; i < 5; ++i)
         data[i].value_ = i + 1;

      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , false
                 , false
                >::test_all(data);
      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , false
                 , false
                >::test_all(data);

      //Now linear slists
      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , true
                 , false
                >::test_all(data);

      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , true
                 , false
                >::test_all(data);

      //Now the same but caching the last node
      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , false
                 , true
                >::test_all(data);
      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , false
                 , true
                >::test_all(data);

      //Now linear slists
      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , true
                 , true
                >::test_all(data);

      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , true
                 , true
                >::test_all(data);
      return 0;
   }
};

template<class VoidPointer>
class test_main_template<VoidPointer, false>
{
   public:
   int operator()()
   {
      typedef testvalue<hooks<VoidPointer> , false> value_type;
      std::vector<value_type> data (5);
      for (int i = 0; i < 5; ++i)
         data[i].value_ = i + 1;

      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , false
                 , false
                >::test_all(data);

      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , false
                 , false
                >::test_all(data);

      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::auto_base_hook_type
                  >::type
                 , false
                 , false
                >::test_all(data);

      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::auto_member_hook_type
                               , &value_type::auto_node_
                               >
                  >::type
                 , false
                 , false
                >::test_all(data);

      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , true
                 , false
                >::test_all(data);

      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , true
                 , false
                >::test_all(data);

      //Now cache last
      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , false
                 , true
                >::test_all(data);

      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , false
                 , true
                >::test_all(data);

      test_slist < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                 , true
                 , true
                >::test_all(data);

      test_slist < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                 , true
                 , true
                >::test_all(data);
      return 0;
   }
};

int main(int, char* [])
{
   test_main_template<void*, false>()();
   test_main_template<smart_ptr<void>, false>()();
   test_main_template<void*, true>()();
   test_main_template<smart_ptr<void>, true>()();
   return boost::report_errors();
}
#include <boost/intrusive/detail/config_end.hpp>

/*
#include <iostream>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>

namespace intrusive = boost::intrusive;

class object : public boost::noncopyable
{
public:
   int o;
    object()
	{
	}
    virtual ~object()
	{
	}
};


class signal : virtual public object
{
public:
	signal()
	{
	}
	virtual ~signal()
	{
	}
};


class set_item : public signal
{
public:
    set_item()
	{
	}
    virtual ~set_item()
	{
	}

public:
	virtual const std::string& get_buffer() const
	{
		return m_buffer;
	}

	typedef intrusive::set_member_hook<
		intrusive::link_mode<intrusive::auto_unlink>
		> hook;
	hook m_hook;

	std::string m_buffer;
};


template <class T, class M, M (T::*V)>
struct member_comparator
{
    bool operator()(const T& t1, const T& t2) const
    {
        return (t1.*V) < (t2.*V);
    }
    bool operator()(const M& m, const T& t) const
    {
        return m < (t.*V);
    }
    bool operator()(const T& t, const M& m) const
    {
        return (t.*V) < m;
    }
};

class kk{ int a; float f; };

class list_item : public kk, virtual public object
{
public:
	list_item()
	{
	}
	virtual ~list_item()
	{
	}

	virtual void f()
	{
	}

	typedef intrusive::list_member_hook<
		intrusive::link_mode<intrusive::auto_unlink>
		> hook;
	hook m_hook;
};

set_item  srec;
list_item lrec;

const set_item::hook  set_item::*  sptr_to_member = &set_item::m_hook;
const list_item::hook list_item::* lptr_to_member = &list_item::m_hook;

int main(int argc, char** argv)
{
   int a = sizeof(sptr_to_member);
   int b = sizeof(lptr_to_member);
   const std::type_info &ta = typeid(set_item);
   const std::type_info &tb = typeid(list_item);

   const set_item::hook  &sh = srec.*sptr_to_member;
   const list_item::hook &l2 = lrec.*lptr_to_member;
   
	{
		typedef member_comparator<
			set_item,
			std::string,
			&set_item::m_buffer
			> set_item_comparator;

		typedef intrusive::set<
			set_item,
			intrusive::compare<set_item_comparator>,
			intrusive::member_hook<
				set_item,
				set_item::hook,
				&set_item::m_hook
				>,
			intrusive::constant_time_size<false>
			> set_items
			;

		union
		{
			int as_int[2];
			const set_item::hook set_item::* ptr_to_member;
		}
		sss;
		sss.ptr_to_member = &set_item::m_hook;

		std::cout << "set offsets: " << sss.as_int[0] << ":" << sss.as_int[1] << " and " << offsetof(set_item,m_hook) << std::endl;

		set_items rr;

		std::string key = "123";
		set_items::insert_commit_data icd;
		std::pair<set_items::iterator,bool> ir = rr.insert_check(
			key,
			set_item_comparator(),
			icd
			);

		if ( !ir.second )
		{
			throw std::exception();
		}

		set_item rec;
		rec.m_buffer = key;
		set_items::iterator i = rr.insert_commit( rec, icd );

		set_item* rrr = &(*i);

		std::cout << "set pointers: " << ((void*)rrr) << " and " << ((void*)&rec) << std::endl;
	}

	{
		typedef intrusive::list<
			list_item,
			intrusive::member_hook<
				list_item,
				list_item::hook,
				&list_item::m_hook
				>,
			intrusive::constant_time_size<false>
			> list_items
			;

		union
		{
			int as_int[2];
			const list_item::hook list_item::* ptr_to_member;
		}
		sss;
		sss.ptr_to_member = &list_item::m_hook;

		std::cout << "list offsets: " << sss.as_int[0] << ":" << sss.as_int[1] << " and " << offsetof(list_item,m_hook) << std::endl;

		list_items rr;

		list_item rec;
      const list_item::hook &h = rec.*sss.ptr_to_member;
		rr.push_back( rec );

		list_item* rrr = &rr.front();

		std::cout << "list pointers: " << ((void*)rrr) << " and " << ((void*)&rec) << std::endl;
	}

	return 0;
}
*/