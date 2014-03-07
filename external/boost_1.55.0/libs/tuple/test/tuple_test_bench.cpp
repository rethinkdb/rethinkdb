// Copyright (C) 1999, 2000 Jaakko Jarvi (jaakko.jarvi@cs.utu.fi)
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// For more information, see http://www.boost.org

//  tuple_test_bench.cpp  --------------------------------

#define BOOST_INCLUDE_MAIN  // for testing, include rather than link
#include <boost/test/test_tools.hpp>    // see "Header Implementation Option"

#include "boost/tuple/tuple.hpp"

#include "boost/tuple/tuple_comparison.hpp"

#include "boost/type_traits/is_const.hpp"

#include "boost/ref.hpp"
#include <string>
#include <utility>

using namespace boost;

// ----------------------------------------------------------------------------
// helpers 
// ----------------------------------------------------------------------------

class A {}; 
class B {}; 
class C {};

// classes with different kinds of conversions
class AA {};
class BB : public AA {}; 
struct CC { CC() {} CC(const BB&) {} };
struct DD { operator CC() const { return CC(); }; };

// something to prevent warnings for unused variables
template<class T> void dummy(const T&) {}

// no public default constructor
class foo {
public:
  explicit foo(int v) : val(v) {}

  bool operator==(const foo& other) const  {
    return val == other.val;
  }

private:
  foo() {}
  int val;
};

// another class without a public default constructor
class no_def_constructor {
  no_def_constructor() {}
public:
  no_def_constructor(std::string) {}
};

// A non-copyable class 
class no_copy {
  no_copy(const no_copy&) {}
public:
  no_copy() {};
};


// ----------------------------------------------------------------------------
// Testing different element types --------------------------------------------
// ----------------------------------------------------------------------------


typedef tuple<int> t1;

typedef tuple<double&, const double&, const double, double*, const double*> t2;
typedef tuple<A, int(*)(char, int), C> t3;
typedef tuple<std::string, std::pair<A, B> > t4;
typedef tuple<A*, tuple<const A*, const B&, C>, bool, void*> t5;
typedef tuple<volatile int, const volatile char&, int(&)(float) > t6;

# if !defined(__BORLANDC__) || __BORLAND__ > 0x0551
typedef tuple<B(A::*)(C&), A&> t7;
#endif

// -----------------------------------------------------------------------
// -tuple construction tests ---------------------------------------------
// -----------------------------------------------------------------------


no_copy y;
tuple<no_copy&> x = tuple<no_copy&>(y); // ok

char cs[10];
tuple<char(&)[10]> v2(cs);  // ok

void
construction_test()
{

  // Note, the get function can be called without the tuples:: qualifier,
  // as it is lifted to namespace boost with a "using tuples::get" but
  // MSVC 6.0 just cannot find get without the namespace qualifier

  tuple<int> t1;
  BOOST_CHECK(get<0>(t1) == int());

  tuple<float> t2(5.5f);
  BOOST_CHECK(get<0>(t2) > 5.4f && get<0>(t2) < 5.6f);

  tuple<foo> t3(foo(12));
  BOOST_CHECK(get<0>(t3) == foo(12));

  tuple<double> t4(t2);
  BOOST_CHECK(get<0>(t4) > 5.4 && get<0>(t4) < 5.6);

  tuple<int, float> t5;
  BOOST_CHECK(get<0>(t5) == int());
  BOOST_CHECK(get<1>(t5) == float());

  tuple<int, float> t6(12, 5.5f);
  BOOST_CHECK(get<0>(t6) == 12);
  BOOST_CHECK(get<1>(t6) > 5.4f && get<1>(t6) < 5.6f);

  tuple<int, float> t7(t6);
  BOOST_CHECK(get<0>(t7) == 12);
  BOOST_CHECK(get<1>(t7) > 5.4f && get<1>(t7) < 5.6f);

  tuple<long, double> t8(t6);
  BOOST_CHECK(get<0>(t8) == 12);
  BOOST_CHECK(get<1>(t8) > 5.4f && get<1>(t8) < 5.6f);

  dummy(
    tuple<no_def_constructor, no_def_constructor, no_def_constructor>(
       std::string("Jaba"),   // ok, since the default
       std::string("Daba"),   // constructor is not used
       std::string("Doo")
    )
  );

// testing default values
  dummy(tuple<int, double>());
  dummy(tuple<int, double>(1));
  dummy(tuple<int, double>(1,3.14));


  //  dummy(tuple<double&>()); // should fail, not defaults for references
  //  dummy(tuple<const double&>()); // likewise

  double dd = 5;
  dummy(tuple<double&>(dd)); // ok

  dummy(tuple<const double&>(dd+3.14)); // ok, but dangerous

  //  dummy(tuple<double&>(dd+3.14)); // should fail,
  //                                  // temporary to non-const reference
}


// ----------------------------------------------------------------------------
// - testing element access ---------------------------------------------------
// ----------------------------------------------------------------------------

void element_access_test()
{
  double d = 2.7;
  A a;
  tuple<int, double&, const A&, int> t(1, d, a, 2);
  const tuple<int, double&, const A, int> ct = t;

  int i  = get<0>(t);
  int i2 = get<3>(t);

  BOOST_CHECK(i == 1 && i2 == 2);

  int j  = get<0>(ct);
  BOOST_CHECK(j == 1);
   
  get<0>(t) = 5;
  BOOST_CHECK(t.head == 5);
   
  //  get<0>(ct) = 5; // can't assign to const

  double e = get<1>(t);
  BOOST_CHECK(e > 2.69 && e < 2.71);
     
  get<1>(t) = 3.14+i;
  BOOST_CHECK(get<1>(t) > 4.13 && get<1>(t) < 4.15);

  //  get<4>(t) = A(); // can't assign to const
  //  dummy(get<5>(ct)); // illegal index

  ++get<0>(t);
  BOOST_CHECK(get<0>(t) == 6);

  BOOST_STATIC_ASSERT((boost::is_const<boost::tuples::element<0, tuple<int, float> >::type>::value != true));
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
  BOOST_STATIC_ASSERT((boost::is_const<boost::tuples::element<0, const tuple<int, float> >::type>::value));
#endif 

  BOOST_STATIC_ASSERT((boost::is_const<boost::tuples::element<1, tuple<int, float> >::type>::value != true));
#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
  BOOST_STATIC_ASSERT((boost::is_const<boost::tuples::element<1, const tuple<int, float> >::type>::value));
#endif 


  dummy(i); dummy(i2); dummy(j); dummy(e); // avoid warns for unused variables
}


// ----------------------------------------------------------------------------
// - copying tuples -----------------------------------------------------------
// ----------------------------------------------------------------------------



void
copy_test()
{
  tuple<int, char> t1(4, 'a');
  tuple<int, char> t2(5, 'b');
  t2 = t1;
  BOOST_CHECK(get<0>(t1) == get<0>(t2));
  BOOST_CHECK(get<1>(t1) == get<1>(t2));

  tuple<long, std::string> t3(2, "a");
  t3 = t1;
  BOOST_CHECK((double)get<0>(t1) == get<0>(t3));
  BOOST_CHECK(get<1>(t1) == get<1>(t3)[0]);

// testing copy and assignment with implicit conversions between elements
// testing tie

  tuple<char, BB*, BB, DD> t;
  tuple<int, AA*, CC, CC> a(t);
  a = t;

  int i; char c; double d;
  tie(i, c, d) = make_tuple(1, 'a', 5.5);
  
  BOOST_CHECK(i==1);
  BOOST_CHECK(c=='a');
  BOOST_CHECK(d>5.4 && d<5.6);
}

void
mutate_test()
{
  tuple<int, float, bool, foo> t1(5, 12.2f, true, foo(4));
  get<0>(t1) = 6;
  get<1>(t1) = 2.2f;
  get<2>(t1) = false;
  get<3>(t1) = foo(5);

  BOOST_CHECK(get<0>(t1) == 6);
  BOOST_CHECK(get<1>(t1) > 2.1f && get<1>(t1) < 2.3f);
  BOOST_CHECK(get<2>(t1) == false);
  BOOST_CHECK(get<3>(t1) == foo(5));
}

// ----------------------------------------------------------------------------
// make_tuple tests -----------------------------------------------------------
// ----------------------------------------------------------------------------

void
make_tuple_test()
{
  tuple<int, char> t1 = make_tuple(5, 'a');
  BOOST_CHECK(get<0>(t1) == 5);
  BOOST_CHECK(get<1>(t1) == 'a');

  tuple<int, std::string> t2;
  t2 = boost::make_tuple((short int)2, std::string("Hi"));
  BOOST_CHECK(get<0>(t2) == 2);
  BOOST_CHECK(get<1>(t2) == "Hi");


    A a = A(); B b;
    const A ca = a;
    make_tuple(boost::cref(a), b);
    make_tuple(boost::ref(a), b);
    make_tuple(boost::ref(a), boost::cref(b));

    make_tuple(boost::ref(ca));
     
// the result of make_tuple is assignable:
   BOOST_CHECK(make_tuple(2, 4, 6) == 
             (make_tuple(1, 2, 3) = make_tuple(2, 4, 6)));

#ifndef BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION
    make_tuple("Donald", "Daisy"); // should work;
#endif 
    //    std::make_pair("Doesn't","Work"); // fails

// You can store a reference to a function in a tuple
    tuple<void(&)()> adf(make_tuple_test);

    dummy(adf); // avoid warning for unused variable
 
// But make_tuple doesn't work 
// with function references, since it creates a const qualified function type

//   make_tuple(make_tuple_test);
  
// With function pointers, make_tuple works just fine

#if !defined(__BORLANDC__) || __BORLAND__ > 0x0551
   make_tuple(&make_tuple_test);
#endif
      
// NOTE:
//
// wrapping it the function reference with ref helps on gcc 2.95.2.
// on edg 2.43. it results in a catastrophic error?

// make_tuple(ref(foo3));

// It seems that edg can't use implicitly the ref's conversion operator, e.g.:
// typedef void (&func_t) (void);
// func_t fref = static_cast<func_t>(ref(make_tuple_test)); // works fine 
// func_t fref = ref(make_tuple_test);                        // error

// This is probably not a very common situation, so currently
// I don't know how which compiler is right (JJ)
}

void
tie_test()
{
  int a;
  char b;
  foo c(5);

  tie(a, b, c) = make_tuple(2, 'a', foo(3));
  BOOST_CHECK(a == 2);
  BOOST_CHECK(b == 'a');
  BOOST_CHECK(c == foo(3));

  tie(a, tuples::ignore, c) = make_tuple((short int)5, false, foo(5));
  BOOST_CHECK(a == 5);
  BOOST_CHECK(b == 'a');
  BOOST_CHECK(c == foo(5));

// testing assignment from std::pair
   int i, j; 
   tie (i, j) = std::make_pair(1, 2);
   BOOST_CHECK(i == 1 && j == 2);

   tuple<int, int, float> ta;
#ifdef E11
   ta = std::make_pair(1, 2); // should fail, tuple is of length 3, not 2
#endif

   dummy(ta);
}


// ----------------------------------------------------------------------------
// - testing tuple equality   -------------------------------------------------
// ----------------------------------------------------------------------------

void
equality_test()
{
  tuple<int, char> t1(5, 'a');
  tuple<int, char> t2(5, 'a');
  BOOST_CHECK(t1 == t2);

  tuple<int, char> t3(5, 'b');
  tuple<int, char> t4(2, 'a');
  BOOST_CHECK(t1 != t3);
  BOOST_CHECK(t1 != t4);
  BOOST_CHECK(!(t1 != t2));
}


// ----------------------------------------------------------------------------
// - testing tuple comparisons  -----------------------------------------------
// ----------------------------------------------------------------------------

void
ordering_test()
{
  tuple<int, float> t1(4, 3.3f);
  tuple<short, float> t2(5, 3.3f);
  tuple<long, double> t3(5, 4.4);
  BOOST_CHECK(t1 < t2);
  BOOST_CHECK(t1 <= t2);
  BOOST_CHECK(t2 > t1);
  BOOST_CHECK(t2 >= t1);
  BOOST_CHECK(t2 < t3);
  BOOST_CHECK(t2 <= t3);
  BOOST_CHECK(t3 > t2);
  BOOST_CHECK(t3 >= t2);

}


// ----------------------------------------------------------------------------
// - testing cons lists -------------------------------------------------------
// ----------------------------------------------------------------------------
void cons_test()
{
  using tuples::cons;
  using tuples::null_type;

  cons<volatile float, null_type> a(1, null_type());
  cons<const int, cons<volatile float, null_type> > b(2,a);
  int i = 3;
  cons<int&, cons<const int, cons<volatile float, null_type> > > c(i, b);
  BOOST_CHECK(make_tuple(3,2,1)==c);

  cons<char, cons<int, cons<float, null_type> > > x;
  dummy(x);
}

// ----------------------------------------------------------------------------
// - testing const tuples -----------------------------------------------------
// ----------------------------------------------------------------------------
void const_tuple_test()
{
  const tuple<int, float> t1(5, 3.3f);
  BOOST_CHECK(get<0>(t1) == 5);
  BOOST_CHECK(get<1>(t1) == 3.3f);
}

// ----------------------------------------------------------------------------
// - testing length -----------------------------------------------------------
// ----------------------------------------------------------------------------
void tuple_length_test()
{
  typedef tuple<int, float, double> t1;
  using tuples::cons;
  typedef cons<int, cons< float, cons <double, tuples::null_type> > > t1_cons;
  typedef tuple<> t2;
  typedef tuples::null_type t3;  

  BOOST_STATIC_ASSERT(tuples::length<t1>::value == 3);
  BOOST_STATIC_ASSERT(tuples::length<t1_cons>::value == 3);
  BOOST_STATIC_ASSERT(tuples::length<t2>::value == 0);
  BOOST_STATIC_ASSERT(tuples::length<t3>::value == 0);

}

// ----------------------------------------------------------------------------
// - testing swap -----------------------------------------------------------
// ----------------------------------------------------------------------------
void tuple_swap_test()
{
  tuple<int, float, double> t1(1, 2.0f, 3.0), t2(4, 5.0f, 6.0);
  swap(t1, t2);
  BOOST_CHECK(get<0>(t1) == 4);
  BOOST_CHECK(get<1>(t1) == 5.0f);
  BOOST_CHECK(get<2>(t1) == 6.0);
  BOOST_CHECK(get<0>(t2) == 1);
  BOOST_CHECK(get<1>(t2) == 2.0f);
  BOOST_CHECK(get<2>(t2) == 3.0);

  int i = 1,j = 2;
  boost::tuple<int&> t3(i), t4(j);
  swap(t3, t4);
  BOOST_CHECK(i == 2);
  BOOST_CHECK(j == 1);
}



// ----------------------------------------------------------------------------
// - main ---------------------------------------------------------------------
// ----------------------------------------------------------------------------

int test_main(int, char *[]) {

  construction_test();
  element_access_test();
  copy_test();
  mutate_test();
  make_tuple_test();
  tie_test();
  equality_test();
  ordering_test();
  cons_test();
  const_tuple_test();
  tuple_length_test();
  tuple_swap_test();
  return 0;
}







