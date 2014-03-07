/* Boost.MultiIndex test for safe_mode.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_safe_mode.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include "pre_multi_index.hpp"
#include "employee.hpp"
#include "pair_of_ints.hpp"
#include <stdexcept>
#include <boost/detail/lightweight_test.hpp>

using namespace boost::multi_index;

#define TRY_SAFE_MODE \
try{

#define CATCH_SAFE_MODE(err) \
  throw std::runtime_error("safe mode violation not detected");\
}catch(const safe_mode_exception& e){\
  if(e.error_code!=(err))throw std::runtime_error(\
    "safe mode violation not expected");\
}

template<typename T> void prevent_unused_var_warning(const T&){}

template<typename Policy>
static void local_test_safe_mode(
  std::forward_iterator_tag
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Policy))
{
  typedef typename Policy::container      container;
  typedef typename Policy::index_type     index_type;
  typedef typename index_type::value_type value_type;
  typedef typename index_type::iterator   iterator;

  container   c,c2;
  index_type& i=Policy::index_from_container(c);
  index_type& i2=Policy::index_from_container(c2);
  Policy::insert(i,Policy::some_value());

  TRY_SAFE_MODE
    iterator it;
    iterator it2=i.begin();
    it2=it;
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    iterator it;
    value_type e=*it;
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)
  
  TRY_SAFE_MODE
    iterator it=i.end();
    value_type e=*it;
  CATCH_SAFE_MODE(safe_mode::not_dereferenceable_iterator)

  TRY_SAFE_MODE
    iterator it=i.end();
    ++it;
  CATCH_SAFE_MODE(safe_mode::not_incrementable_iterator)

  TRY_SAFE_MODE
    iterator it;
    iterator it2;
    bool b=(it==it2);
    prevent_unused_var_warning(b);
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    iterator it=i.begin();
    iterator it2;
    bool b=(it==it2);
    prevent_unused_var_warning(b);
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    iterator it=i.begin();
    iterator it2=i2.begin();
    bool b=(it==it2);
    prevent_unused_var_warning(b);
  CATCH_SAFE_MODE(safe_mode::not_same_owner)

  TRY_SAFE_MODE
    i.erase(i.end(),i.begin());
  CATCH_SAFE_MODE(safe_mode::invalid_range)

  TRY_SAFE_MODE
    iterator it;
    Policy::insert(i,it,Policy::some_value());
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    i.erase(i.end());
  CATCH_SAFE_MODE(safe_mode::not_dereferenceable_iterator)

  TRY_SAFE_MODE
    iterator it=i.begin();
    Policy::insert(i2,it,Policy::some_value());
  CATCH_SAFE_MODE(safe_mode::not_owner)

  TRY_SAFE_MODE
    iterator it=i.begin();
    iterator it2=i2.end();
    i2.erase(it,it2);
  CATCH_SAFE_MODE(safe_mode::not_owner)

  TRY_SAFE_MODE
    iterator it=Policy::insert(i,Policy::another_value());
    i.erase(it);
    i.erase(it);
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    container   c3(c);
    index_type& i3=Policy::index_from_container(c3);
    iterator it=Policy::insert(i3,Policy::another_value());
    i3.clear();
    i3.erase(it);
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    iterator it;
    {
      container   c3;
      index_type& i3=Policy::index_from_container(c3);
      it=i3.end();
    }
    it=it;
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    iterator it;
    {
      container   c3;
      index_type& i3=Policy::index_from_container(c3);
      it=Policy::insert(i3,Policy::some_value());
    }
    value_type e=*it;
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    iterator it;
    {
      container   c3;
      index_type& i3=Policy::index_from_container(c3);
      it=Policy::insert(i3,Policy::some_value());
    }
    iterator it2;
    it2=it;
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    container   c3(c);
    container   c4;
    index_type& i3=Policy::index_from_container(c3);
    index_type& i4=Policy::index_from_container(c4);
    iterator  it=i3.begin();
    i3.swap(i4);
    i3.erase(it);
  CATCH_SAFE_MODE(safe_mode::not_owner)

  /* this, unlike the previous case, is indeed correct, test safe mode
   * gets it right
   */
  { 
    container   c3(c);
    container   c4;
    index_type& i3=Policy::index_from_container(c3);
    index_type& i4=Policy::index_from_container(c4);
    iterator  it=i3.begin();
    i3.swap(i4);
    i4.erase(it);
  }

  TRY_SAFE_MODE
    iterator it=i.end();
    typename container::iterator it2=project<0>(c2,it);
  CATCH_SAFE_MODE(safe_mode::not_owner)

  TRY_SAFE_MODE
    iterator it=Policy::insert(i,Policy::another_value());
    typename container::iterator it2=project<0>(c,it);
    i.erase(it);
    value_type e=*it2;
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  /* testcase for bug reported at
   * http://lists.boost.org/boost-users/2006/02/17230.php
   */
  {
    container c3(c);
    index_type& i3=Policy::index_from_container(c3);
    iterator it=i3.end();
    i3.clear();
    it=i3.end();
  }

  /* testcase for doppelganger bug of that discovered for STLport at
   * http://lists.boost.org/Archives/boost/2006/04/102740.php
   */
  {
    container c3;
    index_type& i3=Policy::index_from_container(c3);
    iterator it=i3.end();
    i3.clear();
    it=it;
    BOOST_TEST(it==i3.end());
  }
}

template<typename Policy>
static void local_test_safe_mode(
  std::bidirectional_iterator_tag
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Policy))
{
  ::local_test_safe_mode<Policy>(std::forward_iterator_tag());

  typedef typename Policy::container      container;
  typedef typename Policy::index_type     index_type;
  typedef typename index_type::value_type value_type;
  typedef typename index_type::iterator   iterator;

  container   c;
  index_type& i=Policy::index_from_container(c);
  Policy::insert(i,Policy::some_value());

  TRY_SAFE_MODE
    iterator it=i.begin();
    --it;
  CATCH_SAFE_MODE(safe_mode::not_decrementable_iterator)
}

template<typename Policy>
static void local_test_safe_mode(
  std::random_access_iterator_tag
  BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(Policy))
{
  ::local_test_safe_mode<Policy>(std::bidirectional_iterator_tag());

  typedef typename Policy::container      container;
  typedef typename Policy::index_type     index_type;
  typedef typename index_type::value_type value_type;
  typedef typename index_type::iterator   iterator;

  container   c;
  index_type& i=Policy::index_from_container(c);
  Policy::insert(i,Policy::some_value());

  TRY_SAFE_MODE
    iterator it=i.begin();
    it+=2;
  CATCH_SAFE_MODE(safe_mode::out_of_bounds)

  TRY_SAFE_MODE
    iterator it=i.begin();
    it-=1;
  CATCH_SAFE_MODE(safe_mode::out_of_bounds)
}

template<typename Policy>
static void local_test_safe_mode(BOOST_EXPLICIT_TEMPLATE_TYPE(Policy))
{
  typedef typename Policy::index_type::iterator::iterator_category category;
  ::local_test_safe_mode<Policy>(category());
}

template<typename Policy>
static void local_test_safe_mode_with_rearrange(
  BOOST_EXPLICIT_TEMPLATE_TYPE(Policy))
{
  ::local_test_safe_mode<Policy>();

  typedef typename Policy::container      container;
  typedef typename Policy::index_type     index_type;
  typedef typename index_type::value_type value_type;
  typedef typename index_type::iterator   iterator;

  container   c;
  index_type& i=Policy::index_from_container(c);
  Policy::insert(i,Policy::some_value());

  TRY_SAFE_MODE
    iterator it;
    i.splice(it,i,i.begin(),i.end());
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    container   c2(c);
    index_type& i2=Policy::index_from_container(c2);
    iterator    it2=i2.begin();
    iterator    it=i.begin();
    i.splice(it2,i2,it);
  CATCH_SAFE_MODE(safe_mode::not_owner)

  TRY_SAFE_MODE
    i.splice(i.begin(),i,i.begin(),i.end());
  CATCH_SAFE_MODE(safe_mode::inside_range)

  TRY_SAFE_MODE
    i.splice(i.begin(),i,i.end(),i.begin());
  CATCH_SAFE_MODE(safe_mode::invalid_range)

  TRY_SAFE_MODE
    i.splice(i.begin(),i);
  CATCH_SAFE_MODE(safe_mode::same_container)

  TRY_SAFE_MODE
    iterator it;
    i.relocate(it,i.begin(),i.end());
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    i.relocate(i.begin(),i.begin(),i.end());
  CATCH_SAFE_MODE(safe_mode::inside_range)

  TRY_SAFE_MODE
    i.relocate(i.begin(),i.end(),i.begin());
  CATCH_SAFE_MODE(safe_mode::invalid_range)
}

template<typename MultiIndexContainer,int N>
struct index_policy_base
{
  typedef MultiIndexContainer                    container;
  typedef typename 
    boost::multi_index::detail::prevent_eti<
      container,
    typename nth_index<container,N>::type>::type index_type;

  static index_type& index_from_container(container& c){return get<N>(c);}
};

template<typename MultiIndexContainer,int N>
struct key_based_index_policy_base:
  index_policy_base<MultiIndexContainer,N>
{
  typedef index_policy_base<MultiIndexContainer,N> super;

  typedef typename super::container       container;
  typedef typename super::index_type      index_type;
  typedef typename index_type::value_type value_type;
  typedef typename index_type::iterator   iterator;

  static iterator insert(index_type& i,const value_type& v)
  {
    return i.insert(v).first;
  }

  static iterator insert(index_type& i,iterator it,const value_type& v)
  {
    return i.insert(it,v);
  }
};

template<typename MultiIndexContainer,int N>
struct non_key_based_index_policy_base:
  index_policy_base<MultiIndexContainer,N>
{
  typedef index_policy_base<MultiIndexContainer,N> super;

  typedef typename super::container       container;
  typedef typename super::index_type      index_type;
  typedef typename index_type::value_type value_type;
  typedef typename index_type::iterator   iterator;

  static iterator insert(index_type& i,const value_type& v)
  {
    return i.push_back(v).first;
  }

  static iterator insert(index_type& i,iterator it,const value_type& v)
  {
    return i.insert(it,v).first;
  }
};

struct employee_set_policy_base
{
  static employee    some_value(){return employee(0,"Joe",31,1123);}
  static employee    another_value(){return employee(1,"Robert",27,5601);}
};

struct employee_set_policy:
  employee_set_policy_base,
  key_based_index_policy_base<employee_set,0>
{};

struct employee_set_by_name_policy:
  employee_set_policy_base,
  key_based_index_policy_base<employee_set,1>
{};

struct employee_set_as_inserted_policy:
  employee_set_policy_base,
  non_key_based_index_policy_base<employee_set,3>
{};

struct employee_set_randomly_policy:
  employee_set_policy_base,
  non_key_based_index_policy_base<employee_set,5>
{};

template<typename IntegralBimap>
static void test_integral_bimap(BOOST_EXPLICIT_TEMPLATE_TYPE(IntegralBimap))
{
  typedef typename IntegralBimap::value_type value_type;
  typedef typename IntegralBimap::iterator   iterator;

  TRY_SAFE_MODE
    IntegralBimap bm;
    iterator it=bm.insert(value_type(0,0)).first;
    bm.insert(value_type(1,1));
    bm.modify(it,increment_first);
    value_type v=*it;
    prevent_unused_var_warning(v);
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)

  TRY_SAFE_MODE
    IntegralBimap bm;
    iterator it=bm.insert(value_type(0,0)).first;
    bm.insert(value_type(1,1));
    bm.modify(it,increment_second);
    pair_of_ints v=*it;
    prevent_unused_var_warning(v);
  CATCH_SAFE_MODE(safe_mode::invalid_iterator)
}

void test_safe_mode()
{
  local_test_safe_mode<employee_set_policy>();
  local_test_safe_mode<employee_set_by_name_policy>();
  local_test_safe_mode_with_rearrange<employee_set_as_inserted_policy>();
  local_test_safe_mode_with_rearrange<employee_set_randomly_policy>();

  typedef multi_index_container<
    pair_of_ints,
    indexed_by<
      ordered_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,first)>,
      ordered_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,second)> >
  > bimap0_type;

  /* MSVC++ 6.0 chokes on test_integral_bimap without this
   * explicit instantiation
   */
  bimap0_type bm0;
  test_integral_bimap<bimap0_type>();

  typedef multi_index_container<
    pair_of_ints,
    indexed_by<
      ordered_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,first)>,
      hashed_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,second)> >
  > bimap1_type;

  bimap1_type bm1;
  test_integral_bimap<bimap1_type>();

  typedef multi_index_container<
    pair_of_ints,
    indexed_by<
      hashed_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,first)>,
      ordered_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,second)> >
  > bimap2_type;

  bimap2_type bm2;
  test_integral_bimap<bimap2_type>();

  typedef multi_index_container<
    pair_of_ints,
    indexed_by<
      hashed_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,first)>,
      hashed_unique<BOOST_MULTI_INDEX_MEMBER(pair_of_ints,int,second)> >
  > bimap3_type;

  bimap3_type bm3;
  test_integral_bimap<bimap3_type>();
}
