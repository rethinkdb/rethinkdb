/** -*- c++ -*- \file num_rows.hpp \file Test the \c num_rows operation. */

#include <boost/numeric/ublas/fwd.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/operation/num_rows.hpp>
#include <iostream>
#include "libs/numeric/ublas/test/utils.hpp"


BOOST_UBLAS_TEST_DEF( test_row_major_matrix_container )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Row-major Matrix Container" );

    typedef double value_type;
    typedef boost::numeric::ublas::matrix<value_type, boost::numeric::ublas::row_major> matrix_type;

    matrix_type A(5,4);

    A(0,0) = 0.555950; A(0,1) = 0.274690; A(0,2) = 0.540605; A(0,3) = 0.798938;
    A(1,0) = 0.108929; A(1,1) = 0.830123; A(1,2) = 0.891726; A(1,3) = 0.895283;
    A(2,0) = 0.948014; A(2,1) = 0.973234; A(2,2) = 0.216504; A(2,3) = 0.883152;
    A(3,0) = 0.023787; A(3,1) = 0.675382; A(3,2) = 0.231751; A(3,3) = 0.450332;
    A(4,0) = 1.023787; A(4,1) = 1.675382; A(4,2) = 1.231751; A(4,3) = 1.450332;


    BOOST_UBLAS_DEBUG_TRACE( "num_rows(A) = " << boost::numeric::ublas::num_rows(A) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::num_rows(A) == A.size1() );
}


BOOST_UBLAS_TEST_DEF( test_col_major_matrix_container )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Column-major Matrix Container" );

    typedef double value_type;
    typedef boost::numeric::ublas::matrix<value_type, boost::numeric::ublas::column_major> matrix_type;

    matrix_type A(5,4);

    A(0,0) = 0.555950; A(0,1) = 0.274690; A(0,2) = 0.540605; A(0,3) = 0.798938;
    A(1,0) = 0.108929; A(1,1) = 0.830123; A(1,2) = 0.891726; A(1,3) = 0.895283;
    A(2,0) = 0.948014; A(2,1) = 0.973234; A(2,2) = 0.216504; A(2,3) = 0.883152;
    A(3,0) = 0.023787; A(3,1) = 0.675382; A(3,2) = 0.231751; A(3,3) = 0.450332;
    A(4,0) = 1.023787; A(4,1) = 1.675382; A(4,2) = 1.231751; A(4,3) = 1.450332;


    BOOST_UBLAS_DEBUG_TRACE( "num_rows(A) = " << boost::numeric::ublas::num_rows(A) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::num_rows(A) == A.size1() );
}


BOOST_UBLAS_TEST_DEF( test_matrix_expression )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Matrix Expression" );

    typedef double value_type;
    typedef boost::numeric::ublas::matrix<value_type> matrix_type;

    matrix_type A(5,4);

    A(0,0) = 0.555950; A(0,1) = 0.274690; A(0,2) = 0.540605; A(0,3) = 0.798938;
    A(1,0) = 0.108929; A(1,1) = 0.830123; A(1,2) = 0.891726; A(1,3) = 0.895283;
    A(2,0) = 0.948014; A(2,1) = 0.973234; A(2,2) = 0.216504; A(2,3) = 0.883152;
    A(3,0) = 0.023787; A(3,1) = 0.675382; A(3,2) = 0.231751; A(3,3) = 0.450332;
    A(4,0) = 1.023787; A(4,1) = 1.675382; A(4,2) = 1.231751; A(4,3) = 1.450332;


    BOOST_UBLAS_DEBUG_TRACE( "num_rows(A') = " << boost::numeric::ublas::num_rows(boost::numeric::ublas::trans(A)) << " ==> " << boost::numeric::ublas::trans(A).size1() );
    BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::num_rows(boost::numeric::ublas::trans(A)) == boost::numeric::ublas::trans(A).size1() );
}


BOOST_UBLAS_TEST_DEF( test_matrix_reference )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Matrix Reference" );

    typedef double value_type;
    typedef boost::numeric::ublas::matrix<value_type> matrix_type;
    typedef boost::numeric::ublas::matrix_reference<matrix_type> matrix_reference_type;

    matrix_type A(5,4);

    A(0,0) = 0.555950; A(0,1) = 0.274690; A(0,2) = 0.540605; A(0,3) = 0.798938;
    A(1,0) = 0.108929; A(1,1) = 0.830123; A(1,2) = 0.891726; A(1,3) = 0.895283;
    A(2,0) = 0.948014; A(2,1) = 0.973234; A(2,2) = 0.216504; A(2,3) = 0.883152;
    A(3,0) = 0.023787; A(3,1) = 0.675382; A(3,2) = 0.231751; A(3,3) = 0.450332;
    A(4,0) = 1.023787; A(4,1) = 1.675382; A(4,2) = 1.231751; A(4,3) = 1.450332;


    BOOST_UBLAS_DEBUG_TRACE( "num_rows(reference(A)) = " << boost::numeric::ublas::num_rows(matrix_reference_type(A)) << " ==> " << matrix_reference_type(A).size1() );
    BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::num_rows(matrix_reference_type(A)) == matrix_reference_type(A).size1() );
}


int main()
{
    BOOST_UBLAS_TEST_BEGIN();

    BOOST_UBLAS_TEST_DO( test_row_major_matrix_container );
    BOOST_UBLAS_TEST_DO( test_col_major_matrix_container );
    BOOST_UBLAS_TEST_DO( test_matrix_expression );
    BOOST_UBLAS_TEST_DO( test_matrix_reference );

    BOOST_UBLAS_TEST_END();
}
