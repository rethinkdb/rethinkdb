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
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include "itestvalue.hpp"
#include "smart_ptr.hpp"
#include "common_functors.hpp"
#include <vector>
#include <boost/detail/lightweight_test.hpp>
#include "test_macros.hpp"
#include "test_container.hpp"

using namespace boost::intrusive;

class my_tag;

template<class VoidPointer>
struct hooks
{
   typedef list_base_hook<void_pointer<VoidPointer> >                base_hook_type;
   typedef list_base_hook< link_mode<auto_unlink>
                         , void_pointer<VoidPointer>, tag<my_tag> >  auto_base_hook_type;
   typedef list_member_hook<void_pointer<VoidPointer>, tag<my_tag> > member_hook_type;
   typedef list_member_hook< link_mode<auto_unlink>
                           , void_pointer<VoidPointer> >             auto_member_hook_type;
};

template<class ValueTraits>
struct test_list
{
   typedef typename ValueTraits::value_type value_type;
   static void test_all(std::vector<value_type>& values);
   static void test_front_back(std::vector<value_type>& values);
   static void test_sort(std::vector<value_type>& values);
   static void test_merge(std::vector<value_type>& values);
   static void test_remove_unique(std::vector<value_type>& values);
   static void test_insert(std::vector<value_type>& values);
   static void test_shift(std::vector<value_type>& values);
   static void test_swap(std::vector<value_type>& values);
   static void test_clone(std::vector<value_type>& values);
   static void test_container_from_end(std::vector<value_type>& values);
};

template<class ValueTraits>
void test_list<ValueTraits>::test_all(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
   {
      list_type list(values.begin(), values.end());
      test::test_container(list);
      list.clear();
      list.insert(list.end(), values.begin(), values.end());
      test::test_sequence_container(list, values);
   }

   test_front_back(values);
   test_sort(values);
   test_merge(values);
   test_remove_unique(values);
   test_insert(values);
   test_shift(values);
   test_swap(values);
   test_clone(values);
   test_container_from_end(values);
}

//test: push_front, pop_front, push_back, pop_back, front, back, size, empty:
template<class ValueTraits>
void test_list<ValueTraits>
   ::test_front_back(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
   list_type testlist;
   BOOST_TEST (testlist.empty());

   testlist.push_back (values[0]);
   BOOST_TEST (testlist.size() == 1);
   BOOST_TEST (&testlist.front() == &values[0]);
   BOOST_TEST (&testlist.back() == &values[0]);

   testlist.push_front (values[1]);
   BOOST_TEST (testlist.size() == 2);
   BOOST_TEST (&testlist.front() == &values[1]);
   BOOST_TEST (&testlist.back() == &values[0]);

   testlist.pop_back();
   BOOST_TEST (testlist.size() == 1);
   const list_type &const_testlist = testlist;
   BOOST_TEST (&const_testlist.front() == &values[1]);
   BOOST_TEST (&const_testlist.back() == &values[1]);

   testlist.pop_front();
   BOOST_TEST (testlist.empty());
}


//test: constructor, iterator, reverse_iterator, sort, reverse:
template<class ValueTraits>
void test_list<ValueTraits>
   ::test_sort(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
   list_type testlist(values.begin(), values.end());

   {  int init_values [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }

   testlist.sort (even_odd());
   {  int init_values [] = { 5, 3, 1, 4, 2 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.rbegin() );  }

   testlist.reverse();
   {  int init_values [] = { 5, 3, 1, 4, 2 };
      TEST_INTRUSIVE_SEQUENCE( init_values, testlist.begin() );  }
}

//test: merge due to error in merge implementation:
template<class ValueTraits>
void test_list<ValueTraits>
   ::test_remove_unique (std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
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
      list.insert(list.end(), values2.begin(), values2.end());
      list.sort();
      int init_values [] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, list.begin() );
      list.unique();
      int init_values2 [] = { 1, 2, 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values2, list.begin() );
   }
}

//test: merge due to error in merge implementation:
template<class ValueTraits>
void test_list<ValueTraits>
   ::test_merge (std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
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

//test: assign, insert, const_iterator, const_reverse_iterator, erase, s_iterator_to:
template<class ValueTraits>
void test_list<ValueTraits>
   ::test_insert(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
   list_type testlist;
   testlist.assign (&values[0] + 2, &values[0] + 5);

   const list_type& const_testlist = testlist;
   {  int init_values [] = { 3, 4, 5 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }

   typename list_type::iterator i = ++testlist.begin();
   BOOST_TEST (i->value_ == 4);

   {
   typename list_type::const_iterator ci = typename list_type::iterator();
   (void)ci;
   }

   testlist.insert (i, values[0]);
   {  int init_values [] = { 5, 4, 1, 3 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.rbegin() );  }

   i = testlist.iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);

   i = list_type::s_iterator_to (values[4]);
   BOOST_TEST (&*i == &values[4]);

   i = testlist.erase (i);
   BOOST_TEST (i == testlist.end());

   {  int init_values [] = { 3, 1, 4 };
      TEST_INTRUSIVE_SEQUENCE( init_values, const_testlist.begin() );  }
}


template<class ValueTraits>
void test_list<ValueTraits>
   ::test_shift(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
   list_type testlist;
   const int num_values = (int)values.size();
   std::vector<int> expected_values(num_values);

   for(int s = 1; s <= num_values; ++s){
      expected_values.resize(s);
      //Shift forward all possible positions 3 times
      for(int i = 0; i < s*3; ++i){
         testlist.insert(testlist.begin(), &values[0], &values[0] + s);
         testlist.shift_forward(i);
         for(int j = 0; j < s; ++j){
            expected_values[(j + s - i%s) % s] = (j + 1);
         }
         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin());
         testlist.clear();
      }

      //Shift backwards all possible positions
      for(int i = 0; i < s*3; ++i){
         testlist.insert(testlist.begin(), &values[0], &values[0] + s);
         testlist.shift_backwards(i);
         for(int j = 0; j < s; ++j){
            expected_values[(j + i) % s] = (j + 1);
         }
         TEST_INTRUSIVE_SEQUENCE_EXPECTED(expected_values, testlist.begin());
         testlist.clear();
      }
   }
}

//test: insert (seq-version), swap, splice, erase (seq-version):
template<class ValueTraits>
void test_list<ValueTraits>
   ::test_swap(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
   {
      list_type testlist1 (&values[0], &values[0] + 2);
      list_type testlist2;
      testlist2.insert (testlist2.end(), &values[0] + 2, &values[0] + 5);
      testlist1.swap (testlist2);

      {  int init_values [] = { 3, 4, 5 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 1, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      testlist2.splice (++testlist2.begin(), testlist1);
      {  int init_values [] = { 1, 3, 4, 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      BOOST_TEST (testlist1.empty());

      testlist1.splice (testlist1.end(), testlist2, ++(++testlist2.begin()));
      {  int init_values [] = { 4 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }

      {  int init_values [] = { 1, 3, 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      testlist1.splice (testlist1.end(), testlist2,
                        testlist2.begin(), ----testlist2.end());
      {  int init_values [] = { 4, 1, 3 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist1.begin() );  }
      {  int init_values [] = { 5, 2 };
         TEST_INTRUSIVE_SEQUENCE( init_values, testlist2.begin() );  }

      testlist1.erase (testlist1.iterator_to(values[0]), testlist1.end());
      BOOST_TEST (testlist1.size() == 1);
      BOOST_TEST (&testlist1.front() == &values[3]);
   }
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

template<class ValueTraits>
void test_list<ValueTraits>
   ::test_container_from_end(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
   list_type testlist1 (&values[0], &values[0] + values.size());
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.end()));
   BOOST_TEST (testlist1 == list_type::container_from_end_iterator(testlist1.cend()));
}


template<class ValueTraits>
void test_list<ValueTraits>
   ::test_clone(std::vector<typename ValueTraits::value_type>& values)
{
   typedef typename ValueTraits::value_type value_type;
   typedef list
      < value_type
      , value_traits<ValueTraits>
      , size_type<std::size_t>
      , constant_time_size<value_type::constant_time_size>
      > list_type;
      list_type testlist1 (&values[0], &values[0] + values.size());
      list_type testlist2;

      testlist2.clone_from(testlist1, test::new_cloner<value_type>(), test::delete_disposer<value_type>());
      BOOST_TEST (testlist2 == testlist1);
      testlist2.clear_and_dispose(test::delete_disposer<value_type>());
      BOOST_TEST (testlist2.empty());
}

template<class VoidPointer, bool constant_time_size>
class test_main_template
{
   public:
   int operator()()
   {
      typedef testvalue<hooks<VoidPointer>, constant_time_size> value_type;
      std::vector<value_type> data (5);
      for (int i = 0; i < 5; ++i)
         data[i].value_ = i + 1;

      test_list < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                >::test_all(data);
      test_list < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
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
      typedef testvalue<hooks<VoidPointer>, false> value_type;
      std::vector<value_type> data (5);
      for (int i = 0; i < 5; ++i)
         data[i].value_ = i + 1;

      test_list < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::base_hook_type
                  >::type
                >::test_all(data);

      test_list < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::member_hook_type
                               , &value_type::node_
                               >
                  >::type
                >::test_all(data);

//      test_list<stateful_value_traits
//                  < value_type
//                  , list_node_traits<VoidPointer>
//                  , safe_link>
//               >::test_all(data);
      test_list < typename detail::get_base_value_traits
                  < value_type
                  , typename hooks<VoidPointer>::auto_base_hook_type
                  >::type
                >::test_all(data);

      test_list < typename detail::get_member_value_traits
                  < value_type
                  , member_hook< value_type
                               , typename hooks<VoidPointer>::auto_member_hook_type
                               , &value_type::auto_node_
                               >
                  >::type
                >::test_all(data);

//      test_list<stateful_value_traits
//                  < value_type
//                  , list_node_traits<VoidPointer>
//                  , auto_unlink>
//               >::test_all(data);

      return 0;
   }
};

int main( int, char* [] )
{
   test_main_template<void*, false>()();
   test_main_template<smart_ptr<void>, false>()();
   test_main_template<void*, true>()();
   test_main_template<smart_ptr<void>, true>()();
   return boost::report_errors();
}
