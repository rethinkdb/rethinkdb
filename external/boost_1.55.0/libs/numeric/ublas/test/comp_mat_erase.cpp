#include "boost/numeric/ublas/matrix_sparse.hpp"

#define BOOST_TEST_MODULE SparseMatrixErasureTest
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_CASE( compressed_matrix_erase_after_end )
{
    boost::numeric::ublas::compressed_matrix<int, boost::numeric::ublas::row_major > A(2, 2);

    BOOST_CHECK_EQUAL( A.filled1(), (int) 1 );
    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );

    A(0,0) = 1;

    BOOST_CHECK_EQUAL( A.nnz(), 1 );
    BOOST_CHECK_EQUAL( A.filled1(), 2 );

    // check new element
    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.index2_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.value_data()[0], 1 );
    // check end of list marker
    BOOST_CHECK_EQUAL( A.index1_data()[1], 1 );

    A.erase_element(1,0);

    BOOST_CHECK_EQUAL( A.nnz(), 1 );
    BOOST_CHECK_EQUAL( A.filled1(), 2 );
    BOOST_CHECK_EQUAL( A.filled2(), 1 );

    // check new element
    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.index2_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.value_data()[0], 1 );
    // check end of list marker
    BOOST_CHECK_EQUAL( A.index1_data()[1], 1 );

    A.erase_element(0,0);

    BOOST_CHECK_EQUAL( A.nnz(), 0 );
    BOOST_CHECK_EQUAL( A.filled1(), 2 );
    BOOST_CHECK_EQUAL( A.filled2(), 0 );
    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );

}

BOOST_AUTO_TEST_CASE( compressed_matrix_erase_in_the_middle )
{
    boost::numeric::ublas::compressed_matrix<int, boost::numeric::ublas::row_major > A(2, 2);

    BOOST_CHECK_EQUAL( A.filled1(), 1 );
    BOOST_CHECK_EQUAL( A.filled2(), 0 );
    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );

    A.insert_element(0,1,5);

    BOOST_CHECK_EQUAL( A.filled1(), 2 );
    BOOST_CHECK_EQUAL( A.filled2(), 1 );

    // check new element
    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.index2_data()[0], 1 );
    BOOST_CHECK_EQUAL( A.value_data()[0], 5 );
    // check end of list marker
    BOOST_CHECK_EQUAL( A.index1_data()[1], 1 );

    A.insert_element(0,0,4);

    BOOST_CHECK_EQUAL( A.filled1(), 2 );
    BOOST_CHECK_EQUAL( A.filled2(), 2 );

    // check new element
    BOOST_CHECK_EQUAL( A.index2_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.value_data()[0], 4 );
    // check previous element
    BOOST_CHECK_EQUAL( A.index2_data()[1], 1 );
    BOOST_CHECK_EQUAL( A.value_data()[1], 5 );
    // check end of list marker
    BOOST_CHECK_EQUAL( A.index1_data()[1], 2 );

    A.erase_element(0,0);

    BOOST_CHECK_EQUAL( A.filled1(), 2 );
    BOOST_CHECK_EQUAL( A.filled2(), 1 );

    BOOST_CHECK_EQUAL( A.index2_data()[0], 1 );
    BOOST_CHECK_EQUAL( A.value_data()[0], 5 );

    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.index1_data()[1], 1 );

    A.erase_element(0,1);

    BOOST_CHECK_EQUAL( A.filled1(), 2 );
    BOOST_CHECK_EQUAL( A.filled2(), 0 );
    BOOST_CHECK_EQUAL( A.index1_data()[0], 0 );
    BOOST_CHECK_EQUAL( A.index1_data()[1], 0 );

}
