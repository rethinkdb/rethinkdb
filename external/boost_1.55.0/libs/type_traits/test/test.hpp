
//  (C) Copyright John Maddock 2000. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TT_TEST_HPP
#define TT_TEST_HPP

#include <boost/config.hpp>

#if defined(_WIN32_WCE) && defined(BOOST_MSVC)
#pragma warning(disable:4201)
#endif

#ifdef USE_UNIT_TEST
#  include <boost/test/unit_test.hpp>
#endif
#include <boost/utility.hpp>
#include <iostream>
#include <typeinfo>

#ifdef __BORLANDC__
// we have to turn off these warnings otherwise we get swamped by the things:
#pragma option -w-8008 -w-8066
#endif

#ifdef _MSC_VER
// We have to turn off warnings that occur within the test suite:
#pragma warning(disable:4127)
#endif
#ifdef BOOST_INTEL
// remark #1418: external function definition with no prior declaration
// remark #981: operands are evaluated in unspecified order
#pragma warning(disable:1418 981)
#endif

#ifdef BOOST_INTEL
// turn off warnings from this header:
#pragma warning(push)
#pragma warning(disable:444)
#endif

//
// basic configuration:
//
#ifdef TEST_STD

#define tt std::tr1

//#define TYPE_TRAITS(x) <type_traits>
//#define TYPE_COMPARE(x) <type_compare>
//#define TYPE_TRANSFORM(x) <type_transform>

#else

#define tt boost

//#define TYPE_TRAITS(x) BOOST_STRINGIZE(boost/type_traits/x.hpp)
//#define TYPE_COMPARE(x) BOOST_STRINGIZE(boost/type_traits/x.hpp)
//#define TYPE_TRANSFORM(x) BOOST_STRINGIZE(boost/type_traits/x.hpp)

#endif

#ifdef USE_UNIT_TEST
//
// global unit, this is not safe, but until the unit test framework uses
// shared_ptr throughout this is about as good as it gets :-(
//
boost::unit_test::test_suite* get_master_unit(const char* name = 0);

//
// initialisation class:
//
class unit_initialiser
{
public:
   unit_initialiser(void (*f)(), const char* /*name*/)
   {
      get_master_unit("Type Traits")->add( BOOST_TEST_CASE(f) );
   }
};

#define TT_TEST_BEGIN(trait_name)\
   namespace{\
   void trait_name();\
   unit_initialiser init(trait_name, BOOST_STRINGIZE(trait_name));\
   void trait_name(){

#define TT_TEST_END }}

#else

//
// replacements for Unit test macros:
//
int error_count = 0;

#define BOOST_CHECK_MESSAGE(pred, message)\
   do{\
   if(!(pred))\
   {\
      std::cerr << __FILE__ << ":" << __LINE__ << ": " << message << std::endl;\
      ++error_count;\
   }\
   }while(0)

#define BOOST_WARN_MESSAGE(pred, message)\
   do{\
   if(!(pred))\
   {\
      std::cerr << __FILE__ << ":" << __LINE__ << ": " << message << std::endl;\
   }\
   }while(0)

#define BOOST_TEST_MESSAGE(message)\
   do{ std::cout << __FILE__ << ":" << __LINE__ << ": " << message << std::endl; }while(0)

#define BOOST_CHECK(pred)\
   do{ \
      if(!(pred)){\
         std::cout << __FILE__ << ":" << __LINE__ << ": Error in " << BOOST_STRINGIZE(pred) << std::endl;\
         ++error_count;\
      } \
   }while(0)

#define TT_TEST_BEGIN(trait_name)\
   int main(){
#define TT_TEST_END return error_count; }

#endif

#define TRANSFORM_CHECK(name, from_suffix, to_suffix)\
   BOOST_CHECK_TYPE(bool to_suffix, name<bool from_suffix>::type);\
   BOOST_CHECK_TYPE(char to_suffix, name<char from_suffix>::type);\
   BOOST_CHECK_TYPE(wchar_t to_suffix, name<wchar_t from_suffix>::type);\
   BOOST_CHECK_TYPE(signed char to_suffix, name<signed char from_suffix>::type);\
   BOOST_CHECK_TYPE(unsigned char to_suffix, name<unsigned char from_suffix>::type);\
   BOOST_CHECK_TYPE(short to_suffix, name<short from_suffix>::type);\
   BOOST_CHECK_TYPE(unsigned short to_suffix, name<unsigned short from_suffix>::type);\
   BOOST_CHECK_TYPE(int to_suffix, name<int from_suffix>::type);\
   BOOST_CHECK_TYPE(unsigned int to_suffix, name<unsigned int from_suffix>::type);\
   BOOST_CHECK_TYPE(long to_suffix, name<long from_suffix>::type);\
   BOOST_CHECK_TYPE(unsigned long to_suffix, name<unsigned long from_suffix>::type);\
   BOOST_CHECK_TYPE(float to_suffix, name<float from_suffix>::type);\
   BOOST_CHECK_TYPE(long double to_suffix, name<long double from_suffix>::type);\
   BOOST_CHECK_TYPE(double to_suffix, name<double from_suffix>::type);\
   BOOST_CHECK_TYPE(UDT to_suffix, name<UDT from_suffix>::type);\
   BOOST_CHECK_TYPE(enum1 to_suffix, name<enum1 from_suffix>::type);

#define BOOST_DUMMY_MACRO_PARAM /**/

#define BOOST_DECL_TRANSFORM_TEST(name, type, from, to)\
void name(){ TRANSFORM_CHECK(type, from, to) }
#define BOOST_DECL_TRANSFORM_TEST3(name, type, from)\
void name(){ TRANSFORM_CHECK(type, from, BOOST_DUMMY_MACRO_PARAM) }
#define BOOST_DECL_TRANSFORM_TEST2(name, type, to)\
void name(){ TRANSFORM_CHECK(type, BOOST_DUMMY_MACRO_PARAM, to) }
#define BOOST_DECL_TRANSFORM_TEST0(name, type)\
void name(){ TRANSFORM_CHECK(type, BOOST_DUMMY_MACRO_PARAM, BOOST_DUMMY_MACRO_PARAM) }



//
// VC++ emits an awful lot of warnings unless we define these:
#ifdef BOOST_MSVC
#  pragma warning(disable:4800)
#endif

//
// define some test types:
//
enum enum_UDT{ one, two, three };
struct UDT
{
   UDT();
   ~UDT();
   UDT(const UDT&);
   UDT& operator=(const UDT&);
   int i;

   void f1();
   int f2();
   int f3(int);
   int f4(int, float);
};

typedef void(*f1)();
typedef int(*f2)(int);
typedef int(*f3)(int, bool);
typedef void (UDT::*mf1)();
typedef int (UDT::*mf2)();
typedef int (UDT::*mf3)(int);
typedef int (UDT::*mf4)(int, float);
typedef int (UDT::*mp);
typedef int (UDT::*cmf)(int) const;

// cv-qualifiers applied to reference types should have no effect
// declare these here for later use with is_reference and remove_reference:
# ifdef BOOST_MSVC
#  pragma warning(push)
#  pragma warning(disable: 4181)
# elif defined(BOOST_INTEL)
#  pragma warning(push)
#  pragma warning(disable: 21)
# endif
//
// This is intentional:
// r_type and cr_type should be the same type
// but some compilers wrongly apply cv-qualifiers
// to reference types (this may generate a warning
// on some compilers):
//
typedef int& r_type;
#ifndef BOOST_INTEL
typedef const r_type cr_type;
#else
// recent Intel compilers generate a hard error on the above:
typedef r_type cr_type;
#endif
# ifdef BOOST_MSVC
#  pragma warning(pop)
# elif defined(BOOST_INTEL)
#  pragma warning(pop)
#  pragma warning(disable: 985) // identifier truncated in debug information
# endif

struct POD_UDT { int x; };
struct empty_UDT
{
   empty_UDT();
   empty_UDT(const empty_UDT&);
   ~empty_UDT();
   empty_UDT& operator=(const empty_UDT&);
   bool operator==(const empty_UDT&)const;
};
struct empty_POD_UDT
{
   bool operator==(const empty_POD_UDT&)const
   { return true; }
};
union union_UDT
{
  int x;
  double y;
  ~union_UDT(){}
};
union POD_union_UDT
{
  int x;
  double y;
};
union empty_union_UDT
{
   ~empty_union_UDT(){}
};
union empty_POD_union_UDT{};

struct nothrow_copy_UDT
{
   nothrow_copy_UDT();
   nothrow_copy_UDT(const nothrow_copy_UDT&)throw();
   ~nothrow_copy_UDT(){}
   nothrow_copy_UDT& operator=(const nothrow_copy_UDT&);
   bool operator==(const nothrow_copy_UDT&)const
   { return true; }
};

struct nothrow_assign_UDT
{
   nothrow_assign_UDT();
   nothrow_assign_UDT(const nothrow_assign_UDT&);
   ~nothrow_assign_UDT(){};
   nothrow_assign_UDT& operator=(const nothrow_assign_UDT&)throw(){ return *this; }
   bool operator==(const nothrow_assign_UDT&)const
   { return true; }
};

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
struct nothrow_move_UDT
{
   nothrow_move_UDT();
   nothrow_move_UDT(nothrow_move_UDT&&) throw();
   nothrow_move_UDT& operator=(nothrow_move_UDT&&) throw();
   bool operator==(const nothrow_move_UDT&)const
   { return true; }
};
#endif


struct nothrow_construct_UDT
{
   nothrow_construct_UDT()throw();
   nothrow_construct_UDT(const nothrow_construct_UDT&);
   ~nothrow_construct_UDT(){};
   nothrow_construct_UDT& operator=(const nothrow_construct_UDT&){ return *this; }
   bool operator==(const nothrow_construct_UDT&)const
   { return true; }
};

class Base { };

class Derived : public Base { };
class Derived2 : public Base { };
class MultiBase : public Derived, public Derived2 {};
class PrivateBase : private Base {};

class NonDerived { };

enum enum1
{
   one_,two_
};

enum enum2
{
   three_,four_
};

struct VB
{
   virtual ~VB(){};
};

struct VD : public VB
{
   ~VD(){};
};
//
// struct non_pointer:
// used to verify that is_pointer does not return
// true for class types that implement operator void*()
//
struct non_pointer
{
   operator void*(){return this;}
};
struct non_int_pointer
{
   int i;
   operator int*(){return &i;}
};
struct int_constructible
{
   int_constructible(int);
};
struct int_convertible
{
   operator int();
};
//
// struct non_empty:
// used to verify that is_empty does not emit
// spurious warnings or errors.
//
struct non_empty : private boost::noncopyable
{
   int i;
};
//
// abstract base classes:
struct test_abc1
{
   test_abc1();
   virtual ~test_abc1();
   test_abc1(const test_abc1&);
   test_abc1& operator=(const test_abc1&);
   virtual void foo() = 0;
   virtual void foo2() = 0;
};

struct test_abc2
{
   virtual ~test_abc2();
   virtual void foo() = 0;
   virtual void foo2() = 0;
};

struct test_abc3 : public test_abc1
{
   virtual void foo3() = 0;
};

struct incomplete_type;

struct polymorphic_base
{
   virtual ~polymorphic_base();
   virtual void method();
};

struct polymorphic_derived1 : public polymorphic_base
{
};

struct polymorphic_derived2 : public polymorphic_base
{
   virtual void method();
};

struct virtual_inherit1 : public virtual Base { };
struct virtual_inherit2 : public virtual_inherit1 { };
struct virtual_inherit3 : private virtual Base {};
struct virtual_inherit4 : public virtual boost::noncopyable {};
struct virtual_inherit5 : public virtual int_convertible {};
struct virtual_inherit6 : public virtual Base { virtual ~virtual_inherit6()throw(); };

typedef void foo0_t();
typedef void foo1_t(int);
typedef void foo2_t(int&, double);
typedef void foo3_t(int&, bool, int, int);
typedef void foo4_t(int, bool, int*, int[], int, int, int, int, int);

struct trivial_except_construct
{
   trivial_except_construct();
   int i;
};

struct trivial_except_destroy
{
   ~trivial_except_destroy();
   int i;
};

struct trivial_except_copy
{
   trivial_except_copy(trivial_except_copy const&);
   int i;
};

struct trivial_except_assign
{
   trivial_except_assign& operator=(trivial_except_assign const&);
   int i;
};

template <class T>
struct wrap
{
   T t;
   int j;
protected:
   wrap();
   wrap(const wrap&);
   wrap& operator=(const wrap&);
};

#ifdef BOOST_INTEL
#pragma warning(pop)
#endif

#endif

