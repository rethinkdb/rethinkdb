//  smart pointer test program  ----------------------------------------------//

//  Copyright Beman Dawes 1998, 1999.  Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/smart_ptr for documentation.

//  Revision History
//  24 May 01  use Boost test library for error detection, reporting, add tests
//             for operations on incomplete types (Beman Dawes) 
//  29 Nov 99  added std::swap and associative container tests (Darin Adler)
//  25 Sep 99  added swap tests
//  20 Jul 99  header name changed to .hpp
//  20 Apr 99  additional error tests added.

#include <boost/config.hpp>

#if defined(BOOST_MSVC)

# pragma warning(disable: 4786)  // identifier truncated in debug info
# pragma warning(disable: 4710)  // function not inlined
# pragma warning(disable: 4711)  // function selected for automatic inline expansion
# pragma warning(disable: 4514)  // unreferenced inline removed

#if (BOOST_MSVC >= 1310)
# pragma warning(disable: 4675)  // resolved overload found with Koenig lookup
#endif

#endif

#ifdef __BORLANDC__
# pragma warn -8092 // template argument passed to 'find' is not an iterator
#endif

#include <boost/smart_ptr.hpp>

#include <boost/detail/lightweight_test.hpp>

#include <iostream>
#include <set>
#include <string.h>

class Incomplete;

Incomplete * get_ptr(  boost::shared_ptr<Incomplete>& incomplete )
{
  return incomplete.get();
}

template<class T>
void ck( const T* v1, T v2 ) { BOOST_TEST( *v1 == v2 ); }

namespace {
  int UDT_use_count;  // independent of pointer maintained counts
  }

//  user defined type  -------------------------------------------------------//

class UDT {
  long value_;
 public:
  explicit UDT( long value=0 ) : value_(value) { ++UDT_use_count; }
  ~UDT() {
    --UDT_use_count;
    std::cout << "UDT with value " << value_ << " being destroyed\n";
    }
  long value() const { return value_; }
  void value( long v ) { value_ = v;; }
  };  // UDT

//  tests on incomplete types  -----------------------------------------------//

//  Certain smart pointer operations are specified to work on incomplete types,
//  and some uses depend upon this feature.  These tests verify compilation
//  only - the functions aren't actually invoked.

class Incomplete;

Incomplete * check_incomplete( boost::scoped_ptr<Incomplete>& incomplete )
{
  return incomplete.get();
}

Incomplete * check_incomplete( boost::shared_ptr<Incomplete>& incomplete,
                               boost::shared_ptr<Incomplete>& i2 )
{
  incomplete.swap(i2);
  std::cout << incomplete.use_count() << ' ' << incomplete.unique() << '\n';
  return incomplete.get();
}

//  This isn't a very systematic test; it just hits some of the basics.

void test()
{
    BOOST_TEST( UDT_use_count == 0 );  // reality check

    //  test scoped_ptr with a built-in type
    long * lp = new long;
    boost::scoped_ptr<long> sp ( lp );
    BOOST_TEST( sp.get() == lp );
    BOOST_TEST( lp == sp.get() );
    BOOST_TEST( &*sp == lp );

    *sp = 1234568901L;
    BOOST_TEST( *sp == 1234568901L );
    BOOST_TEST( *lp == 1234568901L );
    ck( static_cast<long*>(sp.get()), 1234568901L );
    ck( lp, *sp );

    sp.reset();
    BOOST_TEST( sp.get() == 0 );

    //  test scoped_ptr with a user defined type
    boost::scoped_ptr<UDT> udt_sp ( new UDT( 999888777 ) );
    BOOST_TEST( udt_sp->value() == 999888777 );
    udt_sp.reset();
    udt_sp.reset( new UDT( 111222333 ) );
    BOOST_TEST( udt_sp->value() == 111222333 );
    udt_sp.reset( new UDT( 333222111 ) );
    BOOST_TEST( udt_sp->value() == 333222111 );

    //  test scoped_array with a build-in type
    char * sap = new char [ 100 ];
    boost::scoped_array<char> sa ( sap );
    BOOST_TEST( sa.get() == sap );
    BOOST_TEST( sap == sa.get() );

    strcpy( sa.get(), "Hot Dog with mustard and relish" );
    BOOST_TEST( strcmp( sa.get(), "Hot Dog with mustard and relish" ) == 0 );
    BOOST_TEST( strcmp( sap, "Hot Dog with mustard and relish" ) == 0 );

    BOOST_TEST( sa[0] == 'H' );
    BOOST_TEST( sa[30] == 'h' );

    sa[0] = 'N';
    sa[4] = 'd';
    BOOST_TEST( strcmp( sap, "Not dog with mustard and relish" ) == 0 );

    sa.reset();
    BOOST_TEST( sa.get() == 0 );

    //  test shared_ptr with a built-in type
    int * ip = new int;
    boost::shared_ptr<int> cp ( ip );
    BOOST_TEST( ip == cp.get() );
    BOOST_TEST( cp.use_count() == 1 );

    *cp = 54321;
    BOOST_TEST( *cp == 54321 );
    BOOST_TEST( *ip == 54321 );
    ck( static_cast<int*>(cp.get()), 54321 );
    ck( static_cast<int*>(ip), *cp );

    boost::shared_ptr<int> cp2 ( cp );
    BOOST_TEST( ip == cp2.get() );
    BOOST_TEST( cp.use_count() == 2 );
    BOOST_TEST( cp2.use_count() == 2 );

    BOOST_TEST( *cp == 54321 );
    BOOST_TEST( *cp2 == 54321 );
    ck( static_cast<int*>(cp2.get()), 54321 );
    ck( static_cast<int*>(ip), *cp2 );

    boost::shared_ptr<int> cp3 ( cp );
    BOOST_TEST( cp.use_count() == 3 );
    BOOST_TEST( cp2.use_count() == 3 );
    BOOST_TEST( cp3.use_count() == 3 );
    cp.reset();
    BOOST_TEST( cp2.use_count() == 2 );
    BOOST_TEST( cp3.use_count() == 2 );
    cp.reset( new int );
    *cp =  98765;
    BOOST_TEST( *cp == 98765 );
    *cp3 = 87654;
    BOOST_TEST( *cp3 == 87654 );
    BOOST_TEST( *cp2 == 87654 );
    cp.swap( cp3 );
    BOOST_TEST( *cp == 87654 );
    BOOST_TEST( *cp2 == 87654 );
    BOOST_TEST( *cp3 == 98765 );
    cp.swap( cp3 );
    BOOST_TEST( *cp == 98765 );
    BOOST_TEST( *cp2 == 87654 );
    BOOST_TEST( *cp3 == 87654 );
    cp2 = cp2;
    BOOST_TEST( cp2.use_count() == 2 );
    BOOST_TEST( *cp2 == 87654 );
    cp = cp2;
    BOOST_TEST( cp2.use_count() == 3 );
    BOOST_TEST( *cp2 == 87654 );
    BOOST_TEST( cp.use_count() == 3 );
    BOOST_TEST( *cp == 87654 );

#if defined( BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP )
    using boost::swap;
#endif

    boost::shared_ptr<int> cp4;
    swap( cp2, cp4 );
    BOOST_TEST( cp4.use_count() == 3 );
    BOOST_TEST( *cp4 == 87654 );
    BOOST_TEST( cp2.get() == 0 );

    std::set< boost::shared_ptr<int> > scp;
    scp.insert(cp4);
    BOOST_TEST( scp.find(cp4) != scp.end() );
    BOOST_TEST( scp.find(cp4) == scp.find( boost::shared_ptr<int>(cp4) ) );

    //  test shared_array with a built-in type
    char * cap = new char [ 100 ];
    boost::shared_array<char> ca ( cap );
    BOOST_TEST( ca.get() == cap );
    BOOST_TEST( cap == ca.get() );
    BOOST_TEST( &ca[0] == cap );

    strcpy( ca.get(), "Hot Dog with mustard and relish" );
    BOOST_TEST( strcmp( ca.get(), "Hot Dog with mustard and relish" ) == 0 );
    BOOST_TEST( strcmp( cap, "Hot Dog with mustard and relish" ) == 0 );

    BOOST_TEST( ca[0] == 'H' );
    BOOST_TEST( ca[30] == 'h' );

    boost::shared_array<char> ca2 ( ca );
    boost::shared_array<char> ca3 ( ca2 );

    ca[0] = 'N';
    ca[4] = 'd';
    BOOST_TEST( strcmp( ca.get(), "Not dog with mustard and relish" ) == 0 );
    BOOST_TEST( strcmp( ca2.get(), "Not dog with mustard and relish" ) == 0 );
    BOOST_TEST( strcmp( ca3.get(), "Not dog with mustard and relish" ) == 0 );
    BOOST_TEST( ca.use_count() == 3 );
    BOOST_TEST( ca2.use_count() == 3 );
    BOOST_TEST( ca3.use_count() == 3 );
    ca2.reset();
    BOOST_TEST( ca.use_count() == 2 );
    BOOST_TEST( ca3.use_count() == 2 );
    BOOST_TEST( ca2.use_count() == 0 );

    ca.reset();
    BOOST_TEST( ca.get() == 0 );

    boost::shared_array<char> ca4;
    swap( ca3, ca4 );
    BOOST_TEST( ca4.use_count() == 1 );
    BOOST_TEST( strcmp( ca4.get(), "Not dog with mustard and relish" ) == 0 );
    BOOST_TEST( ca3.get() == 0 );

    std::set< boost::shared_array<char> > sca;
    sca.insert(ca4);
    BOOST_TEST( sca.find(ca4) != sca.end() );
    BOOST_TEST( sca.find(ca4) == sca.find( boost::shared_array<char>(ca4) ) );

    //  test shared_array with user defined type
    boost::shared_array<UDT> udta ( new UDT[3] );

    udta[0].value( 111 );
    udta[1].value( 222 );
    udta[2].value( 333 );
    boost::shared_array<UDT> udta2 ( udta );

    BOOST_TEST( udta[0].value() == 111 );
    BOOST_TEST( udta[1].value() == 222 );
    BOOST_TEST( udta[2].value() == 333 );
    BOOST_TEST( udta2[0].value() == 111 );
    BOOST_TEST( udta2[1].value() == 222 );
    BOOST_TEST( udta2[2].value() == 333 );
    udta2.reset();
    BOOST_TEST( udta2.get() == 0 );
    BOOST_TEST( udta.use_count() == 1 );
    BOOST_TEST( udta2.use_count() == 0 );

    BOOST_TEST( UDT_use_count == 4 );  // reality check

    //  test shared_ptr with a user defined type
    UDT * up = new UDT;
    boost::shared_ptr<UDT> sup ( up );
    BOOST_TEST( up == sup.get() );
    BOOST_TEST( sup.use_count() == 1 );

    sup->value( 54321 ) ;
    BOOST_TEST( sup->value() == 54321 );
    BOOST_TEST( up->value() == 54321 );

    boost::shared_ptr<UDT> sup2;
    sup2 = sup;
    BOOST_TEST( sup2->value() == 54321 );
    BOOST_TEST( sup.use_count() == 2 );
    BOOST_TEST( sup2.use_count() == 2 );
    sup2 = sup2;
    BOOST_TEST( sup2->value() == 54321 );
    BOOST_TEST( sup.use_count() == 2 );
    BOOST_TEST( sup2.use_count() == 2 );

    std::cout << "OK\n";

    new char[12345]; // deliberate memory leak to verify leaks detected
}

int main()
{
    test();
    return boost::report_errors();
}
