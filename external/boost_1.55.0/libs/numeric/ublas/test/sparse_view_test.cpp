
/* Test program to test find functions of triagular matrices
 *
 * author: Gunter Winkler ( guwi17 at gmx dot de )
 */


// ublas headers

#include <boost/numeric/ublas/experimental/sparse_view.hpp>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/io.hpp>

#include <boost/numeric/ublas/traits/c_array.hpp>

// other boost headers

// headers for testcase

#define BOOST_TEST_MODULE SparseMatrixErasureTest
#include <boost/test/included/unit_test.hpp>

// standard and system headers

#include <iostream>
#include <string>

namespace ublas = boost::numeric::ublas;

    /*
      sparse input matrix:

      1 2 0 0
      0 3 9 0
      0 1 4 0
    */

    static const std::string inputMatrix = "[3,4]((1,2,0,0),(0,3,9,0),(0,1,4,0))\n";

    const unsigned int NNZ  = 6;
    const unsigned int IB   = 1;
    const double VA[]       = { 1.0, 2.0, 3.0, 9.0, 1.0, 4.0 };
    const unsigned int IA[] = { 1, 3, 5, 7 };
    const unsigned int JA[] = { 1, 2, 2, 3, 2, 3 };

BOOST_AUTO_TEST_CASE( test_construction_and_basic_operations )
{

    typedef ublas::matrix<double> DENSE_MATRIX;
    
    // prepare data

    DENSE_MATRIX A;

    std::istringstream iss(inputMatrix);
    iss >> A;

    std::cout << A << std::endl;

    std::cout << ( ublas::make_compressed_matrix_view<ublas::row_major,IB>(3,4,NNZ,IA,JA,VA) ) << std::endl;

    typedef ublas::compressed_matrix_view<ublas::row_major, IB, unsigned int [4], unsigned int [NNZ], double[NNZ]> COMPMATVIEW;

    COMPMATVIEW viewA(3,4,NNZ,IA,JA,VA);

    std::cout << viewA << std::endl;

}



BOOST_AUTO_TEST_CASE( test_construction_from_pointers )
{

    std::cout << ( ublas::make_compressed_matrix_view<ublas::column_major,IB>(4,3,NNZ
                                                                              , ublas::c_array_view<const unsigned int>(4,&(IA[0]))
                                                                              , ublas::c_array_view<const unsigned int>(6,&(JA[0]))
                                                                              , ublas::c_array_view<const double>(6,&(VA[0]))) ) << std::endl;

    unsigned int * ia = new unsigned int[4]();
    unsigned int * ja = new unsigned int[6]();
    double * va = new double[6]();

    std::copy(&(IA[0]),&(IA[4]),ia);
    std::copy(&(JA[0]),&(JA[6]),ja);
    std::copy(&(VA[0]),&(VA[6]),va);

    typedef ublas::compressed_matrix_view<ublas::column_major
      , IB
      , ublas::c_array_view<unsigned int>
      , ublas::c_array_view<unsigned int>
      , ublas::c_array_view<double> > COMPMATVIEW;

    COMPMATVIEW viewA(4,3,NNZ
                      , ublas::c_array_view<unsigned int>(4,ia)
                      , ublas::c_array_view<unsigned int>(6,ja)
                      , ublas::c_array_view<double>(6,va));

    std::cout << viewA << std::endl;

    delete[] va;
    delete[] ja;
    delete[] ia;

}
