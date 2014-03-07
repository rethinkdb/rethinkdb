// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.



#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"

#include <algorithm>
#include <list>


int
test_main(int, char*[])
{
  typedef boost::multi_array<double, 3> array;
  typedef array::size_type size_type;
  boost::array<size_type,3> sizes = { { 3, 3, 3 } };
  const size_type num_elements = 27;

  // Copy Constructor
  {
    array A(sizes);
    std::vector<double> vals(num_elements, 4.5);
    A.assign(vals.begin(),vals.end());
    array B(A);
    BOOST_CHECK(A == B);
    BOOST_CHECK(B == A);
    BOOST_CHECK(A[0] == B[0]);
  }
  // Assignment Operator
  {
    array A(sizes), B(sizes);
    std::vector<double> vals(num_elements, 4.5);
    A.assign(vals.begin(),vals.end());
    B = A;
    BOOST_CHECK(A == B);
    BOOST_CHECK(B == A);
    BOOST_CHECK(B[0] == A[0]);

    typedef array::index_range range;
    array::index_gen indices;
    array::array_view<2>::type C = A[indices[2][range()][range()]];
    array::array_view<2>::type D = B[indices[2][range()][range()]];
    BOOST_CHECK(C == D);
  }
  // Different Arrays
  {
    array A(sizes), B(sizes);
    std::vector<double> valsA(num_elements, 4.5);
    std::vector<double> valsB(num_elements, 2.5);
    A.assign(valsA.begin(),valsA.end());
    B.assign(valsB.begin(),valsB.end());

    BOOST_CHECK(A != B);
    BOOST_CHECK(B != A);
    BOOST_CHECK(A[0] != B[0]);

    typedef array::index_range range;
    array::index_gen indices;
    array::array_view<2>::type C = A[indices[2][range()][range()]];
    array::array_view<2>::type D = B[indices[2][range()][range()]];
    BOOST_CHECK(C != D);
  }

  // Comparisons galore!
  {
    array A(sizes), B(sizes);

    double valsA[] = {
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,

        1, 1, 1,
        1, 1, 1,
        1, 1, 1,

        2, 2, 2,
        2, 2, 2,
        2, 2, 2
    };

    double valsB[] = {
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,

        1, 1, 1,
        1, 1, 1,
        1, 1, 1,

        2, 2, 2,
        2, 2, 2,
        2, 2, 1
    };

    A.assign(valsA,valsA+num_elements);
    B.assign(valsB,valsB+num_elements);

    BOOST_CHECK(B < A);
    BOOST_CHECK(A > B);

    BOOST_CHECK(B <= A);
    BOOST_CHECK(A >= B);

    BOOST_CHECK(B[0] == A[0]);
    BOOST_CHECK(B[2] < A[2]);

    array C = A;
    
    BOOST_CHECK(C <= A);
    BOOST_CHECK(C >= A);

    BOOST_CHECK(!(C < A));
    BOOST_CHECK(!(C > A));

    typedef array::index_range range;
    array::index_gen indices;
    array::array_view<2>::type D = A[indices[2][range()][range()]];
    array::array_view<2>::type E = B[indices[2][range()][range()]];

    BOOST_CHECK(E < D);
    BOOST_CHECK(E <= D);
  }



  return boost::exit_success;
}


