#include <iostream>

#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "utils.hpp"

namespace ublas  = boost::numeric::ublas;

static const double TOL(1.0e-5); ///< Used for comparing two real numbers.
static const int n(10);           ///< defines the test matrix size

template<class mat, class vec>
double diff(const mat& A, const vec& x, const vec& b) {
  return ublas::norm_2(prod(A, x) - b);
}

// efficiently fill matrix depending on majority
template<class mat>
void fill_matrix(mat& A, ublas::column_major_tag) {
  for (int i=0; i<n; ++i) {
    if (i-1>=0) {
      A(i-1, i) = -1;
    }
    A(i, i) = 1;
    if (i+1<n) {
      A(i+1, i) = -2;
    }
  }
}
template<class mat>
void fill_matrix(mat& A, ublas::row_major_tag) {
  for (int i=0; i<n; ++i) {
    if (i-1>=0) {
      A(i, i-1) = -1;
    }
    A(i, i) = 1;
    if (i+1<n) {
      A(i, i+1) = -2;
    }
  }
}

template<class mat>
BOOST_UBLAS_TEST_DEF ( test_inplace_solve )
{
  mat A(n, n);
  A.clear();
  fill_matrix(A, typename mat::orientation_category());

  ublas::vector<double>  b(n, 1.0);

  // The test matrix is not triangular, but is interpreted that way by
  // inplace_solve using the lower_tag/upper_tags. For checking, the
  // triangular_adaptor makes A triangular for comparison.
  {
    ublas::vector<double>  x(b);
    ublas::inplace_solve(A, x, ublas::lower_tag());
    BOOST_UBLAS_TEST_CHECK(diff(ublas::triangular_adaptor<mat, ublas::lower>(A), x, b) < TOL);
  }
  {
    ublas::vector<double>  x(b);
    ublas::inplace_solve(A, x, ublas::upper_tag());
    BOOST_UBLAS_TEST_CHECK(diff (ublas::triangular_adaptor<mat, ublas::upper>(A), x, b) < TOL);
  }
  {
    ublas::vector<double>  x(b);
    ublas::inplace_solve(x, A, ublas::lower_tag());
    BOOST_UBLAS_TEST_CHECK(diff (trans(ublas::triangular_adaptor<mat, ublas::lower>(A)), x, b) < TOL);
  }
  {
    ublas::vector<double>  x(b);
    ublas::inplace_solve(x, A, ublas::upper_tag());
    BOOST_UBLAS_TEST_CHECK(diff (trans(ublas::triangular_adaptor<mat, ublas::upper>(A)), x , b) < TOL);
  }
}

int main() {

  // typedefs are needed as macros do not work with "," in template arguments
  typedef ublas::compressed_matrix<double, ublas::row_major>     commat_doub_rowmaj;
  typedef ublas::compressed_matrix<double, ublas::column_major>  commat_doub_colmaj;
  typedef ublas::matrix<double, ublas::row_major>                mat_doub_rowmaj;
  typedef ublas::matrix<double, ublas::column_major>             mat_doub_colmaj;
  typedef ublas::mapped_matrix<double, ublas::row_major>         mapmat_doub_rowmaj;
  typedef ublas::mapped_matrix<double, ublas::column_major>      mapmat_doub_colmaj;
  typedef ublas::coordinate_matrix<double, ublas::row_major>     cormat_doub_rowmaj;
  typedef ublas::coordinate_matrix<double, ublas::column_major>  cormat_doub_colmaj;
  typedef ublas::mapped_vector_of_mapped_vector<double, ublas::row_major> mvmv_doub_rowmaj;
  typedef ublas::mapped_vector_of_mapped_vector<double, ublas::column_major> mvmv_doub_colmaj;

  BOOST_UBLAS_TEST_BEGIN();

#ifdef USE_MATRIX
  BOOST_UBLAS_TEST_DO( test_inplace_solve<mat_doub_rowmaj> );
  BOOST_UBLAS_TEST_DO( test_inplace_solve<mat_doub_colmaj> );
#endif

#ifdef USE_COMPRESSED_MATRIX
  BOOST_UBLAS_TEST_DO( test_inplace_solve<commat_doub_rowmaj> );
  BOOST_UBLAS_TEST_DO( test_inplace_solve<commat_doub_colmaj> );
#endif

#ifdef USE_MAPPED_MATRIX
  BOOST_UBLAS_TEST_DO( test_inplace_solve<mapmat_doub_rowmaj> );
  BOOST_UBLAS_TEST_DO( test_inplace_solve<mapmat_doub_colmaj> );
#endif

#ifdef USE_COORDINATE_MATRIX
  BOOST_UBLAS_TEST_DO( test_inplace_solve<cormat_doub_rowmaj> );
  BOOST_UBLAS_TEST_DO( test_inplace_solve<cormat_doub_colmaj> );
#endif

#ifdef USE_MAPPED_VECTOR_OF_MAPPED_VECTOR
  BOOST_UBLAS_TEST_DO( test_inplace_solve<mvmv_doub_rowmaj> );
  BOOST_UBLAS_TEST_DO( test_inplace_solve<mvmv_doub_colmaj> );
#endif

  BOOST_UBLAS_TEST_END();
}
