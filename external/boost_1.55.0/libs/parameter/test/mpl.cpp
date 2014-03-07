// Copyright David Abrahams 2006. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "basics.hpp"
#include <boost/mpl/list.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/size.hpp>
#include <boost/type_traits/add_pointer.hpp>

# include <boost/mpl/contains.hpp>

namespace test
{
  namespace mpl = boost::mpl;

  template <class Set>
  struct assert_in_set
  {
      template <class T>
      void operator()(T*)
      {
          BOOST_MPL_ASSERT((mpl::contains<Set,T>));
      }
  };

  
  template<class Expected, class Params>
  void f_impl(Params const& p BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Expected))
  {
      BOOST_MPL_ASSERT_RELATION(
          mpl::size<Expected>::value
        , ==
        , mpl::size<Params>::value
      );

      mpl::for_each<Params, boost::add_pointer<mpl::_1> >(assert_in_set<Expected>());
  }

  template<class Expected, class Tester, class Name, class Value, class Index>
  void f(Tester const& t, const Name& name_, 
      const Value& value_, const Index& index_ BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Expected))
  {
      f_impl<Expected>(f_parameters()(t, name_, value_, index_));
  }

  template<class Expected, class Tester, class Name, class Value>
  void f(Tester const& t, const Name& name_, const Value& value_ BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Expected))
  {
      f_impl<Expected>(f_parameters()(t, name_, value_));
  }

  template<class Expected, class Tester, class Name>
  void f(Tester const& t, const Name& name_ BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Expected))
  {
      f_impl<Expected>(f_parameters()(t, name_));
  }

  void run()
  {
      typedef test::tag::tester tester_;
      typedef test::tag::name name_;
      typedef test::tag::value value_;
      typedef test::tag::index index_;

      f<mpl::list4<tester_,name_,value_,index_> >(1, 2, 3, 4);
      f<mpl::list3<tester_,name_,index_> >(1, 2, index = 3);
      f<mpl::list3<tester_,name_,index_> >(1, index = 2, name = 3);
      f<mpl::list2<name_,value_> >(name = 3, value = 4);
      f_impl<mpl::list1<value_> >(value = 4);
  }
}

int main()
{
    test::run();
    return 0;
}

