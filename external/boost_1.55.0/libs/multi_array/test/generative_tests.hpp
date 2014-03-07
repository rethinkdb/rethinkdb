#ifndef GENERATIVE_TESTS_RG072001_HPP
#define GENERATIVE_TESTS_RG072001_HPP

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
// generative-tests.hpp - Framework for running tests on all the types
//   of multi_array
//
//  In order to create a set of tests, you must define the following two
//  function signatures:
//   template <typename Array>
//   void access(Array& A, const mutable_array_tag&);
//
//   template <typename Array>
//   void access(Array& A, const const_array_tag&);
//
//  The framework will always pass 2x3x4 arrays into these functions.
//  The const_array_tag version of access must NOT attempt to modify 
//  the array.  Assume that the passed array has constness in this case.
// 
//  The mutable_array_tag version of access should pass the array to the
//  assign() function in order to set its values before running tests.
//
//  If you wish to write your own code to assign data to the array
//  (ie. test the iterators by assigning data with them), you must
//  #define MULTIARRAY_TEST_ASSIGN before including this file.
//  assign() will call this function.
//
//  If you wish to know how many tests were run, you must increment
//  the global variable 'tests_run' somewhere in your test code.
//
//  Since generative-tests uses the Boost.Test framework, you must
//  define at least the following:
//
//  int test_main(int,char*[]) { return run_generative_tests(); }
//
#include "boost/multi_array.hpp"

#include "boost/test/minimal.hpp"

#include <boost/config.hpp> /* BOOST_NO_SFINAE */
#include <algorithm>
#include <iostream>
#include <vector>

namespace {
  unsigned int tests_run = 0;
} // empty namespace 

struct mutable_array_tag { };
struct const_array_tag { };

template <typename Array>
void assign_if_not_const(Array&, const const_array_tag&) {
  // do nothing
}

template <typename Array>
void assign_if_not_const(Array& A, const mutable_array_tag&);

#ifndef MULTIARRAY_TEST_ASSIGN
template <typename Array>
void assign_if_not_const(Array& A, const mutable_array_tag&) {

  typedef typename Array::index index;

  const index idx0 = A.index_bases()[0];
  const index idx1 = A.index_bases()[1];
  const index idx2 = A.index_bases()[2];


  int num = 0;
  for (index i = idx0; i != idx0 + 2; ++i)
    for (index j = idx1; j != idx1 + 3; ++j)
      for (index k = idx2; k != idx2 + 4; ++k) 
        A[i][j][k] = num++;
}
#endif // MULTIARRAY_TEST_ASSIGN

template <typename Array>
void assign(Array& A) {
  assign_if_not_const(A,mutable_array_tag());
}

template <typename Array>
void access(Array& A, const mutable_array_tag&);

template <typename Array>
void access(Array& A, const const_array_tag&);

template <typename StorageOrder3,typename StorageOrder4,typename Modifier>
int run_configuration(const StorageOrder3& so3,
                      const StorageOrder4& so4,
                      const Modifier& modifier) {
  // multi_array
  {
    typedef boost::multi_array<int,3> array;
    typename array::extent_gen extents;
    {
      array A(extents[2][3][4],so3);
      modifier.modify(A);
      access(A,mutable_array_tag());
    }
  }
  // multi_array_ref
  {
    typedef boost::multi_array_ref<int,3> array_ref;
    typename array_ref::extent_gen extents;
    {
      int local[24];
      array_ref A(local,extents[2][3][4],so3);
      modifier.modify(A);
      access(A,mutable_array_tag());
    }
  }
  // const_multi_array_ref
  {
    typedef boost::multi_array_ref<int,3> array_ref;
    typedef boost::const_multi_array_ref<int,3> const_array_ref;
    typename array_ref::extent_gen extents;
    {
      int local[24];
      array_ref A(local,extents[2][3][4],so3);
      modifier.modify(A);
      assign(A);

      const_array_ref B = A;
      access(B,const_array_tag());
    }
  }
  // sub_array
  {
    typedef boost::multi_array<int,4> array;
    typename array::extent_gen extents;
    {
      array A(extents[2][2][3][4],so4);
      modifier.modify(A);
      typename array::template subarray<3>::type B = A[1];
      access(B,mutable_array_tag());
    }
  }
  // const_sub_array
  {
    typedef boost::multi_array<int,4> array;
    typename array::extent_gen extents;
    {
      array A(extents[2][2][3][4],so4);
      modifier.modify(A);
      typename array::template subarray<3>::type B = A[1];
      assign(B);

      typename array::template const_subarray<3>::type C = B;
      access(C,const_array_tag());
    }
  }
  // array_view
  {
    typedef boost::multi_array<int,3> array;
    typedef typename array::index_range range;
    typename array::index_gen indices;
    typename array::extent_gen extents;
    {
      typedef typename array::index index;

      array A(extents[4][5][6],so3);
      modifier.modify(A);
      const index idx0 = A.index_bases()[0];
      const index idx1 = A.index_bases()[1];
      const index idx2 = A.index_bases()[2];

      typename array::template array_view<3>::type B =A[
        indices[range(idx0+1,idx0+3)]
               [range(idx1+1,idx1+4)]
               [range(idx2+1,idx2+5)]
      ];
      access(B,mutable_array_tag());
    }
  }
  // const_array_view
  {
    typedef boost::multi_array<int,3> array;
    typedef typename array::index_range range;
    typename array::index_gen indices;
    typename array::extent_gen extents;
    {
      typedef typename array::index index;

      array A(extents[4][5][6],so3);
      modifier.modify(A);
      const index idx0 = A.index_bases()[0];
      const index idx1 = A.index_bases()[1];
      const index idx2 = A.index_bases()[2];

      typename array::template array_view<3>::type B =A[
        indices[range(idx0+1,idx0+3)]
               [range(idx1+1,idx1+4)]
               [range(idx2+1,idx2+5)]
      ];
      assign(B);

      typename array::template const_array_view<3>::type C = B;
      access(C,const_array_tag());
    }
  }
  return boost::exit_success;
}

template <typename ArrayModifier>
int run_storage_tests(const ArrayModifier& modifier) {
  run_configuration(boost::c_storage_order(),
                    boost::c_storage_order(),modifier);
  run_configuration(boost::fortran_storage_order(),
                    boost::fortran_storage_order(),modifier);
  
  std::size_t ordering[] = {2,0,1,3};
  bool ascending[] = {false,true,true,true};
  run_configuration(boost::general_storage_order<3>(ordering,ascending),
                    boost::general_storage_order<4>(ordering,ascending),
                    modifier); 

  return boost::exit_success;
}

struct null_modifier {
  template <typename Array>
  void modify(Array&) const { }
};

struct set_index_base_modifier {
  template <typename Array>
  void modify(Array& A) const {
#ifdef BOOST_NO_SFINAE
    typedef boost::multi_array_types::index index;
    A.reindex(index(1));
#else
    A.reindex(1);
#endif 
  }
};

struct reindex_modifier {
  template <typename Array>
  void modify(Array& A) const {
    boost::array<int,4> bases = {{1,2,3,4}};
    A.reindex(bases);
 }
};

struct reshape_modifier {
  template <typename Array>
  void modify(Array& A) const {
    typedef typename Array::size_type size_type;
    std::vector<size_type> old_shape(A.num_dimensions());
    std::vector<size_type> new_shape(A.num_dimensions());

    std::copy(A.shape(),A.shape()+A.num_dimensions(),old_shape.begin());
    std::copy(old_shape.rbegin(),old_shape.rend(),new_shape.begin());

    A.reshape(new_shape);
    A.reshape(old_shape);
  }
};

int run_generative_tests() {

  run_storage_tests(null_modifier());
  run_storage_tests(set_index_base_modifier());
  run_storage_tests(reindex_modifier());
  run_storage_tests(reshape_modifier());
  std::cout << "Total Tests Run: " << tests_run << '\n';
  return boost::exit_success;
}

#endif // GENERATIVE_TESTS_RG072001_HPP
