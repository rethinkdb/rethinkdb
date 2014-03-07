
//  (C) Copyright Edward Diener 2011,2012
//  Use, modification and distribution are subject to the Boost Software License,
//  Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt).

#if !defined(TEST_STRUCTS_HPP)
#define TEST_STRUCTS_HPP

struct AType 
  {
  
  // Type
  
  typedef int AnIntType;
  struct AStructType
    {
    template <class> struct MStrMemberTemplate { };
    template<class X,class Y,short AA> static int StatFuncTemplate(X *,Y) { int ret(AA); return ret; }
    };
  typedef int & AnIntTypeReference;
  struct BType
    {
    typedef int AnIntegerType;
    struct CType
      {
      typedef int AnotherIntegerType;
      template <class,class,int,short,class,template <class,int> class,class> struct CTManyParameters { };
      template<class X,class Y,class Z,short AA> double SomeFuncTemplate(X,Y *,Z &) { double ret(AA); return ret; }
      };
    };
    
  // Template
    
  template <class> struct ATPMemberTemplate { };
  template <int> struct AMemberTemplate { };
  template <int,int> struct AnotherMemberTemplate { };
  template <class,class> struct CLMemberTemplate { };
  
  // Data
  
  int AnInt;
  BType IntBT;
  BType::CType NestedCT;
  
  // Function
  
  void VoidFunction() { }
  int IntFunction() { return 0; }
  
  // Const function
  
  double AConstFunction(long, char) const { return 2.57; }
  void WFunction(float, double = 4.3) const { }
  
  // Volatile function
  
  double AVolatileFunction(long, char = 'c') volatile { return 2.58; }
  void VolFunction(float, double) volatile { }
  
  // Const Volatile function
  
  double ACVFunction(long, char) const volatile { return 2.59; }
  void ConstVolFunction(float, double) const volatile { }
  
  // Function Templates
  
  template<class X,int Y> int AFuncTemplate(const X &) { return Y; }
  template<class X,class Y,class Z> void AFuncTemplate(X *,Y,Z &) { }
  
  // Static Data
  
  static short DSMember;
  
  // Static Function
  
  static int SIntFunction(long,double) { return 2; }
  
  // Static Function Template
  
  template<class X,class Y,class Z> static void AnotherFuncTemplate(X,Y &,const Z &) { }
  
  };
  
struct AnotherType
  {
  
  // Type
  
  typedef AType::AnIntType someOtherType;
    
  // Template
  
  template <class,class,class,class,class,class> struct SomeMemberTemplate { };
  template <class,class,int,class,template <class> class,class,long> struct ManyParameters { };
  template <class,class,class,class> struct SimpleTMP { };
  
  // Data
  
  bool aMember;
  bool cMem;
  long AnInt;
  AType OtherAT;
  AType::AStructType ONestStr;
  
  // Function
  
  AType aFunction(int = 7) { return AType(); }
  int anotherFunction(AType) { return 0; }
  AType::AnIntType sFunction(int,long = 88,double = 1.0) { return 0; }
  double IntFunction(int = 9922) { return 0; }
  
  // Const function
  
  int AnotherConstFunction(AType *, short) const { return 0; }
  AType StillSame(int) const { return OtherAT; }
  
  // Volatile function
  
  int AnotherVolatileFunction(AType *, short) volatile { return 0; }
  bool StillVolatile(int) volatile { return false; }
  
  // Const Volatile function
  
  int AnotherConstVolatileFunction(AType *, short) const volatile { return 0; }
  short StillCV(int = 3) const volatile { return 32; }
  
  // Function Templates
  
  template<class X> long MyFuncTemplate(X &) { return 0; }
  
  // Static Function
  
  static AType TIntFunction(long,double = 3.0) { return AType(); }
  static AType::AStructType TSFunction(AType::AnIntType,double) { return AType::AStructType(); }
  
  // Static Data
  
  static AType::AStructType AnStat;
  
  // Static Function Template
  
  template<class X,class Y> static void YetAnotherFuncTemplate(const X &,Y &) { }
  
  static const int CIntValue = 10;
  
  };
  
struct MarkerType
  {
  };

short AType::DSMember(5622);
AType::AStructType AnotherType::AnStat;

#endif // TEST_STRUCTS_HPP
