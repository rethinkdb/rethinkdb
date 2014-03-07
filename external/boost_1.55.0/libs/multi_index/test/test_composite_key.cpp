/* Boost.MultiIndex test for composite_key.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_composite_key.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/lightweight_test.hpp>
#include "pre_multi_index.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

using namespace boost::multi_index;
using namespace boost::tuples;

struct is_composite_key_result_helper
{
  typedef char yes;
  struct no{char m[2];};

  static no test(void*);

  template<typename CompositeKey>
  static yes test(composite_key_result<CompositeKey>*);
};

template<typename T>
struct is_composite_key_result
{
  typedef is_composite_key_result_helper helper;

  BOOST_STATIC_CONSTANT(bool,
    value=(
      sizeof(helper::test((T*)0))==
      sizeof(typename helper::yes)));
};

template<typename CompositeKeyResult>
struct composite_key_result_length
{
  BOOST_STATIC_CONSTANT(int,
    value=boost::tuples::length<
      BOOST_DEDUCED_TYPENAME 
      CompositeKeyResult::composite_key_type::key_extractor_tuple
    >::value);
};

template<typename T>
struct composite_object_length
{
  typedef typename boost::mpl::if_c<
    is_composite_key_result<T>::value,
    composite_key_result_length<T>,
    boost::tuples::length<T>
  >::type type;

  BOOST_STATIC_CONSTANT(int,value=type::value);
};

template<typename CompositeKeyResult,typename T2>
struct comparison_equal_length
{
  static bool is_less(const CompositeKeyResult& x,const T2& y)
  {
    composite_key_result_equal_to<CompositeKeyResult> eq;
    composite_key_result_less<CompositeKeyResult>     lt;
    composite_key_result_greater<CompositeKeyResult>  gt;

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    std::equal_to<CompositeKeyResult> std_eq;
    std::less<CompositeKeyResult>     std_lt;
    std::greater<CompositeKeyResult>  std_gt;
#endif

    return  (x< y) && !(y< x)&&
           !(x==y) && !(y==x)&&
            (x!=y) &&  (y!=x)&&
           !(x> y) &&  (y> x)&&
           !(x>=y) &&  (y>=x)&&
            (x<=y) && !(y<=x)&&

          !eq(x,y) && !eq(y,x)&&
           lt(x,y) && !lt(y,x)&&
          !gt(x,y) &&  gt(y,x)

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
                   &&
      !std_eq(x,y) && !std_eq(y,x)&&
       std_lt(x,y) && !std_lt(y,x)&&
      !std_gt(x,y) &&  std_gt(y,x)
#endif
                    ;
  }

  static bool is_greater(const CompositeKeyResult& x,const T2& y)
  {
    composite_key_result_equal_to<CompositeKeyResult> eq;
    composite_key_result_less<CompositeKeyResult>     lt;
    composite_key_result_greater<CompositeKeyResult>  gt;

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    std::equal_to<CompositeKeyResult> std_eq;
    std::less<CompositeKeyResult>     std_lt;
    std::greater<CompositeKeyResult>  std_gt;
#endif

    return !(x< y) &&  (y< x)&&
           !(x==y) && !(y==x)&&
            (x!=y) &&  (y!=x)&&
            (x> y) && !(y> x)&&
            (x>=y) && !(y>=x)&&
           !(x<=y) &&  (y<=x)&&

          !eq(x,y) && !eq(y,x)&&
          !lt(x,y) &&  lt(y,x)&&
           gt(x,y) && !gt(y,x)

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
                   &&
      !std_eq(x,y) && !std_eq(y,x)&&
      !std_lt(x,y) &&  std_lt(y,x)&&
       std_gt(x,y) && !std_gt(y,x)
#endif
                    ;
  }

  static bool is_equiv(const CompositeKeyResult& x,const T2& y)
  {
    composite_key_result_equal_to<CompositeKeyResult> eq;
    composite_key_result_less<CompositeKeyResult>     lt;
    composite_key_result_greater<CompositeKeyResult>  gt;

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    std::equal_to<CompositeKeyResult> std_eq;
    std::less<CompositeKeyResult>     std_lt;
    std::greater<CompositeKeyResult>  std_gt;
#endif

    return !(x< y) && !(y< x)&&
            (x==y) &&  (y==x)&&
           !(x!=y) && !(y!=x)&&
           !(x> y) && !(y> x)&&
            (x>=y) &&  (y>=x)&&
            (x<=y) &&  (y<=x)&&

           eq(x,y) &&  eq(y,x)&&
          !lt(x,y) && !lt(y,x)&&
          !gt(x,y) && !gt(y,x)

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
                   &&
       std_eq(x,y) &&  std_eq(y,x)&&
      !std_lt(x,y) && !std_lt(y,x)&&
      !std_gt(x,y) && !std_gt(y,x)
#endif
                    ;
  }
};

template<typename CompositeKeyResult,typename T2>
struct comparison_different_length
{
  static bool is_less(const CompositeKeyResult& x,const T2& y)
  {
    composite_key_result_less<CompositeKeyResult>    lt;
    composite_key_result_greater<CompositeKeyResult> gt;

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    std::less<CompositeKeyResult>    std_lt;
    std::greater<CompositeKeyResult> std_gt;
#endif

    return  (x< y) && !(y< x)&&
           !(x> y) &&  (y> x)&&
           !(x>=y) &&  (y>=x)&&
            (x<=y) && !(y<=x)&&

           lt(x,y) && !lt(y,x)&&
          !gt(x,y) &&  gt(y,x)
          
#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
                   &&
       std_lt(x,y) && !std_lt(y,x)&&
      !std_gt(x,y) &&  std_gt(y,x)
#endif
                    ;
  }

  static bool is_greater(const CompositeKeyResult& x,const T2& y)
  {
    composite_key_result_less<CompositeKeyResult>    lt;
    composite_key_result_greater<CompositeKeyResult> gt;

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    std::less<CompositeKeyResult>    std_lt;
    std::greater<CompositeKeyResult> std_gt;
#endif

    return !(x< y) &&  (y< x)&&
            (x> y) && !(y> x)&&
            (x>=y) && !(y>=x)&&
           !(x<=y) &&  (y<=x)&&

          !lt(x,y) &&  lt(y,x)&&
           gt(x,y) && !gt(y,x)

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
                   &&
      !std_lt(x,y) && std_lt(y,x)&&
       std_gt(x,y) && !std_gt(y,x)
#endif
                    ;
  }

  static bool is_equiv(const CompositeKeyResult& x,const T2& y)
  {
    composite_key_result_less<CompositeKeyResult>    lt;
    composite_key_result_greater<CompositeKeyResult> gt;

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
    std::less<CompositeKeyResult>    std_lt;
    std::greater<CompositeKeyResult> std_gt;
#endif

    return !(x< y) && !(y< x)&&
           !(x> y) && !(y> x)&&
            (x>=y) &&  (y>=x)&&
            (x<=y) &&  (y<=x)&&

          !lt(x,y) && !lt(y,x)&&
          !gt(x,y) && !gt(y,x)

#if !defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
                   &&
      !std_lt(x,y) && !std_lt(y,x)&&
      !std_gt(x,y) && !std_gt(y,x)
#endif
                    ;
  }
};

template<typename CompositeKeyResult,typename T2>
struct comparison_helper:
  boost::mpl::if_c<
    composite_key_result_length<CompositeKeyResult>::value==
      composite_object_length<T2>::value,
    comparison_equal_length<CompositeKeyResult,T2>,
    comparison_different_length<CompositeKeyResult,T2>
  >::type
{
};

template<typename CompositeKeyResult,typename T2>
static bool is_less(const CompositeKeyResult& x,const T2& y)
{
  return comparison_helper<CompositeKeyResult,T2>::is_less(x,y);
}

template<typename CompositeKeyResult,typename T2>
static bool is_greater(const CompositeKeyResult& x,const T2& y)
{
  return comparison_helper<CompositeKeyResult,T2>::is_greater(x,y);
}

template<typename CompositeKeyResult,typename T2>
static bool is_equiv(const CompositeKeyResult& x,const T2& y)
{
  return comparison_helper<CompositeKeyResult,T2>::is_equiv(x,y);
}

template<typename T1,typename T2,typename Compare>
static bool is_less(const T1& x,const T2& y,const Compare& c)
{
  return c(x,y)&&!c(y,x);
}

template<typename T1,typename T2,typename Compare>
static bool is_greater(const T1& x,const T2& y,const Compare& c)
{
  return c(y,x)&&!c(x,y);
}

template<typename T1,typename T2,typename Compare>
static bool is_equiv(const T1& x,const T2& y,const Compare& c)
{
  return !c(x,y)&&!c(y,x);
}

template<typename T1,typename T2,typename Compare,typename Equiv>
static bool is_less(
  const T1& x,const T2& y,const Compare& c,const Equiv& eq)
{
  return c(x,y)&&!c(y,x)&&!eq(x,y)&&!eq(y,x);
}

template<typename T1,typename T2,typename Compare,typename Equiv>
static bool is_greater(
  const T1& x,const T2& y,const Compare& c,const Equiv& eq)
{
  return c(y,x)&&!c(x,y)&&!eq(x,y)&&!eq(y,x);
}

template<typename T1,typename T2,typename Compare,typename Equiv>
static bool is_equiv(
  const T1& x,const T2& y,const Compare& c,const Equiv& eq)
{
  return !c(x,y)&&!c(y,x)&&eq(x,y)&&eq(y,x);
}

struct xyz
{
  xyz(int x_=0,int y_=0,int z_=0):x(x_),y(y_),z(z_){}

  int  x;
  int  y;
  int  z;
};

struct modulo_equal
{
  modulo_equal(int i):i_(i){}
  bool operator ()(int x,int y)const{return (x%i_)==(y%i_);}

private:
  int i_;
};

struct xystr
{
  xystr(int x_=0,int y_=0,std::string str_=0):x(x_),y(y_),str(str_){}

  int         x;
  int         y;
  std::string str;
};

void test_composite_key()
{
  typedef composite_key<
    xyz,
    BOOST_MULTI_INDEX_MEMBER(xyz,int,x),
    BOOST_MULTI_INDEX_MEMBER(xyz,int,y),
    BOOST_MULTI_INDEX_MEMBER(xyz,int,z)
  > ckey_t1;

  typedef multi_index_container<
    xyz,
    indexed_by<
      ordered_unique<
        ckey_t1
#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
        ,composite_key_result_less<ckey_t1::result_type>
#endif
      >
    >
  > indexed_t1;

  indexed_t1 mc1;
  mc1.insert(xyz(0,0,0));
  mc1.insert(xyz(0,0,1));
  mc1.insert(xyz(0,1,0));
  mc1.insert(xyz(0,1,1));
  mc1.insert(xyz(1,0,0));
  mc1.insert(xyz(1,0,1));
  mc1.insert(xyz(1,1,0));
  mc1.insert(xyz(1,1,1));

  BOOST_TEST(mc1.size()==8);
  BOOST_TEST(
    std::distance(
      mc1.find(mc1.key_extractor()(xyz(0,0,0))),
      mc1.find(mc1.key_extractor()(xyz(1,0,0))))==4);
  BOOST_TEST(
    std::distance(
      mc1.find(make_tuple(0,0,0)),
      mc1.find(make_tuple(1,0,0)))==4);
  BOOST_TEST(
    std::distance(
      mc1.lower_bound(make_tuple(0,0)),
      mc1.upper_bound(make_tuple(1,0)))==6);

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
  BOOST_TEST(
    std::distance(
      mc1.lower_bound(1),
      mc1.upper_bound(1))==4);
#endif

  ckey_t1 ck1;
  ckey_t1 ck2(ck1);
  ckey_t1 ck3(
    boost::make_tuple(
      BOOST_MULTI_INDEX_MEMBER(xyz,int,x)(),
      BOOST_MULTI_INDEX_MEMBER(xyz,int,y)(),
      BOOST_MULTI_INDEX_MEMBER(xyz,int,z)()));
  ckey_t1 ck4(get<0>(ck1.key_extractors()));

  ck3=ck3; /* prevent unused var */

  get<2>(ck4.key_extractors())=
    get<2>(ck2.key_extractors());

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),ck2(xyz(0,0,0))));
  BOOST_TEST(is_less   (ck1(xyz(0,0,1)),ck2(xyz(0,1,0))));
  BOOST_TEST(is_greater(ck1(xyz(0,0,1)),ck2(xyz(0,0,0))));

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0)));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(1)));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(-1)));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0)));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,1)));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,-1)));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0,0)));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,0,1)));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,0,-1)));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0,0,1)));

  typedef composite_key_result_less<ckey_t1::result_type>     ckey_comp_t1;
  typedef composite_key_result_equal_to<ckey_t1::result_type> ckey_eq_t1;

  ckey_comp_t1 cp1;
  ckey_eq_t1   eq1;

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),ck2(xyz(0,0,0)),cp1,eq1));
  BOOST_TEST(is_less   (ck1(xyz(0,0,1)),ck2(xyz(0,1,0)),cp1,eq1));
  BOOST_TEST(is_greater(ck1(xyz(0,0,1)),ck2(xyz(0,0,0)),cp1,eq1));

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0),cp1));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(1),cp1));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(-1),cp1));

#if !defined(BOOST_NO_FUNCTION_TEMPLATE_ORDERING)
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),0,cp1));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),1,cp1));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),-1,cp1));
#endif

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0),cp1));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,1),cp1));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,-1),cp1));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0,0),cp1,eq1));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,0,1),cp1,eq1));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,0,-1),cp1,eq1));

  typedef composite_key_result_greater<ckey_t1::result_type> ckey_comp_t2;

  ckey_comp_t2 cp2;

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),ck2(xyz(0,0,0)),cp2));
  BOOST_TEST(is_greater(ck1(xyz(0,0,1)),ck2(xyz(0,1,0)),cp2));
  BOOST_TEST(is_less   (ck1(xyz(0,0,1)),ck2(xyz(0,0,0)),cp2));

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0),cp2));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(1),cp2));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(-1),cp2));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0),cp2));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,1),cp2));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,-1),cp2));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0,0),cp2));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,0,1),cp2));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,0,-1),cp2));

  typedef composite_key_equal_to<
    modulo_equal,
    modulo_equal,
    std::equal_to<int>,
    std::equal_to<int>
  > ckey_eq_t2;

  ckey_eq_t2 eq2(
    boost::make_tuple(
      modulo_equal(2),
      modulo_equal(3),
      std::equal_to<int>(),
      std::equal_to<int>()));
  ckey_eq_t2 eq3(eq2);
  ckey_eq_t2 eq4(
    get<0>(eq3.key_eqs()),
    get<1>(eq3.key_eqs()));

  eq3=eq4; /* prevent unused var */
  eq4=eq3; /* prevent unused var */

  BOOST_TEST( eq2(ck1(xyz(0,0,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(0,1,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(0,2,0)),ck1(xyz(0,0,0))));
  BOOST_TEST( eq2(ck1(xyz(0,3,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(1,0,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(1,1,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(1,2,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(1,3,0)),ck1(xyz(0,0,0))));
  BOOST_TEST( eq2(ck1(xyz(2,0,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(2,1,0)),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(2,2,0)),ck1(xyz(0,0,0))));
  BOOST_TEST( eq2(ck1(xyz(2,3,0)),ck1(xyz(0,0,0))));

  BOOST_TEST( eq2(make_tuple(0,0,0),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(0,1,0))  ,make_tuple(0,0,0)));
  BOOST_TEST(!eq2(make_tuple(0,2,0),ck1(xyz(0,0,0))));
  BOOST_TEST( eq2(ck1(xyz(0,3,0))  ,make_tuple(0,0,0)));
  BOOST_TEST(!eq2(make_tuple(1,0,0),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(1,1,0))  ,make_tuple(0,0,0)));
  BOOST_TEST(!eq2(make_tuple(1,2,0),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(1,3,0))  ,make_tuple(0,0,0)));
  BOOST_TEST( eq2(make_tuple(2,0,0),ck1(xyz(0,0,0))));
  BOOST_TEST(!eq2(ck1(xyz(2,1,0))  ,make_tuple(0,0,0)));
  BOOST_TEST(!eq2(make_tuple(2,2,0),ck1(xyz(0,0,0))));
  BOOST_TEST( eq2(ck1(xyz(2,3,0))  ,make_tuple(0,0,0)));

  typedef composite_key_compare<
    std::less<int>,
    std::greater<int>, /* order reversed */
    std::less<int>
  > ckey_comp_t3;

  ckey_comp_t3 cp3;
  ckey_comp_t3 cp4(cp3);
  ckey_comp_t3 cp5(
    boost::make_tuple(
      std::less<int>(),
      std::greater<int>(),
      std::less<int>()));
  ckey_comp_t3 cp6(get<0>(cp3.key_comps()));

  cp4=cp5; /* prevent unused var */
  cp5=cp6; /* prevent unused var */
  cp6=cp4; /* prevent unused var */

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),ck2(xyz(0,0,0)),cp3));
  BOOST_TEST(is_greater(ck1(xyz(0,0,1)),ck2(xyz(0,1,0)),cp3));
  BOOST_TEST(is_greater(ck1(xyz(0,0,1)),ck2(xyz(0,0,0)),cp3));

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0),cp3));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(1),cp3));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(-1),cp3));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0),cp3));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,-1),cp3));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,1),cp3));
  BOOST_TEST(is_equiv  (ck1(xyz(0,0,0)),make_tuple(0,0,0),cp3));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),make_tuple(0,0,1),cp3));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),make_tuple(0,0,-1),cp3));

  typedef composite_key<
    xyz,
    BOOST_MULTI_INDEX_MEMBER(xyz,int,y), /* members reversed */
    BOOST_MULTI_INDEX_MEMBER(xyz,int,x)
  > ckey_t2;

  ckey_t2 ck5;

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,1)),ck5(xyz(0,0,0))));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),ck5(xyz(-1,1,0))));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),ck5(xyz(1,-1,0))));

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,1)),ck5(xyz(0,0,0)),cp1));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),ck5(xyz(-1,1,0)),cp1));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),ck5(xyz(1,-1,0)),cp1));

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,1)),ck5(xyz(0,0,0)),cp2));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),ck5(xyz(-1,1,0)),cp2));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),ck5(xyz(1,-1,0)),cp2));

  BOOST_TEST(is_equiv  (ck1(xyz(0,0,1)),ck5(xyz(0,0,0)),cp3));
  BOOST_TEST(is_less   (ck1(xyz(0,0,0)),ck5(xyz(-1,1,0)),cp3));
  BOOST_TEST(is_greater(ck1(xyz(0,0,0)),ck5(xyz(1,-1,0)),cp3));

  typedef multi_index_container<
    xyz,
    indexed_by<
      hashed_unique<
        ckey_t1
#if defined(BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION)
        ,composite_key_result_hash<ckey_t1::result_type>
        ,composite_key_result_equal_to<ckey_t1::result_type>
#endif
      >
    >
  > indexed_t2;

  indexed_t2 mc2;
  mc2.insert(xyz(0,0,0));
  mc2.insert(xyz(0,0,1));
  mc2.insert(xyz(0,1,0));
  mc2.insert(xyz(0,1,1));
  mc2.insert(xyz(1,0,0));
  mc2.insert(xyz(1,0,1));
  mc2.insert(xyz(1,1,0));
  mc2.insert(xyz(1,1,1));
  mc2.insert(xyz(0,0,0));
  mc2.insert(xyz(0,0,1));
  mc2.insert(xyz(0,1,0));
  mc2.insert(xyz(0,1,1));
  mc2.insert(xyz(1,0,0));
  mc2.insert(xyz(1,0,1));
  mc2.insert(xyz(1,1,0));
  mc2.insert(xyz(1,1,1));

  BOOST_TEST(mc2.size()==8);
  BOOST_TEST(mc2.find(make_tuple(0,0,1))->z==1);
  BOOST_TEST(ck1(*(mc2.find(make_tuple(1,0,1))))==make_tuple(1,0,1));

  typedef composite_key<
    xystr,
    BOOST_MULTI_INDEX_MEMBER(xystr,std::string,str),
    BOOST_MULTI_INDEX_MEMBER(xystr,int,x),
    BOOST_MULTI_INDEX_MEMBER(xystr,int,y)
  > ckey_t3;

  ckey_t3 ck6;

  typedef composite_key_hash<
    boost::hash<std::string>,
    boost::hash<int>,
    boost::hash<int>
  > ckey_hash_t;

  ckey_hash_t ch1;
  ckey_hash_t ch2(ch1);
  ckey_hash_t ch3(
    boost::make_tuple(
      boost::hash<std::string>(),
      boost::hash<int>(),
      boost::hash<int>()));
  ckey_hash_t ch4(get<0>(ch1.key_hash_functions()));

  ch2=ch3; /* prevent unused var */
  ch3=ch4; /* prevent unused var */
  ch4=ch2; /* prevent unused var */

  BOOST_TEST(
    ch1(ck6(xystr(0,0,"hello")))==
    ch1(boost::make_tuple(std::string("hello"),0,0)));
  BOOST_TEST(
    ch1(ck6(xystr(4,5,"world")))==
    ch1(boost::make_tuple(std::string("world"),4,5)));

  typedef boost::hash<composite_key_result<ckey_t3> > ckeyres_hash_t;

  ckeyres_hash_t crh;

  BOOST_TEST(
    ch1(ck6(xystr(0,0,"hello")))==crh(ck6(xystr(0,0,"hello"))));
  BOOST_TEST(
    ch1(ck6(xystr(4,5,"world")))==crh(ck6(xystr(4,5,"world"))));
}
