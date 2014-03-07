// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

//
// constructors.cpp - Testing out the various constructor options
//


#include "boost/test/minimal.hpp"

#include "boost/multi_array.hpp"
#include <algorithm>
#include <list>

void check_shape(const double&, std::size_t*, int*, unsigned int)
{}

template <class Array>
void check_shape(const Array& A,
                 std::size_t* sizes,
                 int* strides,
                 unsigned int num_elements)
{
  BOOST_CHECK(A.num_elements() == num_elements);
  BOOST_CHECK(A.size() == *sizes);
  BOOST_CHECK(std::equal(sizes, sizes + A.num_dimensions(), A.shape()));
  BOOST_CHECK(std::equal(strides, strides + A.num_dimensions(), A.strides()));
  check_shape(A[0], ++sizes, ++strides, num_elements / A.size());
}


bool equal(const double& a, const double& b)
{
  return a == b;
}

template <typename ArrayA, typename ArrayB>
bool equal(const ArrayA& A, const ArrayB& B)
{
  typename ArrayA::const_iterator ia;
  typename ArrayB::const_iterator ib = B.begin();
  for (ia = A.begin(); ia != A.end(); ++ia, ++ib)
    if (!::equal(*ia, *ib))
      return false;
  return true;
}


int
test_main(int, char*[])
{
  typedef boost::multi_array<double, 3>::size_type size_type;
  boost::array<size_type,3> sizes = { { 3, 3, 3 } };
  int strides[] = { 9, 3, 1 };
  size_type num_elements = 27;

  // Default multi_array constructor
  {
    boost::multi_array<double, 3> A;
  }

  // Constructor 1, default storage order and allocator
  {
    boost::multi_array<double, 3> A(sizes);
    check_shape(A, &sizes[0], strides, num_elements);

    double* ptr = 0;
    boost::multi_array_ref<double,3> B(ptr,sizes);
    check_shape(B, &sizes[0], strides, num_elements);

    const double* cptr = ptr;
    boost::const_multi_array_ref<double,3> C(cptr,sizes);
    check_shape(C, &sizes[0], strides, num_elements);
  }

  // Constructor 1, fortran storage order and user-supplied allocator
  {
    typedef boost::multi_array<double, 3,
      std::allocator<double> >::size_type size_type;
    size_type num_elements = 27;
    int col_strides[] = { 1, 3, 9 };

    boost::multi_array<double, 3,
      std::allocator<double> > A(sizes,boost::fortran_storage_order());
    check_shape(A, &sizes[0], col_strides, num_elements);

    double *ptr=0;
    boost::multi_array_ref<double, 3>
      B(ptr,sizes,boost::fortran_storage_order());
    check_shape(B, &sizes[0], col_strides, num_elements);

    const double *cptr=ptr;
    boost::const_multi_array_ref<double, 3>
      C(cptr,sizes,boost::fortran_storage_order());
    check_shape(C, &sizes[0], col_strides, num_elements);
  }

  // Constructor 2, default storage order and allocator
  {
    typedef boost::multi_array<double, 3>::size_type size_type;
    size_type num_elements = 27;

    boost::multi_array<double, 3>::extent_gen extents;
    boost::multi_array<double, 3> A(extents[3][3][3]);
    check_shape(A, &sizes[0], strides, num_elements);

    double *ptr=0;
    boost::multi_array_ref<double, 3> B(ptr,extents[3][3][3]);
    check_shape(B, &sizes[0], strides, num_elements);

    const double *cptr=ptr;
    boost::const_multi_array_ref<double, 3> C(cptr,extents[3][3][3]);
    check_shape(C, &sizes[0], strides, num_elements);
  }

  // Copy Constructors
  {
    typedef boost::multi_array<double, 3>::size_type size_type;
    size_type num_elements = 27;
    std::vector<double> vals(27, 4.5);

    boost::multi_array<double, 3> A(sizes);
    A.assign(vals.begin(),vals.end());
    boost::multi_array<double, 3> B(A);
    check_shape(B, &sizes[0], strides, num_elements);
    BOOST_CHECK(::equal(A, B));

    double ptr[27];
    boost::multi_array_ref<double, 3> C(ptr,sizes);
    A.assign(vals.begin(),vals.end());
    boost::multi_array_ref<double, 3> D(C);
    check_shape(D, &sizes[0], strides, num_elements);
    BOOST_CHECK(C.data() == D.data());

    const double* cptr = ptr;
    boost::const_multi_array_ref<double, 3> E(cptr,sizes);
    boost::const_multi_array_ref<double, 3> F(E);
    check_shape(F, &sizes[0], strides, num_elements);
    BOOST_CHECK(E.data() == F.data());
  }


  // Conversion construction
  {
    typedef boost::multi_array<double, 3>::size_type size_type;
    size_type num_elements = 27;
    std::vector<double> vals(27, 4.5);

    boost::multi_array<double, 3> A(sizes);
    A.assign(vals.begin(),vals.end());
    boost::multi_array_ref<double, 3> B(A);
    boost::const_multi_array_ref<double, 3> C(A);
    check_shape(B, &sizes[0], strides, num_elements);
    check_shape(C, &sizes[0], strides, num_elements);
    BOOST_CHECK(B.data() == A.data());
    BOOST_CHECK(C.data() == A.data());

    double ptr[27];
    boost::multi_array_ref<double, 3> D(ptr,sizes);
    D.assign(vals.begin(),vals.end());
    boost::const_multi_array_ref<double, 3> E(D);
    check_shape(E, &sizes[0], strides, num_elements);
    BOOST_CHECK(E.data() == D.data());
  }

  // Assignment Operator
  {
    typedef boost::multi_array<double, 3>::size_type size_type;
    size_type num_elements = 27;
    std::vector<double> vals(27, 4.5);

    boost::multi_array<double, 3> A(sizes), B(sizes);
    A.assign(vals.begin(),vals.end());
    B = A;
    check_shape(B, &sizes[0], strides, num_elements);
    BOOST_CHECK(::equal(A, B));

    double ptr1[27];
    double ptr2[27];
    boost::multi_array_ref<double, 3> C(ptr1,sizes), D(ptr2,sizes);
    C.assign(vals.begin(),vals.end());
    D = C;
    check_shape(D, &sizes[0], strides, num_elements);
    BOOST_CHECK(::equal(C,D));
  }


  // subarray value_type is multi_array
  {
    typedef boost::multi_array<double,3> array;
    typedef array::size_type size_type;
    size_type num_elements = 27;
    std::vector<double> vals(num_elements, 4.5);

    boost::multi_array<double, 3> A(sizes);
    A.assign(vals.begin(),vals.end());

    typedef array::subarray<2>::type subarray;
    subarray B = A[1];
    subarray::value_type C = B[0];

    // should comparisons between the types work?
    BOOST_CHECK(::equal(A[1][0],C));
    BOOST_CHECK(::equal(B[0],C));
  }
  return boost::exit_success;
}


