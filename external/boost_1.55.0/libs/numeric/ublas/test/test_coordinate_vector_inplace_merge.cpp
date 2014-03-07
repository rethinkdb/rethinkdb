//  Copyright (c) 2011 David Bellot
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UBLAS_NO_ELEMENT_PROXIES
# define BOOST_UBLAS_NO_ELEMENT_PROXIES
#endif

#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_sparse.hpp>
#include <boost/numeric/ublas/vector_expression.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "libs/numeric/ublas/test/utils.hpp"

const double TOL = 1e-15;


template <class AE>
typename AE::value_type mean_square(const boost::numeric::ublas::vector_expression<AE> &me) {
    typename AE::value_type s(0);
    typename AE::size_type i;
    for (i=0; i!= me().size(); i++) {
      s += boost::numeric::ublas::scalar_traits<typename AE::value_type>::type_abs(me()(i));
    }
    return s/me().size();
}

template<typename T>
bool check_sortedness(const boost::numeric::ublas::coordinate_vector<T>& vector) {
  bool result = true;
  typedef boost::numeric::ublas::coordinate_vector<T> vector_type;
  typename vector_type::index_array_type idx = vector.index_data();
  typename vector_type::size_type size = vector.filled();

  for (typename vector_type::size_type i = 0; i + 1 < size && result; ++ i) {
    result &= (idx[i] < idx[i + 1]);
  }
  return result;
}

void print_entries(size_t size,
                   const std::vector<size_t>& entries)
{
  std::cerr << "Error entries - Size:" << size << ". Entries: ";
  for (size_t i = 0; i < entries.size(); ++ i) {
    std::cerr << entries[i] << "; ";
  }
  std::cerr << "\n";
}

BOOST_UBLAS_TEST_DEF( test_coordinate_vector_inplace_merge_random )
{
  const size_t max_repeats = 100;
  const size_t max_size = 100;
  const size_t dim_var = 10;
  const size_t nr_entries = 10;

  for (size_t repeats = 1; repeats < max_repeats; ++repeats ) {
    for (size_t size = 1; size < max_size; size += 5) {
      size_t size_vec = size + rand() % dim_var;

      boost::numeric::ublas::coordinate_vector<double> vector_coord(size_vec);
      boost::numeric::ublas::vector<double> vector_dense(size_vec, 0);

      vector_coord.sort();

      std::vector<size_t> entries;
      for (int entry = 0; entry < nr_entries; ++ entry) {
        int x = rand() % size_vec;
        entries.push_back(x);
        vector_coord.append_element(x, 1);
        vector_dense(x) += 1;
      }
      vector_coord.sort();

      {
        bool sorted = check_sortedness(vector_coord);
        bool identical = mean_square(vector_coord - vector_dense) < TOL;
        if (!(sorted && identical)) {
          print_entries(size_vec, entries);
        }
        BOOST_UBLAS_TEST_CHECK( check_sortedness(vector_coord) );
        BOOST_UBLAS_TEST_CHECK( mean_square(vector_coord - vector_dense) < TOL);
      }

      for (int entry = 0; entry < nr_entries; ++ entry) {
        int x = rand() % size_vec;
        entries.push_back(x);
        vector_coord(x) += 1;
        vector_dense(x) += 1;
        vector_coord.sort();
      }

      {
        bool sorted = check_sortedness(vector_coord);
        bool identical = mean_square(vector_coord - vector_dense) < TOL;
        if (!(sorted && identical)) {
          print_entries(size_vec, entries);
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

    BOOST_UBLAS_TEST_DO( test_coordinate_vector_inplace_merge_random );

    BOOST_UBLAS_TEST_END();

    return EXIT_SUCCESS;;
}
