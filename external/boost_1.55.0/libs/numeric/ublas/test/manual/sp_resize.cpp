/*
 * Copyright (c) 2006 Michael Stevens
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

#include <iostream>

#include <boost/numeric/ublas/vector_sparse.hpp>

typedef double Real;

template <class V>
void printV(const V& v) {
  std::cout << "size: " << v.size() << " nnz_capacity: " << v.nnz_capacity() << " nnz: " << v.nnz() << std::endl;
  for (typename V::const_iterator i = v.begin(); i != v.end(); i++) {
    std::cout << i.index() << ":" << (*i) << "  ";
  }
  std::cout << std::endl;
}

template <class V>
void run_test()
{
  V v(10);

  v[0] = 1;
  v[5] = 1;
  v[8] = 1;
  v[9] = 1;

  printV(v);

  v.resize(9); printV(v);
  v.resize(12); printV(v);
  v.resize(2); printV(v);
  v.resize(0); printV(v);

  v.resize(5); v[0] = 1; printV(v);
  v.resize(5,false); printV(v);
}

int main(int, char **) {

  std::cout << "---- MAPPED ----\n";
  run_test< boost::numeric::ublas::mapped_vector<Real> >();
  std::cout << "---- COMPRESSED ----\n";
  run_test< boost::numeric::ublas::compressed_vector<Real> >();
  std::cout << "---- COORDINATE ----\n";
  run_test< boost::numeric::ublas::coordinate_vector<Real> >();

  return 0;
}

