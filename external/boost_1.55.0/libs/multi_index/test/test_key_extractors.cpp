/* Boost.MultiIndex test for key extractors.
 *
 * Copyright 2003-2013 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/multi_index for library home page.
 */

#include "test_key_extractors.hpp"

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/detail/lightweight_test.hpp>
#include "pre_multi_index.hpp"
#include <boost/multi_index/key_extractors.hpp>
#include <boost/ref.hpp>
#include <boost/scoped_ptr.hpp>
#include <list>
#include <memory>

using namespace boost::multi_index;
using namespace boost::tuples;

struct test_class
{
  int       int_member;
  const int int_cmember;

  bool bool_mem_fun_const()const{return true;}
  bool bool_mem_fun(){return false;}

  static bool bool_global_fun(test_class){return true;}
  static bool bool_global_fun_const_ref(const test_class&){return false;}
  static bool bool_global_fun_ref(test_class&){return true;}

  test_class(int i=0):int_member(i),int_cmember(i){}
  test_class(int i,int j):int_member(i),int_cmember(j){}

  test_class& operator=(const test_class& x)
  {
    int_member=x.int_member;
    return *this;
  }

  bool operator<(const test_class& x)const
  {
    if(int_member<x.int_member)return true;
    if(x.int_member<int_member)return false;
    return int_cmember<x.int_cmember;
  }

  bool operator==(const test_class& x)const
  {
    return int_member==x.int_member&&int_cmember==x.int_cmember;
  }
};

struct test_derived_class:test_class
{
  test_derived_class(int i=0):test_class(i){}
  test_derived_class(int i,int j):test_class(i,j){}
};

BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(test_class)
BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(test_derived_class)

typedef identity<test_class>                                       idn;
typedef identity<const test_class>                                 cidn;
typedef BOOST_MULTI_INDEX_MEMBER(test_class,int,int_member)        key_m;
typedef BOOST_MULTI_INDEX_MEMBER(test_class,const int,int_member)  ckey_m;
typedef BOOST_MULTI_INDEX_MEMBER(test_class,const int,int_cmember) key_cm;
typedef BOOST_MULTI_INDEX_CONST_MEM_FUN(
          test_class,bool,bool_mem_fun_const)                      key_cmf;
typedef BOOST_MULTI_INDEX_MEM_FUN(test_class,bool,bool_mem_fun)    key_mf;
typedef global_fun<test_class,bool,&test_class::bool_global_fun>   key_gf;
typedef global_fun<
          const test_class&,bool,
          &test_class::bool_global_fun_const_ref
        >                                                          key_gcrf;
typedef global_fun<
          test_class&,bool,
          &test_class::bool_global_fun_ref
        >                                                          key_grf;
typedef composite_key<
          test_class,
          idn,
          key_m,
          key_cm,
          key_cmf
        >                                                          compkey;
typedef composite_key<
          test_class,
          cidn,
          ckey_m
        >                                                          ccompkey;
typedef composite_key<
          boost::reference_wrapper<test_class>,
          key_mf
        >                                                          ccompw_key;

#if !defined(BOOST_NO_SFINAE)
/* testcases for problems with non-copyable classes reported at
 * http://lists.boost.org/Archives/boost/2006/04/103065.php
 */

struct test_nc_class
{
  int       int_member;
  const int int_cmember;

  bool bool_mem_fun_const()const{return true;}
  bool bool_mem_fun(){return false;}

  static bool bool_global_fun_const_ref(const test_nc_class&){return false;}
  static bool bool_global_fun_ref(test_nc_class&){return true;}

  test_nc_class(int i=0):int_member(i),int_cmember(i){}
  test_nc_class(int i,int j):int_member(i),int_cmember(j){}

  bool operator==(const test_nc_class& x)const
  {
    return int_member==x.int_member&&int_cmember==x.int_cmember;
  }

private:
  test_nc_class(const test_nc_class&);
  test_nc_class& operator=(const test_nc_class&);
};

struct test_nc_derived_class:test_nc_class
{
  test_nc_derived_class(int i=0):test_nc_class(i){}
  test_nc_derived_class(int i,int j):test_nc_class(i,j){}
};

BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(test_nc_class)
BOOST_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(test_nc_derived_class)

typedef identity<test_nc_class>                                nc_idn;
typedef identity<const test_nc_class>                          nc_cidn;
typedef BOOST_MULTI_INDEX_MEMBER(test_nc_class,int,int_member) nc_key_m;
typedef BOOST_MULTI_INDEX_MEMBER(
          test_nc_class,const int,int_member)                  nc_ckey_m;
typedef BOOST_MULTI_INDEX_CONST_MEM_FUN(
          test_nc_class,bool,bool_mem_fun_const)               nc_key_cmf;
typedef BOOST_MULTI_INDEX_MEM_FUN(
          test_nc_class,bool,bool_mem_fun)                     nc_key_mf;
typedef global_fun<
          const test_nc_class&,bool,
          &test_nc_class::bool_global_fun_const_ref
        >                                                      nc_key_gcrf;
typedef global_fun<
          test_nc_class&,bool,
          &test_nc_class::bool_global_fun_ref
        >                                                      nc_key_grf;
typedef composite_key<
          test_nc_class,
          nc_idn,
          nc_key_m,
          nc_ckey_m,
          nc_key_cmf
        >                                                      nc_compkey;
#endif

void test_key_extractors()
{
  idn        id;
  cidn       cid;
  key_m      k_m;
  ckey_m     ck_m;
  key_cm     k_cm;
  key_cmf    k_cmf;
  key_mf     k_mf;
  key_gf     k_gf;
  key_gcrf   k_gcrf;
  key_grf    k_grf;
  compkey    cmpk;
  ccompkey   ccmpk;
  ccompw_key ccmpk_w;

  test_derived_class                         td(-1,0);
  const test_derived_class&                  ctdr=td;

  test_class&                                tr=td;
  const test_class&                          ctr=tr;

  test_derived_class*                        tdp=&td;
  const test_derived_class*                  ctdp=&ctdr;

  test_class*                                tp=&tr;
  const test_class*                          ctp=&tr;

  test_class**                               tpp=&tp;
  const test_class**                         ctpp=&ctp;

  std::auto_ptr<test_class*>                 tap(new test_class*(tp));
  std::auto_ptr<const test_class*>           ctap(new const test_class*(ctp));

  boost::reference_wrapper<test_class>       tw(tr);
  boost::reference_wrapper<const test_class> ctw(tr);

  id(tr).int_member=0;
  BOOST_TEST(id(tr).int_member==0);
  BOOST_TEST(cid(tr).int_member==0);
  BOOST_TEST(k_m(tr)==0);
  BOOST_TEST(ck_m(tr)==0);
  BOOST_TEST(cmpk(tr)==make_tuple(test_class(0,0),0,0,true));
  BOOST_TEST(ccmpk(tr)==make_tuple(test_class(0,0),0));
  BOOST_TEST(id(ctr).int_member==0);
  BOOST_TEST(cid(ctr).int_member==0);
  BOOST_TEST(k_m(ctr)==0);
  BOOST_TEST(ck_m(ctr)==0);
  BOOST_TEST(cmpk(ctr)==make_tuple(test_class(0,0),0,0,true));
  BOOST_TEST(ccmpk(ctr)==make_tuple(test_class(0,0),0));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(id(td).int_member==0);
  BOOST_TEST(cid(td).int_member==0);
  BOOST_TEST(k_m(td)==0);
  BOOST_TEST(ck_m(td)==0);
  BOOST_TEST(cmpk(td)==make_tuple(test_class(0,0),0,0,true));
  BOOST_TEST(ccmpk(td)==make_tuple(test_class(0,0),0));
  BOOST_TEST(id(ctdr).int_member==0);
  BOOST_TEST(cid(ctdr).int_member==0);
  BOOST_TEST(k_m(ctdr)==0);
  BOOST_TEST(ck_m(ctdr)==0);
  BOOST_TEST(cmpk(ctdr)==make_tuple(test_class(0,0),0,0,true));
  BOOST_TEST(ccmpk(ctdr)==make_tuple(test_class(0,0),0));
#endif

  k_m(tr)=1;
  BOOST_TEST(id(tp).int_member==1);
  BOOST_TEST(cid(tp).int_member==1);
  BOOST_TEST(k_m(tp)==1);
  BOOST_TEST(ck_m(tp)==1);
  BOOST_TEST(cmpk(tp)==make_tuple(test_class(1,0),1,0,true));
  BOOST_TEST(ccmpk(tp)==make_tuple(test_class(1,0),1));
  BOOST_TEST(cid(ctp).int_member==1);
  BOOST_TEST(ck_m(ctp)==1);
  BOOST_TEST(cmpk(ctp)==make_tuple(test_class(1,0),1,0,true));
  BOOST_TEST(ccmpk(ctp)==make_tuple(test_class(1,0),1));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(id(tdp).int_member==1);
  BOOST_TEST(cid(tdp).int_member==1);
  BOOST_TEST(k_m(tdp)==1);
  BOOST_TEST(ck_m(tdp)==1);
  BOOST_TEST(cmpk(tdp)==make_tuple(test_class(1,0),1,0,true));
  BOOST_TEST(ccmpk(tdp)==make_tuple(test_class(1,0),1));
  BOOST_TEST(cid(ctdp).int_member==1);
  BOOST_TEST(ck_m(ctdp)==1);
  BOOST_TEST(cmpk(ctdp)==make_tuple(test_class(1,0),1,0,true));
  BOOST_TEST(ccmpk(ctdp)==make_tuple(test_class(1,0),1));
#endif

  k_m(tp)=2;
  BOOST_TEST(id(tpp).int_member==2);
  BOOST_TEST(cid(tpp).int_member==2);
  BOOST_TEST(k_m(tpp)==2);
  BOOST_TEST(ck_m(tpp)==2);
  BOOST_TEST(cmpk(tpp)==make_tuple(test_class(2,0),2,0,true));
  BOOST_TEST(ccmpk(tpp)==make_tuple(test_class(2,0),2));
  BOOST_TEST(cid(ctpp).int_member==2);
  BOOST_TEST(ck_m(ctpp)==2);
  BOOST_TEST(cmpk(ctpp)==make_tuple(test_class(2,0),2,0,true));
  BOOST_TEST(ccmpk(ctpp)==make_tuple(test_class(2,0),2));

  k_m(tpp)=3;
  BOOST_TEST(id(tap).int_member==3);
  BOOST_TEST(cid(tap).int_member==3);
  BOOST_TEST(k_m(tap)==3);
  BOOST_TEST(ck_m(tap)==3);
  BOOST_TEST(cmpk(tap)==make_tuple(test_class(3,0),3,0,true));
  BOOST_TEST(ccmpk(tap)==make_tuple(test_class(3,0),3));
  BOOST_TEST(cid(ctap).int_member==3);
  BOOST_TEST(ck_m(ctap)==3);
  BOOST_TEST(cmpk(ctap)==make_tuple(test_class(3,0),3,0,true));
  BOOST_TEST(ccmpk(ctap)==make_tuple(test_class(3,0),3));

  k_m(tap)=4;
  BOOST_TEST(id(tw).int_member==4);
  BOOST_TEST(cid(tw).int_member==4);
  BOOST_TEST(k_m(tw)==4);
  BOOST_TEST(ck_m(tw)==4);
  BOOST_TEST(cmpk(tw)==make_tuple(test_class(4,0),4,0,true));
  BOOST_TEST(ccmpk(tw)==make_tuple(test_class(4,0),4));

  k_m(tw)=5;
  BOOST_TEST(id(ctw).int_member==5);
  BOOST_TEST(cid(ctw).int_member==5);
  BOOST_TEST(k_m(ctw)==5);
  BOOST_TEST(ck_m(ctw)==5);
  BOOST_TEST(cmpk(ctw)==make_tuple(test_class(5,0),5,0,true));
  BOOST_TEST(ccmpk(ctw)==make_tuple(test_class(5,0),5));

  BOOST_TEST(k_cm(tr)==0);
  BOOST_TEST(k_cm(ctr)==0);

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_cm(td)==0);
  BOOST_TEST(k_cm(ctdr)==0);
#endif

  BOOST_TEST(k_cm(tp)==0);
  BOOST_TEST(k_cm(ctp)==0);

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_cm(tdp)==0);
  BOOST_TEST(k_cm(ctdp)==0);
#endif
  
  BOOST_TEST(k_cm(tpp)==0);
  BOOST_TEST(k_cm(ctpp)==0);
  BOOST_TEST(k_cm(tap)==0);
  BOOST_TEST(k_cm(ctap)==0);

  BOOST_TEST(k_cm(tw)==0);
  BOOST_TEST(k_cm(ctw)==0);

  BOOST_TEST(k_cmf(tr));
  BOOST_TEST(k_cmf(ctr));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_cmf(td));
  BOOST_TEST(k_cmf(ctdr));
#endif

  BOOST_TEST(k_cmf(tp));
  BOOST_TEST(k_cmf(ctp));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_cmf(tdp));
  BOOST_TEST(k_cmf(ctdp));
#endif

  BOOST_TEST(k_cmf(tpp));
  BOOST_TEST(k_cmf(ctpp));
  BOOST_TEST(k_cmf(tap));
  BOOST_TEST(k_cmf(ctap));

  BOOST_TEST(k_cmf(tw));
  BOOST_TEST(k_cmf(ctw));

  BOOST_TEST(!k_mf(tr));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(!k_mf(td));
#endif

  BOOST_TEST(!k_mf(tp));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(!k_mf(tdp));
#endif

  BOOST_TEST(!k_mf(tpp));
  BOOST_TEST(!k_mf(tap));
  BOOST_TEST(!k_mf(tw));

  BOOST_TEST(k_gf(tr));
  BOOST_TEST(k_gf(ctr));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_gf(td));
  BOOST_TEST(k_gf(ctdr));
#endif

  BOOST_TEST(k_gf(tp));
  BOOST_TEST(k_gf(ctp));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_gf(tdp));
  BOOST_TEST(k_gf(ctdp));
#endif

  BOOST_TEST(k_gf(tpp));
  BOOST_TEST(k_gf(ctpp));
  BOOST_TEST(k_gf(tap));
  BOOST_TEST(k_gf(ctap));

  BOOST_TEST(k_gf(tw));
  BOOST_TEST(k_gf(ctw));
  
  BOOST_TEST(!k_gcrf(tr));
  BOOST_TEST(!k_gcrf(ctr));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(!k_gcrf(td));
  BOOST_TEST(!k_gcrf(ctdr));
#endif

  BOOST_TEST(!k_gcrf(tp));
  BOOST_TEST(!k_gcrf(ctp));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(!k_gcrf(tdp));
  BOOST_TEST(!k_gcrf(ctdp));
#endif

  BOOST_TEST(!k_gcrf(tpp));
  BOOST_TEST(!k_gcrf(ctpp));
  BOOST_TEST(!k_gcrf(tap));
  BOOST_TEST(!k_gcrf(ctap));

  BOOST_TEST(!k_gcrf(tw));
  BOOST_TEST(!k_gcrf(ctw));

  BOOST_TEST(k_grf(tr));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_grf(td));
#endif

  BOOST_TEST(k_grf(tp));

#if !defined(BOOST_NO_SFINAE)
  BOOST_TEST(k_grf(tdp));
#endif

  BOOST_TEST(k_grf(tpp));
  BOOST_TEST(k_grf(tap));
  BOOST_TEST(k_grf(tw));

  BOOST_TEST(ccmpk_w(tw)==make_tuple(false));

#if !defined(BOOST_NO_SFINAE)
/* testcases for problems with non-copyable classes reported at
 * http://lists.boost.org/Archives/boost/2006/04/103065.php
 */

  nc_idn        nc_id;
  nc_cidn       nc_cid;
  nc_key_m      nc_k_m;
  nc_ckey_m     nc_ck_m;
  nc_key_cmf    nc_k_cmf;
  nc_key_mf     nc_k_mf;
  nc_key_gcrf   nc_k_gcrf;
  nc_key_grf    nc_k_grf;
  nc_compkey    nc_cmpk;

  test_nc_derived_class nc_td(-1,0);

  nc_id(nc_td).int_member=0;
  BOOST_TEST(nc_id(nc_td).int_member==0);
  BOOST_TEST(nc_cid(nc_td).int_member==0);

  nc_k_m(&nc_td)=1;
  BOOST_TEST(nc_k_m(&nc_td)==1);
  BOOST_TEST(nc_ck_m(&nc_td)==1);

  BOOST_TEST(nc_k_cmf(nc_td));
  BOOST_TEST(!nc_k_mf(nc_td));

  BOOST_TEST(!nc_k_gcrf(nc_td));
  BOOST_TEST(nc_k_grf(nc_td));

  test_nc_class nc_t(1,0);
  BOOST_TEST(nc_cmpk(nc_td)==make_tuple(boost::cref(nc_t),1,1,true));
#endif
  
  std::list<test_class> tl;
  for(int i=0;i<20;++i)tl.push_back(test_class(i));

  int j=0;
  for(std::list<test_class>::iterator it=tl.begin();it!=tl.end();++it){
    BOOST_TEST(k_m(it)==j);
    BOOST_TEST(k_cm(it)==j);
    BOOST_TEST(k_cmf(it));
    BOOST_TEST(!k_mf(it));
    BOOST_TEST(k_gf(it));
    BOOST_TEST(!k_gcrf(it));
    BOOST_TEST(k_grf(it));
    BOOST_TEST(cmpk(it)==make_tuple(test_class(j),j,j,true));
    BOOST_TEST(ccmpk(it)==make_tuple(test_class(j),j));
    ++j;
  }
}
