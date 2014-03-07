//  Copyright (c) 2011 David Bellot
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UBLAS_NO_ELEMENT_PROXIES
# define BOOST_UBLAS_NO_ELEMENT_PROXIES
#endif

#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_sparse.hpp>
#include <boost/numeric/ublas/matrix_expression.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "libs/numeric/ublas/test/utils.hpp"

using std::cout;
using std::endl;

const double TOL = 1e-15;

template <class AE>
typename AE::value_type mean_square(const boost::numeric::ublas::matrix_expression<AE> &me) {
    typename AE::value_type s(0);
    typename AE::size_type i, j;
    for (i=0; i!= me().size1(); i++) {
        for (j=0; j!= me().size2(); j++) {
          s += boost::numeric::ublas::scalar_traits<typename AE::value_type>::type_abs(me()(i,j));
        }
    }
    return s/me().size1()*me().size2();
}

template<typename T>
bool check_sortedness(const boost::numeric::ublas::coordinate_matrix<T>& matrix) {
  bool result = true;
  typedef boost::numeric::ublas::coordinate_matrix<T> matrix_type;
  typename matrix_type::index_array_type i1 = matrix.index1_data();
  typename matrix_type::index_array_type i2 = matrix.index2_data();
  typename matrix_type::array_size_type size = matrix.filled();

  for (typename matrix_type::array_size_type i = 0; i + 1 < size && result; ++ i) {
    result &= ( (i1[i] < i1[i + 1]) ||
                ((i1[i] == i1[i]) &&
                 (i2[i] < i2[i + 1])) );

  }
  return result;
}

void print_entries(size_t size_x, size_t size_y,
                   const std::vector<std::pair<size_t, size_t> >& entries)
{
  std::cerr << "Error - Size:" << size_x << " x " << size_y << ". Entries: ";
  for (size_t i = 0; i < entries.size(); ++ i) {
    std::cerr << entries[i].first << ", " << entries[i].second << "; ";
  }
  std::cerr << "\n";
}


BOOST_UBLAS_TEST_DEF( test_coordinate_matrix_inplace_merge_random )
{
  const size_t max_repeats = 100;
  const size_t max_size = 100;
  const size_t dim_var = 10;
  const size_t nr_entries = 10;

  for (size_t repeats = 1; repeats < max_repeats; ++repeats ) {
    for (size_t size = 1; size < max_size; size += 5) {
      size_t size_x = size + rand() % dim_var;
      size_t size_y = size + rand() % dim_var;

      boost::numeric::ublas::coordinate_matrix<double> matrix_coord(size_x, size_y);
      boost::numeric::ublas::matrix<double> matrix_dense(size_x, size_y, 0);

      matrix_coord.sort();

      std::vector<std::pair<size_t, size_t> > entries;
      for (int entry = 0; entry < nr_entries; ++ entry) {
        int x = rand() % size_x;
        int y = rand() % size_y;
        entries.push_back(std::make_pair(x, y));
        matrix_coord.append_element(x, y, 1);
        matrix_dense(x, y) += 1;
      }
      matrix_coord.sort();

      {
        bool sorted = check_sortedness(matrix_coord);
        bool identical = mean_square(matrix_coord - matrix_dense) < TOL;
        if (!(sorted && identical)) {
          print_entries(size_x, size_y, entries);
        }
        BOOST_UBLAS_TEST_CHECK( check_sortedness(matrix_coord) );
        BOOST_UBLAS_TEST_CHECK( mean_square(matrix_coord - matrix_dense) < TOL);
      }

      for (int entry = 0; entry < nr_entries; ++ entry) {
        int x = rand() % size_x;
        int y = rand() % size_y;
        entries.push_back(std::make_pair(x, y));
        matrix_coord(x, y) += 1;
        matrix_dense(x, y) += 1;
        matrix_coord.sort();
      }

      {
        bool sorted = check_sortedness(matrix_coord);
        bool identical = mean_square(matrix_coord - matrix_dense) < TOL;
        if (!(sorted && identical)) {
          print_entries(size_x, size_y, entries);
        }
        BOOST_UBLAS_TEST_CHECK( sorted );
        BOOST_UBLAS_TEST_CHECK( identical );
      }
    }
  }
}

int main()
{
    BOOST_UBLAS_TEST_BEGIN();

    BOOST_UBLAS_TEST_DO( test_coordinate_matrix_inplace_merge_random );

    BOOST_UBLAS_TEST_END();

    return EXIT_SUCCESS;;
}
