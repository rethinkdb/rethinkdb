//
//=======================================================================
// Author: Jeremiah Willcock
//
// Copyright 2012, Trustees of Indiana University
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//

#include <boost/property_map/function_property_map.hpp>
#include <boost/property_map/transform_value_property_map.hpp>
#include <boost/concept_check.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/test/minimal.hpp>
#include <boost/static_assert.hpp>

// Ensure this is not default constructible
struct times2 {typedef int result_type; times2(int) {}; int operator()(int x) const {return 2 * x;}};

template <typename T>
struct add1 {typedef T result_type; T operator()(const T& x) const {return x + 1;}};

template <typename T>
struct add1_val {typedef T result_type; T operator()(T x) const {return x + 1;}};

template <typename T>
struct return_fixed_ref {
  int* ptr;
  return_fixed_ref(int* ptr): ptr(ptr) {}
  typedef int& result_type;
  int& operator()(const T&) const {return *ptr;}
};

int test_main(int, char**) {
  using namespace boost;
  typedef function_property_map<times2, int> PM;
  PM orig_pm(times2(0));
  function_requires<ReadablePropertyMapConcept<transform_value_property_map<add1<int>, PM>, int> >();
  function_requires<ReadablePropertyMapConcept<transform_value_property_map<add1<int>, PM, double>, int> >();
  function_requires<ReadablePropertyMapConcept<transform_value_property_map<add1_val<int>, PM>, int> >();
  function_requires<ReadablePropertyMapConcept<transform_value_property_map<add1_val<int>, PM, double>, int> >();
  function_requires<ReadablePropertyMapConcept<transform_value_property_map<return_fixed_ref<int>, PM>, int> >();
  function_requires<WritablePropertyMapConcept<transform_value_property_map<return_fixed_ref<int>, PM>, int> >();
  function_requires<ReadWritePropertyMapConcept<transform_value_property_map<return_fixed_ref<int>, PM>, int> >();
  function_requires<LvaluePropertyMapConcept<transform_value_property_map<return_fixed_ref<int>, PM>, int> >();

  BOOST_STATIC_ASSERT((boost::is_same<boost::property_traits<transform_value_property_map<add1<int>, PM> >::category, boost::readable_property_map_tag>::value));
  BOOST_STATIC_ASSERT((boost::is_same<boost::property_traits<transform_value_property_map<add1_val<int>, PM> >::category, boost::readable_property_map_tag>::value));
  BOOST_STATIC_ASSERT((boost::is_same<boost::property_traits<transform_value_property_map<return_fixed_ref<int>, PM> >::category, boost::lvalue_property_map_tag>::value));

  BOOST_CHECK(get(transform_value_property_map<add1<int>, PM>(add1<int>(), orig_pm), 3) == 7);
  BOOST_CHECK(get(transform_value_property_map<add1<int>, PM>(add1<int>(), orig_pm), 4) == 9);
  BOOST_CHECK(get(make_transform_value_property_map(add1<int>(), orig_pm), 5) == 11);
  BOOST_CHECK(get(transform_value_property_map<add1_val<int>, PM>(add1_val<int>(), orig_pm), 3) == 7);
  BOOST_CHECK(get(transform_value_property_map<add1_val<int>, PM>(add1_val<int>(), orig_pm), 4) == 9);
  BOOST_CHECK(get(make_transform_value_property_map<int>(add1_val<int>(), orig_pm), 5) == 11);
  int val;
  const transform_value_property_map<return_fixed_ref<int>, PM> pm(return_fixed_ref<int>((&val)), orig_pm);
  put(pm, 1, 6);
  BOOST_CHECK(get(pm, 2) == 6);
  BOOST_CHECK((get(pm, 3) = 7) == 7);
  BOOST_CHECK(get(pm, 4) == 7);
  const transform_value_property_map<return_fixed_ref<int>, PM> pm2 = pm; // Check shallow copying
  BOOST_CHECK(get(pm2, 5) == 7);
  put(pm2, 3, 1);
  BOOST_CHECK(get(pm, 1) == 1);

  return 0;
}
