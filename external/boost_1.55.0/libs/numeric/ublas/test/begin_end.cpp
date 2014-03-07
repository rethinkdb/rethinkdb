/** -*- c++ -*- \file begin_end.hpp \brief Test the \c begin and \c end operations. */

#include <cmath>
#include <boost/numeric/ublas/traits/const_iterator_type.hpp>
#include <boost/numeric/ublas/traits/iterator_type.hpp>
#include <boost/numeric/ublas/traits/c_array.hpp>
#include <boost/numeric/ublas/fwd.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/operation/begin.hpp>
#include <boost/numeric/ublas/operation/end.hpp>
#include <boost/numeric/ublas/tags.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <iostream>
#include "libs/numeric/ublas/test/utils.hpp"


static const double TOL(1.0e-5); ///< Used for comparing two real numbers.

#ifdef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
#error "sorry this feature is not supported by your compiler"
#endif

BOOST_UBLAS_TEST_DEF( test_vector_iteration )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Vector Iteration" );

    typedef double value_type;
    typedef boost::numeric::ublas::vector<value_type> vector_type;

    vector_type v(5);

    v(0) = 0.555950;
    v(1) = 0.108929;
    v(2) = 0.948014;
    v(3) = 0.023787;
    v(4) = 1.023787;


    vector_type::size_type ix = 0;
    for (
            boost::numeric::ublas::iterator_type<vector_type>::type it = boost::numeric::ublas::begin<vector_type>(v);
            it != boost::numeric::ublas::end<vector_type>(v);
            ++it
    ) {
        BOOST_UBLAS_DEBUG_TRACE( "*it = " << *it << " ==> " << v(ix) );
        BOOST_UBLAS_TEST_CHECK( std::abs(*it - v(ix)) <= TOL );
        ++ix;
    }
}


BOOST_UBLAS_TEST_DEF( test_vector_const_iteration )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Vector Const Iteration" );

    typedef double value_type;
    typedef boost::numeric::ublas::vector<value_type> vector_type;

    vector_type v(5);

    v(0) = 0.555950;
    v(1) = 0.108929;
    v(2) = 0.948014;
    v(3) = 0.023787;
    v(4) = 1.023787;


    vector_type::size_type ix = 0;
    for (
            boost::numeric::ublas::const_iterator_type<vector_type>::type it = boost::numeric::ublas::begin<vector_type>(v);
            it != boost::numeric::ublas::end<vector_type>(v);
            ++it
    ) {
        BOOST_UBLAS_DEBUG_TRACE( "*it = " << *it << " ==> " << v(ix) );
        BOOST_UBLAS_TEST_CHECK( std::abs(*it - v(ix)) <= TOL );
        ++ix;
    }
}


BOOST_UBLAS_TEST_DEF( test_row_major_matrix_iteration )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Row-major Matrix Iteration" );

    typedef double value_type;
    typedef boost::numeric::ublas::matrix<value_type, boost::numeric::ublas::row_major> matrix_type;
    typedef boost::numeric::ublas::iterator_type<matrix_type, boost::numeric::ublas::tag::major>::type outer_iterator_type;
    typedef boost::numeric::ublas::iterator_type<matrix_type, boost::numeric::ublas::tag::minor>::type inner_iterator_type;

    matrix_type A(5,4);

    A(0,0) = 0.555950; A(0,1) = 0.274690; A(0,2) = 0.540605; A(0,3) = 0.798938;
    A(1,0) = 0.108929; A(1,1) = 0.830123; A(1,2) = 0.891726; A(1,3) = 0.895283;
    A(2,0) = 0.948014; A(2,1) = 0.973234; A(2,2) = 0.216504; A(2,3) = 0.883152;
    A(3,0) = 0.023787; A(3,1) = 0.675382; A(3,2) = 0.231751; A(3,3) = 0.450332;
    A(4,0) = 1.023787; A(4,1) = 1.675382; A(4,2) = 1.231751; A(4,3) = 1.450332;


    matrix_type::size_type row(0);
    for (
            outer_iterator_type outer_it = boost::numeric::ublas::begin<boost::numeric::ublas::tag::major>(A);
            outer_it != boost::numeric::ublas::end<boost::numeric::ublas::tag::major>(A);
            ++outer_it
    ) {
        matrix_type::size_type col(0);

        for (
                inner_iterator_type inner_it = boost::numeric::ublas::begin(outer_it);
                inner_it != boost::numeric::ublas::end(outer_it);
                ++inner_it
        ) {
            BOOST_UBLAS_DEBUG_TRACE( "*it = " << *inner_it << " ==> " << A(row,col) );
            BOOST_UBLAS_TEST_CHECK( std::abs(*inner_it - A(row,col)) <= TOL );

            ++col;
        }

        ++row;
    }
}


BOOST_UBLAS_TEST_DEF( test_col_major_matrix_iteration )
{
    BOOST_UBLAS_DEBUG_TRACE( "TEST Column-major Matrix Iteration" );

    typedef double value_type;
    typedef boost::numeric::ublas::matrix<value_type, boost::numeric::ublas::column_major> matrix_type;
    typedef boost::numeric::ublas::iterator_type<matrix_type, boost::numeric::ublas::tag::major>::type outer_iterator_type;
    typedef boost::numeric::ublas::iterator_type<matrix_type, boost::numeric::ublas::tag::minor>::type inner_iterator_type;

    matrix_type A(5,4);

    A(0,0) = 0.555950; A(0,1) = 0.274690; A(0,2) = 0.540605; A(0,3) = 0.798938;
    A(1,0) = 0.108929; A(1,1) = 0.830123; A(1,2) = 0.891726; A(1,3) = 0.895283;
    A(2,0) = 0.948014; A(2,1) = 0.973234; A(2,2) = 0.216504; A(2,3) = 0.883152;
    A(3,0) = 0.023787; A(3,1) = 0.675382; A(3,2) = 0.231751; A(3,3) = 0.450332;
    A(4,0) = 1.023787; A(4,1) = 1.675382; A(4,2) = 1.231751; A(4,3) = 1.450332;


    matrix_type::size_type col(0);
    for (
            outer_iterator_type outer_it = boost::numeric::ublas::begin<boost::numeric::ublas::tag::major>(A);
            outer_it != boost::numeric::ublas::end<boost::numeric::ublas::tag::major>(A);
            ++outer_it
    ) {
        matrix_type::size_type row(0);

        for (
                inner_iterator_type inner_it = boost::numeric::ublas::begin(outer_it);
                inner_it != boost::numeric::ublas::end(outer_it);
                ++inner_it
        ) {
            BOOST_UBLAS_DEBUG_TRACE( "*it = " << *inner_it << " ==> " << A(row,col) );
            BOOST_UBLAS_TEST_CHECK( std::abs(*inner_it - A(row,col)) <= TOL );

            ++row;
        }

        ++col;
    }
}


int main()
{
    BOOST_UBLAS_TEST_BEGIN();

    BOOST_UBLAS_TEST_DO( test_vector_iteration );
    BOOST_UBLAS_TEST_DO( test_vector_const_iteration );
    BOOST_UBLAS_TEST_DO( test_row_major_matrix_iteration );
    BOOST_UBLAS_TEST_DO( test_col_major_matrix_iteration );

    BOOST_UBLAS_TEST_END();
}
