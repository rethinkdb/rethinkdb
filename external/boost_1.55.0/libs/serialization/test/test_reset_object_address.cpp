/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// test_reset_object_address.cpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <sstream>
#include <cassert>
#include <cstdlib> // for rand()
#include <cstddef> // size_t

#include <boost/config.hpp>
#if defined(BOOST_NO_STDC_NAMESPACE)
namespace std{
    using ::rand; 
    using ::size_t;
}
#endif

#include "test_tools.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/polymorphic_text_oarchive.hpp>
#include <boost/archive/polymorphic_text_iarchive.hpp>

#include <boost/serialization/list.hpp>
#include <boost/serialization/access.hpp>

// Someday, maybe all tests will be converted to the unit test framework.
// but for now use the text execution monitor to be consistent with all
// the other tests.

// simple test of untracked value
#include "A.hpp"
#include "A.ipp"

void test1(){
    std::stringstream ss;
    const A a;
    {
        boost::archive::text_oarchive oa(ss);
        oa << a;
    }
    A a1;
    {
        boost::archive::text_iarchive ia(ss);
        // load to temporary
        A a2;
        ia >> a2;
        BOOST_CHECK_EQUAL(a, a2);
        // move to final destination
        a1 = a2;
        ia.reset_object_address(& a1, & a2);
    }
    BOOST_CHECK_EQUAL(a, a1);
}

// simple test of tracked value
class B {
    friend class boost::serialization::access;
    int m_i;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /*file_version*/){
        ar & m_i;
    }
public:
    bool operator==(const B & rhs) const {
        return m_i == rhs.m_i;
    }
    B() :
        m_i(std::rand())
    {}
};

//BOOST_TEST_DONT_PRINT_LOG_VALUE( B )

void test2(){
    std::stringstream ss;
    B const b;
    B const * const b_ptr = & b;
    BOOST_CHECK_EQUAL(& b, b_ptr);
    {
        boost::archive::text_oarchive oa(ss);
        oa << b;
        oa << b_ptr;
    }
    B b1;
    B * b1_ptr;
    {
        boost::archive::text_iarchive ia(ss);
        // load to temporary
        B b2;
        ia >> b2;
        BOOST_CHECK_EQUAL(b, b2);
        // move to final destination
        b1 = b2;
        ia.reset_object_address(& b1, & b2);
        ia >> b1_ptr;
    }
    BOOST_CHECK_EQUAL(b, b1);
    BOOST_CHECK_EQUAL(& b1, b1_ptr);
}

// check that nested member values are properly moved
class D {
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /*file_version*/){
        ar & m_b;
    }
public:
    B m_b;
    bool operator==(const D & rhs) const {
        return m_b == rhs.m_b;
    }
    D(){}
};

//BOOST_TEST_DONT_PRINT_LOG_VALUE( D )

void test3(){
    std::stringstream ss;
    D const d;
    B const * const b_ptr = & d.m_b;
    {
        boost::archive::text_oarchive oa(ss);
        oa << d;
        oa << b_ptr;
    }
    D d1;
    B * b1_ptr;
    {
        boost::archive::text_iarchive ia(ss);
        D d2;
        ia >> d2;
        d1 = d2;
        ia.reset_object_address(& d1, & d2);
        ia >> b1_ptr;
    }
    BOOST_CHECK_EQUAL(d, d1);
    BOOST_CHECK_EQUAL(* b_ptr, * b1_ptr);
}

// check that data pointed to by pointer members is NOT moved
class E {
    int m_i;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /*file_version*/){
        ar & m_i;
    }
public:
    bool operator==(const E &rhs) const {
        return m_i == rhs.m_i;
    }
    E() :
        m_i(std::rand())
    {}
    E(const E & rhs) :
        m_i(rhs.m_i)
    {}
};
//BOOST_TEST_DONT_PRINT_LOG_VALUE( E )

// check that moves don't move stuff pointed to
class F {
    friend class boost::serialization::access;
    E * m_eptr;
    template<class Archive>
    void serialize(Archive &ar, const unsigned int /*file_version*/){
        ar & m_eptr;
    }
public:
    bool operator==(const F &rhs) const {
        return *m_eptr == *rhs.m_eptr;
    }
    F & operator=(const F & rhs) {
        * m_eptr = * rhs.m_eptr;
        return *this;
    }
    F(){
        m_eptr = new E;
    }
    F(const F & rhs){
        *this = rhs;
    }
    ~F(){
        delete m_eptr;
    }
};

//BOOST_TEST_DONT_PRINT_LOG_VALUE( F )

void test4(){
    std::stringstream ss;
    const F f;
    {
        boost::archive::text_oarchive oa(ss);
        oa << f;
    }
    F f1;
    {
        boost::archive::text_iarchive ia(ss);
        F f2;
        ia >> f2;
        f1 = f2;
        ia.reset_object_address(& f1, & f2);
    }
    BOOST_CHECK_EQUAL(f, f1);
}

// check that multiple moves keep track of the correct target
class G {
    friend class boost::serialization::access;
    A m_a1;
    A m_a2;
    A *m_pa2;
    template<class Archive>
    void save(Archive &ar, const unsigned int /*file_version*/) const {
        ar << m_a1;
        ar << m_a2;
        ar << m_pa2;
    }
    template<class Archive>
    void load(Archive &ar, const unsigned int /*file_version*/){
        A a; // temporary A
        ar >> a;
        m_a1 = a;
        ar.reset_object_address(& m_a1, & a);
        ar >> a;
        m_a2 = a;
        ar.reset_object_address(& m_a2, & a);
        ar & m_pa2;
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
public:
    bool operator==(const G &rhs) const {
        return 
            m_a1 == rhs.m_a1 
            && m_a2 == rhs.m_a2
            && *m_pa2 == *rhs.m_pa2;
    }
    G & operator=(const G & rhs) {
        m_a1 = rhs.m_a1;
        m_a2 = rhs.m_a2;
        m_pa2 = & m_a2;
        return *this;
    }
    G(){
        m_pa2 = & m_a2;
    }
    G(const G & rhs){
        *this = rhs;
    }
    ~G(){}
};

//BOOST_TEST_DONT_PRINT_LOG_VALUE( G )

void test5(){
    std::stringstream ss;
    const G g;
    {
        boost::archive::text_oarchive oa(ss);
        oa << g;
    }
    G g1;
    {
        boost::archive::text_iarchive ia(ss);
        ia >> g1;
    }
    BOOST_CHECK_EQUAL(g, g1);
}

// joaquin's test - this tests the case where rest_object_address
// is applied to an item which in fact is not tracked so that 
// the call is in fact superfluous.
struct foo
{
  int x;

private:
  friend class boost::serialization::access;

  template<class Archive>
  void serialize(Archive &,const unsigned int)
  {
  }
};

struct bar
{
  foo  f[2];
  foo* pf[2];

private:
  friend class boost::serialization::access;
  BOOST_SERIALIZATION_SPLIT_MEMBER()

  template<class Archive>
  void save(Archive& ar,const unsigned int)const
  {
    for(int i=0;i<2;++i){
      ar<<f[i].x;
      ar<<f[i];
    }
    for(int j=0;j<2;++j){
      ar<<pf[j];
    }
  }

  template<class Archive>
  void load(Archive& ar,const unsigned int)
  {
    for(int i=0;i<2;++i){
      int x;
      ar>>x;
      f[i].x=x;
      ar.reset_object_address(&f[i].x,&x);
      ar>>f[i];
    }
    for(int j=0;j<2;++j){
      ar>>pf[j];
    }
  }
};

int test6()
{
  bar b;
  b.f[0].x=0;
  b.f[1].x=1;
  b.pf[0]=&b.f[0];
  b.pf[1]=&b.f[1];

  std::ostringstream oss;
  {
    boost::archive::text_oarchive oa(oss);
    oa<<const_cast<const bar&>(b);
  }

  bar b1;
  b1.pf[0]=0;
  b1.pf[1]=0;

  std::istringstream iss(oss.str());
  boost::archive::text_iarchive ia(iss);
  ia>>b1;
  BOOST_CHECK(b1.pf[0]==&b1.f[0]&&b1.pf[1]==&b1.f[1]);

  return 0;
}

// test one of the collections
void test7(){
    std::stringstream ss;
    B const b;
    B const * const b_ptr = & b;
    BOOST_CHECK_EQUAL(& b, b_ptr);
    {
        std::list<const B *> l;
        l.push_back(b_ptr);
        boost::archive::text_oarchive oa(ss);
        oa << const_cast<const std::list<const B *> &>(l);
    }
    B b1;
    {
        std::list<B *> l;
        boost::archive::text_iarchive ia(ss);
        ia >> l;
        delete l.front(); // prevent memory leak
    }
}

// test one of the collections with polymorphic archive
void test8(){
    std::stringstream ss;
    B const b;
    B const * const b_ptr = & b;
    BOOST_CHECK_EQUAL(& b, b_ptr);
    {
        std::list<const B *> l;
        l.push_back(b_ptr);
        boost::archive::polymorphic_text_oarchive oa(ss);
        boost::archive::polymorphic_oarchive & poa = oa;
        poa << const_cast<const std::list<const B *> &>(l);
    }
    B b1;
    {
        std::list<B *> l;
        boost::archive::polymorphic_text_iarchive ia(ss);
        boost::archive::polymorphic_iarchive & pia = ia;
        pia >> l;
        delete l.front(); // prevent memory leak
    }
}

int test_main(int /* argc */, char * /* argv */[])
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();
    test7();
    test8();
    return EXIT_SUCCESS;
}
