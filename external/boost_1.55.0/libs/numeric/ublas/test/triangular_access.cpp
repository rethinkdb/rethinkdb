/* Test program to test find functions of triagular matrices
 *
 * author: Gunter Winkler ( guwi17 at gmx dot de )
 */
// Copyright 2008 Gunter Winkler <guwi17@gmx.de>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/cstdlib.hpp>

#include "common/testhelper.hpp"

#ifdef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
using boost::numeric::ublas::iterator1_tag;
using boost::numeric::ublas::iterator2_tag;
#endif

template < class MAT >
void test_iterator( MAT & A ) {

#ifndef NOMESSAGES
    std::cout << "=>";
#endif
  // check mutable iterators
  typename MAT::iterator1 it1 = A.begin1();
  typename MAT::iterator1 it1_end = A.end1();
  
  for ( ; it1 != it1_end; ++it1 ) {
#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
    typename MAT::iterator2 it2 = it1.begin();
    typename MAT::iterator2 it2_end = it1.end();
#else
    typename MAT::iterator2 it2 = begin(it1, iterator1_tag());
    typename MAT::iterator2 it2_end = end(it1, iterator1_tag());
#endif
    for ( ; it2 != it2_end ; ++ it2 ) {
#ifndef NOMESSAGES
      std::cout << "( " << it2.index1() << ", " << it2.index2() << ") " << std::flush;
#endif
      * it2 = ( 10 * it2.index1() + it2.index2() );
    }
#ifndef NOMESSAGES
    std::cout << std::endl;
#endif
  }

}

template < class MAT >
void test_iterator2( MAT & A ) {

#ifndef NOMESSAGES
    std::cout << "=>";
#endif
  // check mutable iterators
  typename MAT::iterator2 it2 = A.begin2();
  typename MAT::iterator2 it2_end = A.end2();
  
  for ( ; it2 != it2_end; ++it2 ) {
#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
    typename MAT::iterator1 it1 = it2.begin();
    typename MAT::iterator1 it1_end = it2.end();
#else
    typename MAT::iterator1 it1 = begin(it2, iterator2_tag());
    typename MAT::iterator1 it1_end = end(it2, iterator2_tag());
#endif
    for ( ; it1 != it1_end ; ++ it1 ) {
#ifndef NOMESSAGES
      std::cout << "( " << it1.index1() << ", " << it1.index2() << ") " << std::flush;
#endif
      * it1 = ( 10 * it1.index1() + it1.index2() );
    }
#ifndef NOMESSAGES
    std::cout << std::endl;
#endif
  }

}

template < class MAT >
typename MAT::value_type 
test_iterator3( const MAT & A ) {

#ifndef NOMESSAGES
    std::cout << "=>";
#endif
  typename MAT::value_type result = 0;

  // check mutable iterators
  typename MAT::const_iterator1 it1 = A.begin1();
  typename MAT::const_iterator1 it1_end = A.end1();
  
  for ( ; it1 != it1_end; ++it1 ) {
#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
    typename MAT::const_iterator2 it2 = it1.begin();
    typename MAT::const_iterator2 it2_end = it1.end();
#else
    typename MAT::const_iterator2 it2 = begin(it1, iterator1_tag());
    typename MAT::const_iterator2 it2_end = end(it1, iterator1_tag());
#endif
    for ( ; it2 != it2_end ; ++ it2 ) {
#ifndef NOMESSAGES
      std::cout << "( " << it2.index1() << ", " << it2.index2() << ") " << std::flush;
#endif
      result += * it2;
    }
#ifndef NOMESSAGES
    std::cout << std::endl;
#endif
  }
  return result;

}


int main (int argc, char * argv[]) {
    using namespace boost::numeric::ublas;

    typedef double VALUE_TYPE;
    typedef triangular_matrix<VALUE_TYPE, lower>        LT;
    typedef triangular_matrix<VALUE_TYPE, unit_lower>   ULT;
    typedef triangular_matrix<VALUE_TYPE, strict_lower> SLT;
    typedef triangular_matrix<VALUE_TYPE, upper>        UT;
    typedef triangular_matrix<VALUE_TYPE, unit_upper>   UUT;
    typedef triangular_matrix<VALUE_TYPE, strict_upper> SUT;

    LT A(5,5);

    test_iterator(A);
    test_iterator2(A);

    ULT B(5,5);

    test_iterator(B);
    test_iterator2(B);

    SLT C(5,5);

    test_iterator(C);
    test_iterator2(C);

    UT D(5,5);

    test_iterator(D);
    test_iterator2(D);

    UUT E(5,5);

    test_iterator(E);
    test_iterator2(E);

    SUT F(5,5);

    test_iterator(F);
    test_iterator2(F);

    assertTrue("Write access using iterators: ", true);

    assertEquals(" LT: ",420.0,test_iterator3(A));
    assertEquals("ULT: ",315.0,test_iterator3(B));
    assertEquals("SLT: ",310.0,test_iterator3(C));
    assertEquals(" UT: ",240.0,test_iterator3(D));
    assertEquals("UUT: ",135.0,test_iterator3(E));
    assertEquals("SUT: ",130.0,test_iterator3(F));

    assertTrue("Read access using iterators: ", true);

#ifndef NOMESSAGES
    std::cout << A << B << C << D << E << F << std::endl;
#endif

    typedef matrix<VALUE_TYPE> MATRIX;
    MATRIX mat(5,5);
    triangular_adaptor<MATRIX, lower> lta((mat));
    triangular_adaptor<MATRIX, unit_lower> ulta((mat));
    triangular_adaptor<MATRIX, strict_lower> slta((mat));
    triangular_adaptor<MATRIX, upper> uta((mat));
    triangular_adaptor<MATRIX, unit_upper> uuta((mat));
    triangular_adaptor<MATRIX, strict_upper> suta((mat));

    test_iterator ( lta );
    test_iterator2( lta );

    test_iterator ( ulta );
    test_iterator2( ulta );

    test_iterator ( slta );
    test_iterator2( slta );

    test_iterator ( uta );
    test_iterator2( uta );

    test_iterator ( uuta );
    test_iterator2( uuta );

    test_iterator ( suta );
    test_iterator2( suta );

    assertTrue("Write access using adaptors: ", true);

    assertEquals(" LTA: ",420.0,test_iterator3( lta ));
    assertEquals("ULTA: ",315.0,test_iterator3( ulta ));
    assertEquals("SLTA: ",310.0,test_iterator3( slta ));

    assertEquals(" UTA: ",240.0,test_iterator3( uta ));
    assertEquals("UUTA: ",135.0,test_iterator3( uuta ));
    assertEquals("SUTA: ",130.0,test_iterator3( suta ));

    assertTrue("Read access using adaptors: ", true);
    
#ifndef NOMESSAGES
    std::cout << mat << std::endl;
#endif

    return (getResults().second > 0) ? boost::exit_failure : boost::exit_success;
}
