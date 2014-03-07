#ifndef BOOST_UBLAS_NO_ELEMENT_PROXIES
# define BOOST_UBLAS_NO_ELEMENT_PROXIES
#endif

#include<boost/numeric/ublas/matrix_sparse.hpp>
#include<boost/numeric/ublas/io.hpp>

#include "libs/numeric/ublas/test/utils.hpp"

using std::cout;
using std::endl;

BOOST_UBLAS_TEST_DEF( test_coordinate_matrix_sort )
{

    boost::numeric::ublas::coordinate_matrix<double> matrix_mask(3, 3, 2);
    cout << "Setting matrix(1,1) = 2.1" << endl;
    matrix_mask(1,1) = 2.1;

    cout << "Displaying matrix(1,1)" << endl;
    std::cout << matrix_mask(1,1) << std::endl;

    BOOST_UBLAS_DEBUG_TRACE( "Displaying matrix(1,1)" << matrix_mask(1,1) );
    BOOST_UBLAS_TEST_CHECK( matrix_mask(1,1) == 2.1 );

    BOOST_UBLAS_TEST_CHECK( matrix_mask.index1_data()[0] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.index2_data()[0] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.value_data()[0] == 2.1 );

    BOOST_UBLAS_DEBUG_TRACE( "Setting matrix(0,1) = 1.1" );
    matrix_mask(0, 1) = 1.1;

    BOOST_UBLAS_TEST_CHECK( matrix_mask.index1_data()[0] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.index2_data()[0] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.value_data()[0] == 2.1 );

    BOOST_UBLAS_TEST_CHECK( matrix_mask.index1_data()[1] == 0 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.index2_data()[1] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.value_data()[1] == 1.1 );

    BOOST_UBLAS_DEBUG_TRACE( "Sort the matrix - this would be triggered by any element lookup." );
    matrix_mask.sort();

    BOOST_UBLAS_TEST_CHECK( matrix_mask.index1_data()[1] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.index2_data()[1] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.value_data()[1] == 2.1 );

    BOOST_UBLAS_TEST_CHECK( matrix_mask.index1_data()[0] == 0 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.index2_data()[0] == 1 );
    BOOST_UBLAS_TEST_CHECK( matrix_mask.value_data()[0] == 1.1 );

    BOOST_UBLAS_DEBUG_TRACE( "Displaying matrix(1,1)" << matrix_mask(1,1) );
    BOOST_UBLAS_TEST_CHECK( matrix_mask(1,1) == 2.1 );

    BOOST_UBLAS_DEBUG_TRACE( "Displaying matrix(0,1)" << matrix_mask(0,1) );
    BOOST_UBLAS_TEST_CHECK( matrix_mask(0,1) == 1.1 );

}

int main()
{
    BOOST_UBLAS_TEST_BEGIN();

    BOOST_UBLAS_TEST_DO( test_coordinate_matrix_sort );

    BOOST_UBLAS_TEST_END();
}
