// Copyright 2008 Gunter Winkler <guwi17@gmx.de>
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// switch automatic singular check off
#define BOOST_UBLAS_TYPE_CHECK 0

#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include <boost/cstdlib.hpp>

#include "common/testhelper.hpp"

#include <iostream>
#include <sstream>

using namespace boost::numeric::ublas;
using std::string;

static const string matrix_IN = "[3,3]((1,2,2),(2,3,3),(3,4,6))\0";
static const string matrix_LU = "[3,3]((3,4,6),(3.33333343e-01,6.66666627e-01,0),(6.66666687e-01,4.99999911e-01,-1))\0";
static const string matrix_INV= "[3,3]((-3,2,-7.94728621e-08),(1.50000012,0,-5.00000060e-01),(4.99999911e-01,-1,5.00000060e-01))\0";
static const string matrix_PM = "[3](2,2,2)";

int main () {

  typedef float TYPE;

  typedef matrix<TYPE> MATRIX;

  MATRIX A;
  MATRIX LU;
  MATRIX INV;
  
  {
    std::istringstream is(matrix_IN);
    is >> A;
  }
  {
    std::istringstream is(matrix_LU);
    is >> LU;
  }
  {
    std::istringstream is(matrix_INV);
    is >> INV;
  }
  permutation_matrix<>::vector_type temp;
  {
    std::istringstream is(matrix_PM);
    is >> temp;
  }
  permutation_matrix<> PM(temp);

  permutation_matrix<> pm(3);
    
  int result = lu_factorize<MATRIX, permutation_matrix<> >(A, pm);

  assertTrue("factorization completed: ", 0 == result);
  assertTrue("LU factors are correct: ", compare(A, LU));
  assertTrue("permutation is correct: ", compare(pm, PM));

  MATRIX B = identity_matrix<TYPE>(A.size2());

  lu_substitute(A, pm, B);

  assertTrue("inverse is correct: ", compare(B, INV));    

  return (getResults().second > 0) ? boost::exit_failure : boost::exit_success;
}
