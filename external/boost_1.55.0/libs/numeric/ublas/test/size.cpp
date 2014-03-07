/** -*- c++ -*- \file size.hpp \brief Test the \c size operation. */

#include <boost/numeric/ublas/fwd.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/operation/size.hpp>
#include <boost/numeric/ublas/tags.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <iostream>
#include "libs/numeric/ublas/test/utils.hpp"


BOOST_UBLAS_TEST_DEF( test_vector_container )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Vector Container" );

    typedef double value_type;
    typedef boost::numeric::ublas::vector<value_type> vector_type;

    vector_type v(5);

    v(0) = 0.555950;
    v(1) = 0.108929;
    v(2) = 0.948014;
    v(3) = 0.023787;
    v(4) = 1.023787;


    // size(v)
    BOOST_UBLAS_DEBUG_TRACE( "size(v) = " << boost::numeric::ublas::size(v) << " ==> " << v.size() );
    BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::size(v) == v.size() );

    // size<1>(v)
    BOOST_UBLAS_DEBUG_TRACE( "size<1>(v) = " << (boost::numeric::ublas::size<1>(v)) << " ==> " << v.size() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<1>(v) == v.size()) );

    // [NOT_COMPILE]: this should *correctly* cause a compilation error
    // size<2>(v)
    //BOOST_UBLAS_DEBUG_TRACE( "size<2>(v) = " << (boost::numeric::ublas::size<vector_type,2>(v)) << " ==> " << v.size() );
    //BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<2>(v) == v.size()) );
    // [/NOT_COMPILE]
}


BOOST_UBLAS_TEST_DEF( test_vector_expression )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Vector Expression" );

    typedef double value_type;
    typedef boost::numeric::ublas::vector<value_type> vector_type;

    vector_type v(5);

    v(0) = 0.555950;
    v(1) = 0.108929;
    v(2) = 0.948014;
    v(3) = 0.023787;
    v(4) = 1.023787;


    // size(-v)
    BOOST_UBLAS_DEBUG_TRACE( "size(-v) = " << boost::numeric::ublas::size(-v) << " ==> " << (-v).size() );
    BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::size(-v) == (-v).size() );

    // size<1>(-v)
    BOOST_UBLAS_DEBUG_TRACE( "size<1>(-v) = " << (boost::numeric::ublas::size<1>(-v)) << " ==> " << (-v).size() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<1>(-v) == (-v).size()) );
}


BOOST_UBLAS_TEST_DEF( test_vector_reference )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Vector Reference" );

    typedef double value_type;
    typedef boost::numeric::ublas::vector<value_type> vector_type;
    typedef boost::numeric::ublas::vector_reference<vector_type> vector_reference_type;

    vector_type v(5);

    v(0) = 0.555950;
    v(1) = 0.108929;
    v(2) = 0.948014;
    v(3) = 0.023787;
    v(4) = 1.023787;


    // size(reference(v)
    BOOST_UBLAS_DEBUG_TRACE( "size(reference(v)) = " << boost::numeric::ublas::size(vector_reference_type(v)) << " ==> " << vector_reference_type(v).size() );
    BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::size(vector_reference_type(v)) == vector_reference_type(v).size() );

    // size<1>(reference(v))
    BOOST_UBLAS_DEBUG_TRACE( "size<1>(reference(v)) = " << (boost::numeric::ublas::size<1>(vector_reference_type(v))) << " ==> " << vector_reference_type(v).size() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<1>(vector_reference_type(v)) == vector_reference_type(v).size()) );
}


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


    // [NOT_COMPILE]
    // size(A)
    //BOOST_UBLAS_DEBUG_TRACE( "size(A) = " << boost::numeric::ublas::size(A) << " ==> " << A.size1() );
    //BOOST_UBLAS_TEST_CHECK( boost::numeric::ublas::size(A) == A.size1() );
    // [/NOT_COMPILE]

    // size<1>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<1>(A) = " << (boost::numeric::ublas::size<1>(A)) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<1>(A) == A.size1()) );

    // size<2>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<2>(A) = " << (boost::numeric::ublas::size<2>(A)) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<2>(A) == A.size2()) );

    // size<major>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<major>(A) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(A)) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(A) == A.size1()) );

    // size<minor>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<minor>(A) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(A)) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(A) == A.size2()) );

    // size<leading>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<leading>(A) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(A)) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(A) == A.size2()) );
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


    // size<1>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<1>(A) = " << (boost::numeric::ublas::size<1>(A)) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<1>(A) == A.size1()) );

    // size<2>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<2>(A) = " << (boost::numeric::ublas::size<2>(A)) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<2>(A) == A.size2()) );

    // size<major>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<major>(A) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(A)) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(A) == A.size2()) );

    // size<minor>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<minor>(A) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(A)) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(A) == A.size1()) );

    // size<leading>(A)
    BOOST_UBLAS_DEBUG_TRACE( "size<leading>(A) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(A)) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(A) == A.size1()) );
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


    // size<1>(A')
    BOOST_UBLAS_DEBUG_TRACE( "size<1>(A') = " << (boost::numeric::ublas::size<1>(boost::numeric::ublas::trans(A))) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<1>(boost::numeric::ublas::trans(A)) == A.size2()) );

    // size<2>(A')
    BOOST_UBLAS_DEBUG_TRACE( "size<2>(A') = " << (boost::numeric::ublas::size<2>(boost::numeric::ublas::trans(A))) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<2>(boost::numeric::ublas::trans(A)) == A.size1()) );

    // size<major>(A') [A is row-major => A' column-major, and viceversa]
    BOOST_UBLAS_DEBUG_TRACE( "size<major>(A') = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(boost::numeric::ublas::trans(A))) << " ==> " << A.size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(boost::numeric::ublas::trans(A)) == A.size1()) );

    // size<minor>(A')  [A is row-major => A' column-major, and viceversa]
    BOOST_UBLAS_DEBUG_TRACE( "size<minor>(A') = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(boost::numeric::ublas::trans(A))) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(boost::numeric::ublas::trans(A)) == A.size2()) );

    // size<leading>(A')  [A row-major => A' column-major, and viceversa]
    BOOST_UBLAS_DEBUG_TRACE( "size<leading>(A') = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(boost::numeric::ublas::trans(A))) << " ==> " << A.size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(boost::numeric::ublas::trans(A)) == A.size2()) );
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


    // size<1>(reference(A))
    BOOST_UBLAS_DEBUG_TRACE( "size<1>(reference(A)) = " << (boost::numeric::ublas::size<1>(matrix_reference_type(A))) << " ==> " << matrix_reference_type(A).size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<1>(matrix_reference_type(A)) == matrix_reference_type(A).size1()) );

    // size<2>(reference(A))
    BOOST_UBLAS_DEBUG_TRACE( "size<2>(reference(A)) = " << (boost::numeric::ublas::size<2>(matrix_reference_type(A))) << " ==> " << matrix_reference_type(A).size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<2>(matrix_reference_type(A)) == matrix_reference_type(A).size2()) );

    // size<major>(reference(A))
    BOOST_UBLAS_DEBUG_TRACE( "size<major>(reference(A) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(matrix_reference_type(A))) << " ==> " << matrix_reference_type(A).size1() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::major>(matrix_reference_type(A)) == matrix_reference_type(A).size1()) );

    // size<minor>(reference(A))
    BOOST_UBLAS_DEBUG_TRACE( "size<minor>(reference(A)) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(matrix_reference_type(A))) << " ==> " << matrix_reference_type(A).size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::minor>(matrix_reference_type(A)) == matrix_reference_type(A).size2()) );

    // size<leading>(reference(A))
    BOOST_UBLAS_DEBUG_TRACE( "size<leading>(reference(A)) = " << (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(matrix_reference_type(A))) << " ==> " << matrix_reference_type(A).size2() );
    BOOST_UBLAS_TEST_CHECK( (boost::numeric::ublas::size<boost::numeric::ublas::tag::leading>(matrix_reference_type(A)) == matrix_reference_type(A).size2()) );
}


int main()
{
    BOOST_UBLAS_TEST_BEGIN();

    BOOST_UBLAS_TEST_DO( test_vector_container );
    BOOST_UBLAS_TEST_DO( test_vector_expression );
    BOOST_UBLAS_TEST_DO( test_vector_reference );
    BOOST_UBLAS_TEST_DO( test_row_major_matrix_container );
    BOOST_UBLAS_TEST_DO( test_col_major_matrix_container );
    BOOST_UBLAS_TEST_DO( test_matrix_expression );
    BOOST_UBLAS_TEST_DO( test_matrix_reference );

    BOOST_UBLAS_TEST_END();
}
